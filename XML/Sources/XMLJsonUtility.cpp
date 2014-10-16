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

#include <xercesc/dom/DOMEntity.hpp>
#include <xercesc/dom/DOMNotation.hpp>
#ifdef XERCES_3_0_1
#include <xercesc/dom/DOMLSParser.hpp>
#endif

#include "XMLJsonUtility.h"
#include "IXMLHandler.h"
#include "XMLSaxParser.h"

BEGIN_TOOLBOX_NAMESPACE

#define DOM_ELEMENT						"1"
#define DOM_COMMENT						"8"
#define DOM_ATTRIBUT					"attributes"
#define DOM_TEXT						"3"
#define DOM_CDATA						"4"
#define DOM_PROCESSING_INSTRUCTION_NODE "7"
#define DOM_ENTITY						"6" 
#define DOM_ENTITY_REFERENCE			"5"  
#define DOM_DOCUMENT_TYPE				"10"  
#define DOM_NOTATION					"12" 
#define DOM_DOCUMENT_TYPE_NAME			"name"  
#define DOM_PUBLIC_ID					"publicId"
#define DOM_SYSTEM_ID					"systemId"
#define DOM_NOTATION_NAME				"notationName"
#define DOM_TARGET						"target"
#define DOM_DATA						"data"
#define DOM_NODE_VALUE					"nodeValue"
#define DOM_CHILDREN					"childNodes"
#define DOM_ENTITIES					"entities"
#define DOM_NOTATIONS					"notations"
#define DOM_ROOT						"document"
#define DOM_TYPE						"nodeType"
#define DOM_NAME						"nodeName"

class  VXmlPrefs : public VObject
{
public:
	VXmlPrefs();
	virtual ~VXmlPrefs();

	virtual bool InitFromXml(const sBYTE *in_xml,sLONG in_size, XBOX::VString& outErrorMessage, sLONG& outLineNumber, Boolean inVerifyDTD = true );	// Buffer

	xercesc::DOMDocument *GetDocument(void) const { return fXercesDoc; }

protected:

private:
	xercesc::DOMDocument *fXercesDoc;
#ifdef XERCES_3_0_1
	xercesc::DOMLSParser *fParser;
#else
	xercesc::DOMBuilder *fParser;
#endif
	void _InitDomDocument();

};

class DOMXPathLightErrorHandler : public xercesc::DOMErrorHandler
{
public:
	DOMXPathLightErrorHandler();
	virtual ~DOMXPathLightErrorHandler();
    DOMXPathLightErrorHandler & operator = (const xercesc::DOMErrorHandler &other) {return *this;};
	virtual bool handleError (  const xercesc::DOMError & domError );
	bool wasFatalError() { return fWasFatalError;}
	void GetErrorMessage( VString& outErrorMessage, sLONG& outLineNumber ) { outErrorMessage = fErrorMessage; outLineNumber = fLineNumber; }

protected:
	bool fWasFatalError;
	VString fErrorMessage;
	sLONG fLineNumber;
};

DOMXPathLightErrorHandler::DOMXPathLightErrorHandler()
{
	fWasFatalError = false;
	fLineNumber = 0;
}

DOMXPathLightErrorHandler::~DOMXPathLightErrorHandler()
{
	;
}

bool DOMXPathLightErrorHandler::handleError(  const xercesc::DOMError & domError )
{
	const	XMLCh*		message = domError.getMessage ();
	bool	result = false;

	try
	{
		fErrorMessage.FromUniCString(message);
		xercesc::DOMLocator*	locator = domError.getLocation ();

		VString lineNumber, columnNumber;
		
		fLineNumber = locator->getLineNumber();
		lineNumber.FromLong(fLineNumber);
		columnNumber.FromLong(locator->getColumnNumber());

		fErrorMessage += " [line: ";
		fErrorMessage += lineNumber;
		fErrorMessage += ", column: ";
		fErrorMessage += columnNumber;
		fErrorMessage += "]";

		switch (domError.getSeverity())
		{
			case 1 : result = true;   break;//DOM_SEVERITY_WARNING
			case 2 : result =  false; break;//DOM_SEVERITY_ERROR
			case 3 : result =  false; break;//DOM_SEVERITY_FATAL_ERROR
			default : result =  true; break;
		}
		if (!result)
		{ 
			fWasFatalError = true;
			xbox_assert(false);	// fatal xml error when parsing
		}
	} // try
	catch (...)
	{
		//	just avoid crash ... for now
	}
	return result;

}

VXmlPrefs::VXmlPrefs()
{
	fXercesDoc = NULL;
	fParser = NULL;
}

VXmlPrefs::~VXmlPrefs()
{
	_InitDomDocument();
}

void VXmlPrefs::_InitDomDocument()
{
	if (fParser)
	{
		// tricky: fParser is freeing automatically fXercesDoc

#ifdef XERCES_3_0_1
		xercesc::DOMErrorHandler *errHandler = (xercesc::DOMErrorHandler *)fParser->getDomConfig()->getParameter(xercesc::XMLUni::fgDOMErrorHandler);
		if (errHandler)
			delete errHandler;
        fParser->getDomConfig()->setParameter(xercesc::XMLUni::fgDOMErrorHandler, (const void *)NULL);
#else
		xercesc::DOMErrorHandler *l_free = fParser->getErrorHandler();
		delete l_free;
		fParser->setErrorHandler(NULL);
#endif

		delete fParser;
		fXercesDoc = NULL;
		fParser = NULL;
	}

	if (fXercesDoc)
		delete fXercesDoc;

	fXercesDoc = NULL;
	fParser = NULL;
}

bool VXmlPrefs::InitFromXml(const sBYTE *in_xml,sLONG in_size, VString& outErrorMessage, sLONG& outLineNumber, Boolean inVerifyDTD )
{
	bool retVal = false;

	if (!in_xml || !in_size)
	{
		_InitDomDocument();
		retVal = true;
	}
	else
	{
		// build DOM tree from existing XML...
		static const XMLCh gLS[] = { xercesc::chLatin_L, xercesc::chLatin_S, xercesc::chNull };

		xercesc::DOMImplementationLS *impl = xercesc::DOMImplementationRegistry::getDOMImplementation(gLS);

#ifdef XERCES_3_0_1
		fParser = impl->createLSParser(xercesc::DOMImplementationLS::MODE_SYNCHRONOUS, 0);

		xercesc::DOMConfiguration* cfg=fParser->getDomConfig();
		if(cfg) cfg->setParameter(xercesc::XMLUni::fgDOMNamespaces,true);	// m.c, needed to work with getLocalname (jmo - updated for xerces 3.0.1)
#else
		fParser = impl->createDOMBuilder(xercesc::DOMImplementationLS::MODE_SYNCHRONOUS, 0);

		fParser->setFeature(xercesc::XMLUni::fgDOMNamespaces,true);	// m.c, needed to work with getLocalname
#endif
		static const sBYTE*  MemBufId = "XMLBob";

		xercesc::MemBufInputSource *memsource = new xercesc::MemBufInputSource((uBYTE*) in_xml,in_size,MemBufId,false);
		xercesc::Wrapper4InputSource *srcToUse = new xercesc::Wrapper4InputSource(memsource,false);	// important: false to keep ownership (m.c)

#ifndef XERCES_3_0_1
		XMLCh tmpStr[150];
#endif
		if(!inVerifyDTD)
		{
#ifdef XERCES_3_0_1
			if(cfg) cfg->setParameter(xercesc::XMLUni::fgDOMValidate, false);
#else
			xercesc::XMLString::transcode("Validation",tmpStr,149);
			fParser->setFeature((XMLCh*)tmpStr,false);
#endif	
#ifdef XERCES_3_0_1
			if(cfg) cfg->setParameter(xercesc::XMLUni::fgXercesLoadExternalDTD, false);
#else
			xercesc::XMLString::transcode("http://apache.org/xml/features/nonvalidating/load-external-dtd",tmpStr,149);
			fParser->setFeature((XMLCh*)tmpStr,false);
#endif
		}

#ifdef XERCES_3_0_1
		if(cfg) cfg->setParameter(xercesc::XMLUni::fgXercesValidationErrorAsFatal, true);
#else
		xercesc::XMLString::transcode("http://apache.org/xml/features/validation-error-as-fatal",tmpStr,149);
		fParser->setFeature((XMLCh*)tmpStr,true);
#endif	
#ifdef XERCES_3_0_1
		if(cfg) cfg->setParameter(xercesc::XMLUni::fgXercesContinueAfterFatalError, false);
#else
		xercesc::XMLString::transcode("http://apache.org/xml/features/continue-after-fatal-error",tmpStr,149);
		fParser->setFeature((XMLCh*)tmpStr,false);
#endif	

		DOMXPathLightErrorHandler *l_errorhandler = new DOMXPathLightErrorHandler();
#ifdef XERCES_3_0_1
		if(cfg) cfg->setParameter(xercesc::XMLUni::fgDOMErrorHandler, static_cast<xercesc::DOMErrorHandler *>(l_errorhandler));
#else
		fParser->setErrorHandler(static_cast<xercesc::DOMErrorHandler *>(l_errorhandler));
#endif

		try
		{
#ifdef XERCES_3_0_1
			fXercesDoc = fParser->parse(srcToUse);
#else
			fXercesDoc = fParser->parse(*srcToUse);
#endif
		}
		catch (...)
		{
			//	just avoid crashing...
			;
		}

		if (!l_errorhandler->wasFatalError())
		{
			if (fXercesDoc)
			{
				xercesc::DOMNode *rootelement = (xercesc::DOMNode*) fXercesDoc->getDocumentElement();
				if (rootelement && rootelement->getNodeType() == xercesc::DOMNode::ELEMENT_NODE)
				{
					retVal = true;	// valid xml
				}
			}
		}
		else
		{
			l_errorhandler->GetErrorMessage( outErrorMessage, outLineNumber );
			retVal = false;	// invalid xml
		}
		
		delete srcToUse;
		delete memsource;
	}

	return retVal;
}

static void FormatToJSONString(VString& ioString);
static void FormatToJSONString(VString& ioString)
{
	VString vn((UniChar)0x0A);
	VString vr((UniChar)0x0D);
	VString vt((UniChar)0x09);

	ioString.ExchangeAll("\\","\\\\");
	ioString.ExchangeAll("/","\\/");
	ioString.ExchangeAll("\"","\\\"");
	ioString.ExchangeAll(vn,"\\n");
	ioString.ExchangeAll(vr,"\\r");
	ioString.ExchangeAll(vt,"\\t");
}

static void FormatFromJSONString(VString& ioString);
static void FormatFromJSONString(VString& ioString)
{
	VString vn((UniChar)0x0A);
	VString vr((UniChar)0x0D);
	VString vt((UniChar)0x09);

	ioString.ExchangeAll("\\n",vn);
	ioString.ExchangeAll("\\r",vr);
	ioString.ExchangeAll("\\t",vt);
	ioString.ExchangeAll("\\\"","\"");
	ioString.ExchangeAll("\\/","/");
	ioString.ExchangeAll("\\\\","\\");
	ioString.ExchangeAll( CVSTR("&"), CVSTR("&amp;"));
	ioString.ExchangeAll( CVSTR("<"), CVSTR("&lt;"));
	ioString.ExchangeAll( CVSTR(">"), CVSTR("&gt;"));
}

static void FormatFromJSONComment(VString& ioString);
static void FormatFromJSONComment(VString& ioString)
{
	VString vn((UniChar)0x0A);
	VString vr((UniChar)0x0D);
	VString vt((UniChar)0x09);

	ioString.ExchangeAll("\\n",vn);
	ioString.ExchangeAll("\\r",vr);
	ioString.ExchangeAll("\\t",vt);
	ioString.ExchangeAll("\\\"","\"");
	ioString.ExchangeAll("\\/","/");
	ioString.ExchangeAll("\\\\","\\");
}

class PaserVString : public XBOX::VObject
{
public:
	PaserVString(const XBOX::VString* inString){fString = inString;fCurrentIndex=0;};
	~PaserVString(){};

	UniChar	GetNextChar();
	VString GetUntilChar(UniChar inChar);
	void Skip(sLONG inCountToSkip){fCurrentIndex+=inCountToSkip;};
	bool EndParsing(){return (fString->GetLength()<= fCurrentIndex);};
		
private:

	const VString*		fString;
	sLONG				fCurrentIndex;
};

UniChar PaserVString::GetNextChar()
{
	UniChar result = 0;
	if(!EndParsing())
	{
		result = (*fString)[fCurrentIndex];
		fCurrentIndex++;
	}

	return result;
}

VString PaserVString::GetUntilChar(UniChar inChar)
{
	VString result;
	bool doit = true;
	UniChar previouschar = 0;

	while(!EndParsing() && doit )
	{
		UniChar thechar = GetNextChar();

		if(inChar==thechar)
		{
			if( previouschar == '\\' && (inChar == '"'))
				result += thechar;
			else
				doit = false;
		}
		else
			result += thechar;
		previouschar = thechar;
	}

	return result;
}

class JsonObject : public XBOX::VObject, public XBOX::IRefCountable
{
public:
	JsonObject(XBOX::VString& inType, bool inToXHTML){fType = inType;fIsXHTML = inToXHTML;};
	~JsonObject()
	{
		for( VectorOfJson::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			(*i)->Release();
		}
	};

	void AddChild(JsonObject* inValue)
	{
		if(inValue)
		{
			fChildren.push_back(inValue);
			inValue->Retain();
		}
	}

	void AddAttribut(VString& inKey, VString& inValue)
	{
		fAttributs[inKey] = inValue;
	}

	void SetType(XBOX::VString& inType){fType = inType;}

	VError GetXml(VString& outXml, bool inWithCR = false);
		
private:

	typedef std::map<VString, VString>	MapOfAttributs;
	typedef std::vector<JsonObject*>	VectorOfJson;
	VString								fType;
	MapOfAttributs						fAttributs;
	VectorOfJson						fChildren;
	bool								fIsXHTML;
};

VError JsonObject::GetXml(VString& outXML, bool inWithCR)
{			
	VError verr = VE_OK;
	VString data;

	if( fType == DOM_TEXT)
	{
		data = fAttributs[DOM_NODE_VALUE];
		FormatFromJSONString(data);
		outXML += data;
	}
	else if( fType == DOM_CDATA)
	{
		data = fAttributs[DOM_NODE_VALUE];
		FormatFromJSONString(data);
		outXML += "<![CDATA[";
		outXML += data;
		outXML += "]]>";
	}
	else if( fType == DOM_COMMENT)
	{
		data = fAttributs[DOM_NODE_VALUE];
		FormatFromJSONComment(data);
		outXML += "<!--";
		outXML += data;
		outXML += "-->";
	}
	else if( fType == DOM_PROCESSING_INSTRUCTION_NODE)
	{
		data = fAttributs[DOM_DATA];
		FormatFromJSONString(data);

		outXML += "<?";
		outXML += fAttributs[DOM_TARGET];
		outXML += " ";
		outXML += data;
		outXML += "?>";
	}
	else if( fType == DOM_ELEMENT)
	{	
		VString vattributs;
		outXML += "<";
		outXML += fAttributs[DOM_NAME];

		vattributs = fAttributs[DOM_ATTRIBUT];
		if(vattributs.GetLength()>0)
		{
			PaserVString parser(&vattributs);
			while(!parser.EndParsing())
			{
				UniChar thechar = parser.GetNextChar();
				if(thechar == '"')
				{
					VString att = parser.GetUntilChar('"');
					outXML += " ";
					outXML += att+"=";
					att = parser.GetUntilChar(':');
					att = parser.GetUntilChar('"');
					att = parser.GetUntilChar('"');
					FormatFromJSONString(att);
					att.ExchangeAll("\"", "&quot;");
					outXML += "\"";
					outXML += att;
					outXML += "\"";
				}
			}
		}
		
		if(fChildren.size() > 0)
		{
			outXML += ">";

			for( VectorOfJson::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
			{
				(*i)->GetXml(outXML, inWithCR);
			}

			outXML += "</";
			outXML += fAttributs[DOM_NAME];
			outXML += ">";
		}
		else if(fIsXHTML)
		{
			bool doit = true;
			VString elementName = fAttributs[DOM_NAME];
			
			//Il y a des cles HTML qui ne supporte pas la syntax xml <elem></elem>, il faut imperativement utiliser <elem/>
			if( elementName == "area" || elementName == "br" || elementName == "hr" || elementName == "img" || elementName == "input" || elementName == "link" || elementName == "meta" || elementName == "param" )
				doit = false;
			if(doit)
			{
				outXML += ">";
				outXML += "</";
				outXML += fAttributs[DOM_NAME];
				outXML += ">";
			}
			else 
				outXML += "/>";

		}
		else
			outXML += "/>";
	}
	else if( fType == DOM_DOCUMENT_TYPE)
	{
		outXML += "<!DOCTYPE ";
		outXML += fAttributs[DOM_DOCUMENT_TYPE_NAME];
		VString publicId, systemId;
		publicId = fAttributs[DOM_PUBLIC_ID];
		systemId = fAttributs[DOM_SYSTEM_ID];
		if(publicId.GetLength() > 0)
		{
			FormatFromJSONString(publicId);
			outXML += " PUBLIC \"";
			outXML += publicId;
			outXML += "\"";
			if(systemId.GetLength() > 0)
			{
				FormatFromJSONString(systemId);
				outXML += " \"";
				outXML += systemId;
				outXML += "\"";
			}
		}
		else if(systemId.GetLength() > 0)
		{
			outXML += " SYSTEM \"";
			outXML += systemId;
			outXML += "\"";
		}

		outXML += ">";
	}
	else if( fType == DOM_ROOT)
	{
		for( VectorOfJson::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			(*i)->GetXml(outXML, inWithCR);
		}
	}
		/*case xercesc::DOMNode::NOTATION_NODE:
			//tbd
			break;
		case xercesc::DOMNode::ENTITY_REFERENCE_NODE:
			//tbd
			break;
		case xercesc::DOMNode::ENTITY_NODE:
			//
			//xercesc::DOMEntity *nodePI = static_cast<xercesc::DOMEntity *>(childelement);
			xercesc::DOMEntity *nodePI = (xercesc::DOMEntity *)(childelement);
			xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
			//if (nodePI->getPublicId() && nodePI->getSystemId() && nodePI->getNotationName)
			{
				VString publicid( const_cast<XMLCh*>(nodePI->getPublicId()), (VSize) -1);
				VString systemid( const_cast<XMLCh*>(nodePI->getSystemId()), (VSize) -1);
				VString notationname( const_cast<XMLCh*>(nodePI->getNotationName()), (VSize) -1);
				//outJson		
				if(!hasElement)
				{
					outJson+=",\"";
					outJson+=DOM_CHILDREN;
					outJson+="\":[";
					hasElement = true;
				}
				else
					outJson+=",";

				FormatToJSONString(publicid);
				FormatToJSONString(systemid);
				FormatToJSONString(notationname);

				outJson+="{\"";
				outJson+=DOM_TYPE;
				outJson+="\":";
				outJson+=DOM_ENTITY;
				outJson+=",\"";
				outJson+=DOM_PUBLIC_ID;
				outJson+="\":\"";
				outJson+=publicid;
				outJson+="\",\"";
				outJson+=DOM_SYSTEM_ID;
				outJson+="\":\"";
				outJson+=systemid;
				outJson+="\",\"";
				outJson+=DOM_NOTATION_NAME;
				outJson+="\":\"";
				outJson+=notationname;
				outJson+="\"}";
				}
			break;*/

	if ( inWithCR && outXML.GetLength() > 0 && outXML[ outXML.GetLength() - 1 ] == '>' )
		outXML.AppendUniChar( 13 );

	return verr;
}

static bool ParseElement( xercesc::DOMNode *inElement, VString& outJson);
static bool ParseNode( xercesc::DOMNode *inNode, VString& outJson, bool inFirst = false);

static bool ParseNode( xercesc::DOMNode *inNode, VString& outJson, bool inFirst)
{
	bool ok = true;
	bool hasElement = false;
	xercesc::DOMNode *childelement = inNode->getFirstChild();
	while( childelement)
	{
		switch( childelement->getNodeType())
		{
			case xercesc::DOMNode::ELEMENT_NODE:
				{
					if(!hasElement)
					{
						if(!inFirst)
							outJson+=",\"";
						else
							outJson+="\"";
						outJson+=DOM_CHILDREN;
						outJson+="\":[";
						hasElement = true;
					}
					else
						outJson+=",";
					ok = ParseElement( childelement, outJson);
					break;
				}
			
			case xercesc::DOMNode::TEXT_NODE:
				{
					if(!hasElement)
					{
						outJson+=",\"";
						outJson+=DOM_CHILDREN;
						outJson+="\":[";
						hasElement = true;
					}
					else
						outJson+=",";
					const XBOX::VString text( const_cast<XMLCh*>(childelement->getNodeValue()), (XBOX::VSize) -1);
					XBOX::VString vdata = text;

					FormatToJSONString(vdata);
					outJson+="{\"";
					outJson+=DOM_TYPE;
					outJson+="\":";
					outJson+=DOM_TEXT;
					outJson+=",\"";
					outJson+=DOM_NODE_VALUE;
					outJson+="\":\"";
					outJson+=vdata;
					outJson+="\"}";
					break;
				}
			
			case xercesc::DOMNode::CDATA_SECTION_NODE:
				{
					const XMLCh *data = childelement->getNodeValue();
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					XBOX::VString text( (const UniChar*)data);
					//outJson
					FormatToJSONString(text);
					outJson+=",";
					outJson+="{\"";
					outJson+=DOM_TYPE;
					outJson+="\":";
					outJson+=DOM_CDATA;
					outJson+=",\"";
					outJson+=DOM_NODE_VALUE;
					outJson+="\":\"";
					outJson+=text;
					outJson+="\"}";
					
					//inHandler->SetText( text);
					break;
				}
			case xercesc::DOMNode::COMMENT_NODE:
				{
					if(!hasElement)
					{
						outJson+=",\"";
						outJson+=DOM_CHILDREN;
						outJson+="\":[";
						hasElement = true;
					}
					else
						outJson+=",";
					const XBOX::VString text( const_cast<XMLCh*>(childelement->getNodeValue()), (XBOX::VSize) -1);
					XBOX::VString vdata = text;

					FormatToJSONString(vdata);
					outJson+="{\"";
					outJson+=DOM_TYPE;
					outJson+="\":";
					outJson+=DOM_COMMENT;
					outJson+=",\"";
					outJson+=DOM_NODE_VALUE;
					outJson+="\":\"";
					outJson+=vdata;
					outJson+="\"}";
					break;
				}
			case xercesc::DOMNode::PROCESSING_INSTRUCTION_NODE:
				{
					xercesc::DOMProcessingInstruction *nodePI = static_cast<xercesc::DOMProcessingInstruction *>(childelement);
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					if (nodePI->getTarget() && nodePI->getData())
					{
						XBOX::VString targetName( const_cast<XMLCh*>(nodePI->getTarget()), (XBOX::VSize) -1);
						XBOX::VString dataName( const_cast<XMLCh*>(nodePI->getData()), (XBOX::VSize) -1);
						//outJson		
						if(!hasElement)
						{
							outJson+=",\"";
							outJson+=DOM_CHILDREN;
							outJson+="\":[";
							hasElement = true;
						}
						else
							outJson+=",";

						FormatToJSONString(targetName);
						FormatToJSONString(dataName);
						outJson+="{\"";
						outJson+=DOM_TYPE;
						outJson+="\":";
						outJson+=DOM_PROCESSING_INSTRUCTION_NODE;
						outJson+=",\"";
						outJson+=DOM_TARGET;
						outJson+="\":\"";
						outJson+=targetName;
						outJson+="\",\"";
						outJson+=DOM_DATA;
						outJson+="\":\"";
						outJson+=dataName;
						outJson+="\"}";
						}
					break;
				}
			case xercesc::DOMNode::DOCUMENT_TYPE_NODE:
				{
					xercesc::DOMDocumentType *nodePI = static_cast<xercesc::DOMDocumentType *>(childelement);
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					VString docname(nodePI->getName());
					VString publicid(nodePI->getPublicId());
					VString systemid(nodePI->getSystemId());

					hasElement = true;

					outJson+="{\"";
					outJson+=DOM_TYPE;
					outJson+="\":";
					outJson+=DOM_DOCUMENT_TYPE;
					outJson+=",\"";
					outJson+=DOM_DOCUMENT_TYPE_NAME;
					outJson+="\":\""+docname+"\"";

					if(publicid.GetLength() > 0)
					{
						FormatToJSONString(publicid);
						outJson+=",\"";
						outJson+=DOM_PUBLIC_ID;
						outJson+="\":\"";
						outJson+= publicid+"\"";
					}
					
					if(systemid.GetLength() > 0)
					{
						FormatToJSONString(systemid);
						outJson+=",\"";
						outJson+=DOM_SYSTEM_ID;
						outJson+="\":\""+systemid+"\"";
					}

					//if(systemid.GetLength()<=0 && publicid.GetLength() <= 0)
					{//ENTITY
						xercesc::DOMNamedNodeMap * entities = nodePI->getEntities();
						xercesc::DOMNamedNodeMap * notations = nodePI->getNotations();

						if(entities)
						{
							sLONG count = entities->getLength();
							if(count > 0)
							{
								outJson+=",\"";
								outJson+=DOM_ENTITIES;
								outJson+="\":[";
							}

							for(sLONG i=0;i<count;i++)
							{
								xercesc::DOMNode* thenode = entities->item(i);
								xercesc::DOMEntity *nodePI2 = static_cast<xercesc::DOMEntity *>(thenode);
								if(nodePI2)
								{
									xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
									
									VString publicid2( const_cast<XMLCh*>(nodePI2->getPublicId()), (VSize) -1);
									VString systemid2( const_cast<XMLCh*>(nodePI2->getSystemId()), (VSize) -1);
									VString notationname( const_cast<XMLCh*>(nodePI2->getNotationName()), (VSize) -1);
									VString name( const_cast<XMLCh*>(nodePI2->getNodeName()), (VSize) -1);
									VString value( const_cast<XMLCh*>(nodePI2->getNodeValue()), (VSize) -1);
										
									if(i!=0)
										outJson+=",";

									FormatToJSONString(publicid2);
									FormatToJSONString(systemid2);
									FormatToJSONString(notationname);

									outJson+="{\"";
									outJson+=DOM_TYPE;
									outJson+="\":";
									outJson+=DOM_ENTITY;
									outJson+=",\"";
									outJson+=DOM_NAME;
									outJson+="\":\"";
									outJson+=name;
									outJson+=",\"";
									outJson+=DOM_PUBLIC_ID;
									outJson+="\":\"";
									outJson+=publicid2;
									outJson+="\",\"";
									outJson+=DOM_SYSTEM_ID;
									outJson+="\":\"";
									outJson+=systemid2;
									outJson+="\",\"";
									outJson+=DOM_NOTATION_NAME;
									outJson+="\":\"";
									outJson+=notationname;
									outJson+="\"}";
								}
							}

							if(count > 0)
								outJson+="]";
						}

						if(notations)
						{
							sLONG count = notations->getLength();
							if(count > 0)
							{
								outJson+=",\"";
								outJson+=DOM_NOTATIONS;
								outJson+="\":[";
							}

							for(sLONG i=0;i<count;i++)
							{
								xercesc::DOMNode* thenode = notations->item(i);
								xercesc::DOMNotation *nodePI2 = static_cast<xercesc::DOMNotation *>(thenode);
								if(nodePI2)
								{
									xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
									
									VString publicid2( const_cast<XMLCh*>(nodePI2->getPublicId()), (VSize) -1);
									VString systemid2( const_cast<XMLCh*>(nodePI2->getSystemId()), (VSize) -1);
										
									if(i!=0)
										outJson+=",";

									FormatToJSONString(publicid2);
									FormatToJSONString(systemid2);

									outJson+="{\"";
									outJson+=DOM_TYPE;
									outJson+="\":";
									outJson+=DOM_NOTATION;
									outJson+=",\"";
									outJson+=DOM_PUBLIC_ID;
									outJson+="\":\"";
									outJson+=publicid2;
									outJson+="\",\"";
									outJson+=DOM_SYSTEM_ID;
									outJson+="\":\"";
									outJson+=systemid2;
									outJson+="\"}";
								}
							}

							if(count > 0)
								outJson+="]";
						}
					}
					outJson+="}";
					break;
				}
			case xercesc::DOMNode::NOTATION_NODE:
				//tbd
				break;
			case xercesc::DOMNode::ENTITY_REFERENCE_NODE:
				//tbd
				break;
			case xercesc::DOMNode::ENTITY_NODE:
				{
					assert(false);
					/*xercesc::DOMEntity *nodePI = static_cast<xercesc::DOMEntity *>(childelement);
					//xercesc::DOMEntity *nodePI = (xercesc::DOMEntity *)(childelement);
					xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
					//if (nodePI->getPublicId() && nodePI->getSystemId() && nodePI->getNotationName)
					{
						VString publicid( const_cast<XMLCh*>(nodePI->getPublicId()), (VSize) -1);
						VString systemid( const_cast<XMLCh*>(nodePI->getSystemId()), (VSize) -1);
						VString notationname( const_cast<XMLCh*>(nodePI->getNotationName()), (VSize) -1);
						//outJson		
						if(!hasElement)
						{
							outJson+=",\"";
							outJson+=DOM_CHILDREN;
							outJson+="\":[";
							hasElement = true;
						}
						else
							outJson+=",";

						FormatToJSONString(publicid);
						FormatToJSONString(systemid);
						FormatToJSONString(notationname);

						outJson+="{\"";
						outJson+=DOM_TYPE;
						outJson+="\":";
						outJson+=DOM_ENTITY;
						outJson+=",\"";
						outJson+=DOM_PUBLIC_ID;
						outJson+="\":\"";
						outJson+=publicid;
						outJson+="\",\"";
						outJson+=DOM_SYSTEM_ID;
						outJson+="\":\"";
						outJson+=systemid;
						outJson+="\",\"";
						outJson+=DOM_NOTATION_NAME;
						outJson+="\":\"";
						outJson+=notationname;
						outJson+="\"}";
					}*/
					break;
				}
			case xercesc::DOMNode::ATTRIBUTE_NODE:
			case xercesc::DOMNode::DOCUMENT_NODE:
			case xercesc::DOMNode::DOCUMENT_FRAGMENT_NODE:
				break;
		}
			
		childelement = childelement->getNextSibling();
	}

	if(hasElement)
		outJson+="]";

	return ok;
}

static bool ParseElement( xercesc::DOMNode *inElement, VString& outJson)
{
	bool ok = false;
	const XBOX::VString elementName( const_cast<XMLCh*>(inElement->getNodeName()), (XBOX::VSize) -1);

	outJson+="{\"";
	outJson+=DOM_TYPE;
	outJson+="\":";
	outJson+=DOM_ELEMENT;
	outJson+=",\"";
	outJson+=DOM_NAME;
	outJson+="\":\"";
	outJson+=elementName;
	outJson+="\"";

	//IXMLHandler *handler = inHandler->StartElement( elementName);
	//if (handler != NULL)
	{
		xercesc::DOMNamedNodeMap* attributes = inElement->getAttributes(); 

		if (attributes != NULL)
		{
			int len = attributes->getLength();
			for( int index = 0; index < len; index++)
			{
				xercesc::DOMNode *attNode = attributes->item( len - 1 - index);
				if (attNode)
				{
					const XBOX::VString attributeName( const_cast<XMLCh*>(attNode->getNodeName()), (XBOX::VSize) -1);
					XBOX::VString attributeValue( const_cast<XMLCh*>(attNode->getNodeValue()), (XBOX::VSize) -1);
					//outJson
					FormatToJSONString(attributeValue);
					if(index==0)
					{
						outJson+=",\"";
						outJson+=DOM_ATTRIBUT;
						outJson+="\":{\"";
						outJson+=attributeName;
						outJson+="\":";
						outJson+="\"";
						outJson+=attributeValue;
						outJson+="\"";						
					}
					else
					{
						outJson+=",\"";
						outJson+=attributeName;
						outJson+="\":";
						outJson+="\"";
						outJson+=attributeValue;
						outJson+="\"";
					}

					if(index==len-1)
					{
						outJson+="}";
					}
				}
			}
		}
		
		ok = ParseNode( inElement, outJson);

		outJson+="}";
		//handler->EndElement( elementName);
		//handler->Release();
	}
	return ok;
}

static bool Parse( xercesc::DOMNode*node/*XBOX::VXMLDOMNodeRef inNode*/, XBOX::VString& outJson)
{
	//xercesc::DOMNode *node = reinterpret_cast<xercesc::DOMNode *>( inNode);

	if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE)
		return ParseElement( node, outJson);
	else
		return ParseNode( node, outJson, true);
}

////////////////////
static void ParseArray(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML);
static void ParseJSONAttributes(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML);
static void ParseJSONElement(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML);
static void ParseObject(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML);

static void ParseArray(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML)
{
	bool doit = !parser.EndParsing();
	UniChar theChar = 0;
	while(doit)
	{
		theChar = parser.GetNextChar();
		
		if(theChar==']')
		{
			doit = false;
		}
		else
		{
			if(theChar=='{')
			{
				VString empty;
				JsonObject* vJsonObj = new JsonObject(empty, inToXHTML);

				ioJsonObj.AddChild(vJsonObj);
				ParseObject(*vJsonObj, parser, inToXHTML);
				vJsonObj->Release();
			}
			doit = !parser.EndParsing();
		}
	}
}

static void ParseJSONAttributes(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML)
{
	VString dummy, value, key;
	bool inDoubleCote = false;
	dummy = parser.GetUntilChar('{');

	while(!parser.EndParsing())
	{
		UniChar thechar = parser.GetNextChar();
		if(thechar == '}')
			break;
		else if(thechar == '"')
		{
			dummy = parser.GetUntilChar('"');
			value+='"';
			value+=dummy;
			value+='"';
		}
		else
		{
			value+=thechar;
		}
	}

	key = DOM_ATTRIBUT;
	ioJsonObj.AddAttribut(key, value);
}

static void ParseJSONElement(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML)
{
	XBOX::VString type, dummy, attribut, key, value; 

	attribut = parser.GetUntilChar('"');
	dummy = parser.GetUntilChar(':');
	
	if(attribut == DOM_ATTRIBUT)
		ParseJSONAttributes(ioJsonObj, parser, inToXHTML);
	else if( attribut == DOM_CHILDREN || attribut == DOM_ENTITIES || attribut == DOM_NOTATIONS) 
	{
		dummy = parser.GetUntilChar('[');
		if(!parser.EndParsing())
			ParseArray(ioJsonObj,parser, inToXHTML);
	}
	else if(attribut == DOM_TYPE)
	{
		UniChar thechar = 0;
		while(!parser.EndParsing())
		{
			thechar = parser.GetNextChar();
			if(thechar == ',' || thechar == '}')
				break;
			else
			{
				value += thechar;
			}
		}

		sLONG thetype = value.GetLong();
		dummy = DOM_TEXT;	
		if( thetype == dummy.GetLong()) 
			type = DOM_TEXT;
		else 
		{
			dummy = DOM_CDATA;
			if( thetype == dummy.GetLong()) 
				type = DOM_CDATA;
			else 
			{
				dummy = DOM_COMMENT;
				if( thetype == dummy.GetLong()) 
					type = DOM_COMMENT;
				else 
				{
					dummy = DOM_PROCESSING_INSTRUCTION_NODE;
					if( thetype == dummy.GetLong()) 
						type = DOM_PROCESSING_INSTRUCTION_NODE;
					else
					{
						dummy = DOM_ELEMENT;
						if( thetype == dummy.GetLong()) 
							type = DOM_ELEMENT;
						else 
						{
							dummy = DOM_DOCUMENT_TYPE;
							if( thetype == dummy.GetLong())
								type = DOM_DOCUMENT_TYPE;
						}
					}
				}
			}
		}

		ioJsonObj.SetType(type);
		
	}
	else
	{
		dummy = parser.GetUntilChar('"');
		value = parser.GetUntilChar('"');
		ioJsonObj.AddAttribut(attribut,value);
	}
	
}

static void ParseObject(JsonObject& ioJsonObj, PaserVString& parser, bool inToXHTML)
{
	bool doit = !parser.EndParsing();
	UniChar theChar = 0;
	while(doit)
	{
		theChar = parser.GetNextChar();
		
		if(theChar=='}')
		{
			doit = false;
		}
		else
		{
			if(theChar=='{')
			{
				ParseObject(ioJsonObj, parser, inToXHTML);
			}
			else if(theChar=='"')
			{
				ParseJSONElement(ioJsonObj, parser, inToXHTML);
			}

			doit = !parser.EndParsing();
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

VXMLJsonUtility::VXMLJsonUtility()
{
}

VXMLJsonUtility::~VXMLJsonUtility()
{
}

VError VXMLJsonUtility::XMLToJson(const VString& inXML, VString& outJson, VString& outErrorMessage, sLONG& outLineNumber )
{
	VError err = VE_UNKNOWN_ERROR;
	VXmlPrefs	xmlData;
	sLONG		len = (inXML.GetLength()+1)*sizeof(UniChar);
	sBYTE*		buffer = (sBYTE*)malloc(len);
	len = inXML.ToBlock(buffer,len,XBOX::VTC_UTF_8,true,false);

	if( buffer && xmlData.InitFromXml(buffer,len, outErrorMessage, outLineNumber, false ))
	{
		xercesc::DOMDocument * document = xmlData.GetDocument();
		if(document)
		{
			xercesc::DOMNode *rootelement = (xercesc::DOMNode*) document->getDocumentElement();
			err = VXMLJsonUtility::XMLNodeToJson((VXMLDOMNodeRef)rootelement, outJson);
		}
	}

	free(buffer);

	return err;
}

VError VXMLJsonUtility::JsonToXHTML(const VString& inJson, VString& outXML, bool inWithCR )
{
	return VXMLJsonUtility::_JsonToXML(inJson, outXML, true, inWithCR);
}

VError VXMLJsonUtility::JsonToXML(const VString& inJson, VString& outXML)
{
	return VXMLJsonUtility::_JsonToXML(inJson, outXML);
}

VError VXMLJsonUtility::_JsonToXML(const VString& inJson, VString& outXML, bool inToXHTML, bool inWithCR)
{
	VError err = VE_OK;
	PaserVString parser(&inJson);

	VString jasonStart;

	parser.GetUntilChar('{');
	parser.GetUntilChar('"');
	jasonStart = parser.GetUntilChar('"');
	parser.GetUntilChar(':');
	
	if(jasonStart == DOM_ROOT)
	{
		VString type;
		type = DOM_ROOT;
		JsonObject jsonObj(type, inToXHTML);
		while(!parser.EndParsing())
		{
			UniChar thechar = parser.GetNextChar();
			if(thechar == '[')
				ParseArray(jsonObj,parser, inToXHTML);
			else if(thechar == '{')
				ParseObject(jsonObj,parser, inToXHTML);
		}

		err = jsonObj.GetXml(outXML, inWithCR);
	}
	else
		err = -1;//not valid wakanda jason

	return err;
}

VError VXMLJsonUtility::XMLNodeToJson(VXMLDOMNodeRef inNode, VString& outJson )
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

	xercesc::DOMDocument* doument= node->getOwnerDocument();
	xercesc::DOMDocumentType* doctype = doument->getDoctype();

	outJson="{\"";
	outJson+=DOM_ROOT;
	outJson+="\":[";

	bool ok = Parse( doument/*inNode*/, outJson);
	
	outJson+="}";
	err = errContext.GetLastError();
	if (err == VE_OK && (!ok))
		err = XBOX::VE_XML_ParsingError;
	return err;
}

END_TOOLBOX_NAMESPACE

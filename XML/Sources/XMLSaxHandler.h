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
#ifndef __XMLSaxHandler__
#define __XMLSaxHandler__

#include "IXMLHandler.h"

// Interface 4D Parsers et Xerces 
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/parsers/SAXParser.hpp>

BEGIN_TOOLBOX_NAMESPACE

class VXMLParser;

class XTOOLBOX_API SAX_Handlers : public xercesc::HandlerBase, private xercesc::XMLFormatTarget
{
public:
	//SAX_Handlers(const   char* const encodingName, const XMLFormatter::UnRepFlags unRepFlags);
    SAX_Handlers( xercesc::SAXParser *inParser, VXMLParser *inUserParser);
    SAX_Handlers( const SAX_Handlers *inParent, VXMLParser *inUserParser);
    ~SAX_Handlers();

			void						SetParent(SAX_Handlers* ParentHandler);
			void						SetChild(SAX_Handlers* ChildHandler) {fChildHandler = ChildHandler;};
			void						DeleteChilds() {if (fChildHandler) fChildHandler->DeleteChilds();delete fChildHandler;};

    virtual	void						startElement( const XMLCh* const name, xercesc::AttributeList& attributes);
    virtual	void						endElement( const XMLCh* const name);
    #ifdef XERCES_3_0_1
 	virtual	void						characters( const XMLCh* const chars, const XMLSize_t length);
	virtual	void						writeChars( const XMLByte* const toWrite, const XMLSize_t count, xercesc::XMLFormatter* const formatter);
	#else
	virtual	void						characters( const XMLCh* const chars, const unsigned int length);
	virtual	void						writeChars( const XMLByte* const toWrite, const unsigned int count, xercesc::XMLFormatter* const formatter);
	#endif

	// Entity Resolver
	virtual	xercesc::InputSource*		resolveEntity( const XMLCh* const inPublicId, const XMLCh* const inSystemId);

	/**
	* Receive notification of a processing instruction (like xml-stylesheet tag)
	*/
    virtual void						processingInstruction(const   XMLCh* const    target, const XMLCh* const    data);

  // -----------------------------------------------------------------------
    //  Implementations of the database ErrorHandler interface
    // -----------------------------------------------------------------------
			void						warning(const xercesc::SAXParseException& exception);
			void						error(const xercesc::SAXParseException& exception);
			void						fatalError(const xercesc::SAXParseException& exception);

			void						SetUserHandler( IXMLHandler *inHandler);
			xercesc::SAXParser*			GetParser() const	{ return fParser; }
			
			VXMLParser*					GetUserParser() const;
			
private :
			void						_ThrowError( VError inErrCode, const xercesc::SAXParseException& e);
			sLONG						GetLevel() const		{ return fLevel; }
			xercesc::XMLFormatter*		GetFormatter() const	{ return fFormatter; }

			sLONG						fLevel;
			xercesc::SAXParser*			fParser;
			xercesc::XMLFormatter*		fFormatter;
			SAX_Handlers*				fChildHandler;
			SAX_Handlers*				fParentHandler;
			IXMLHandler*				fUserHandler;
			VXMLParser*					fUserParser;

};

END_TOOLBOX_NAMESPACE

#endif

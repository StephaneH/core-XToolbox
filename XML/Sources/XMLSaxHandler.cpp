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

#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/URLInputSource.hpp>

#include "XMLSaxHandler.h"
#include "XMLSaxParser.h"

#include "IXMLHandler.h"

BEGIN_TOOLBOX_NAMESPACE


XERCES_CPP_NAMESPACE_USE


// ---------------------------------------------------------------------------
//  Local const data
//
//  Note: This is the 'safe' way to do these strings. If you compiler supports
//        L"" style strings, and portability is not a concern, you can use
//        those types constants directly.
// ---------------------------------------------------------------------------
static const XMLCh  gEndElement[] = { chOpenAngle, chForwardSlash, chNull };
static const XMLCh  gEndPI[] = { chQuestion, chCloseAngle, chNull };
static const XMLCh  gStartPI[] = { chOpenAngle, chQuestion, chNull };
static const XMLCh  gXMLDecl1[] =
{
        chOpenAngle, chQuestion, chLatin_x, chLatin_m, chLatin_l
    ,   chSpace, chLatin_v, chLatin_e, chLatin_r, chLatin_s, chLatin_i
    ,   chLatin_o, chLatin_n, chEqual, chDoubleQuote, chDigit_1, chPeriod
    ,   chDigit_0, chDoubleQuote, chSpace, chLatin_e, chLatin_n, chLatin_c
    ,   chLatin_o, chLatin_d, chLatin_i, chLatin_n, chLatin_g, chEqual
    ,   chDoubleQuote, chNull
};

static const XMLCh  gXMLDecl2[] =
{
        chDoubleQuote, chQuestion, chCloseAngle
    ,   chCR, chLF, chNull
};


// ------------------------------ SAX_Handlers ------------------------------


SAX_Handlers::SAX_Handlers( const SAX_Handlers *inParent, VXMLParser *inUserParser)
{
	assert( inParent != NULL);
	fParser = inParent->GetParser();
	fUserParser = inUserParser;
	fFormatter = inParent->GetFormatter();
	fChildHandler = NULL;
	fParentHandler = const_cast<SAX_Handlers*>(inParent);
	fUserHandler = NULL;
	fLevel = inParent->GetLevel() + 1;
}


SAX_Handlers::SAX_Handlers( SAXParser *inParser, VXMLParser *inUserParser)
{
	assert( inParser != NULL);
	fParser = inParser;
	fUserParser = inUserParser;
	fFormatter = NULL;
	fChildHandler = NULL;
	fParentHandler = NULL;
	fUserHandler = NULL;
	fLevel = 0;
}


SAX_Handlers::~SAX_Handlers()
{
	if (fChildHandler)
		delete fChildHandler;
	ReleaseRefCountable( &fUserHandler);
}




#ifdef XERCES_3_0_1
void SAX_Handlers::writeChars( const XMLByte* const toWrite, const XMLSize_t count, xercesc::XMLFormatter* const formatter)
#else
void SAX_Handlers::writeChars( const XMLByte* const toWrite, const unsigned int count, xercesc::XMLFormatter* const formatter)
#endif
{

}

// ---------------------------------------------------------------------------
//  SAX_Handlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------

void SAX_Handlers::_ThrowError( VError inErrCode, const xercesc::SAXParseException& e)
{
	StThrowError<> errThrow( VE_XML_ActionError, inErrCode);

	const VString systemID( const_cast<XMLCh*>(e.getSystemId()), (VSize) -1);
	const VString publicID( const_cast<XMLCh*>(e.getPublicId()), (VSize) -1);
	const VString message( const_cast<XMLCh*>(e.getMessage()), (VSize) -1);

	errThrow->SetString( "systemID", systemID);
	errThrow->SetString( "publicID", publicID);
	errThrow->SetString( "message", message);
	
	#if VERSIONDEBUG && VERSIONMAC
	char mini_buffer[512];
	message.ToCString(mini_buffer,512);
	printf("XML SAX Error message: %s\n",mini_buffer);
	#endif
	
	errThrow->SetLong( "line", e.getLineNumber());
	errThrow->SetLong( "column", e.getColumnNumber());
}


void SAX_Handlers::error(const SAXParseException& e)
{
	_ThrowError( VE_XML_ParsingError, e);
}


void SAX_Handlers::fatalError(const SAXParseException& e)
{
	_ThrowError( VE_XML_ParsingFatalError, e);
}


void SAX_Handlers::warning(const SAXParseException& e)
{
	char *systemID = XMLString::transcode(e.getSystemId());
	char *publicID = XMLString::transcode(e.getPublicId());
	char *msg = XMLString::transcode(e.getMessage());
	
	DebugMsg( "Xerces warning in: %s (%s), line: %d, column: %d\n  msg: %s\n", systemID, publicID, e.getLineNumber(), e.getColumnNumber(), msg);

	delete [] systemID;
	delete [] publicID;
	delete [] msg;
}

xercesc::InputSource* SAX_Handlers::resolveEntity( const XMLCh* const inPublicId, const XMLCh* const inSystemId)
{
	xercesc::InputSource *source = NULL;
	if (fUserHandler != NULL)
	{
		const VString vSystemID( const_cast<XMLCh*>(inSystemId), (VSize) -1);
		
		XBOX::VFile *file = fUserHandler->ResolveEntity( vSystemID);

		if ( (file == NULL) && (fUserParser != NULL) )
			file = fUserParser->ResolveEntity( vSystemID);

		if (file != NULL)
		{
			XBOX::VString full_path;

			#if VERSIONMAC
			file->GetPath(full_path,FPS_POSIX);
			#else
			file->GetPath(full_path);
			#endif
			
			source = new xercesc::LocalFileInputSource(full_path.GetCPointer());

			file->Release();
		}
	}
	return source;
}



/**
* Receive notification of a processing instruction (like xml-stylesheet tag)
*/
void SAX_Handlers::processingInstruction(const   XMLCh* const    target, const XMLCh* const    data)
{
	if (fUserHandler == NULL)
		return;
	if ((!target) || (!data))
		return;

	xbox_assert(sizeof(XMLCh) == sizeof(UniChar)); //just ensure Xerces uses UTF_16
	VString targetName( const_cast<XMLCh*>(target), (VSize) -1);
	VString dataName( const_cast<XMLCh*>(data), (VSize) -1);
	fUserHandler->processingInstruction( targetName, dataName);
}


void SAX_Handlers::SetParent(SAX_Handlers* ParentHandler)
{
	fParentHandler	= ParentHandler;
	fParser			= ParentHandler->GetParser();
	fFormatter		= ParentHandler->GetFormatter();
	fLevel			= ParentHandler->GetLevel() + 1;	
}


void SAX_Handlers::SetUserHandler( IXMLHandler *inHandler)
{
	CopyRefCountable( &fUserHandler, inHandler);
}


#ifdef XERCES_3_0_1
void SAX_Handlers::characters(const XMLCh* const chars, const XMLSize_t length)
#else
void SAX_Handlers::characters(const XMLCh* const chars, const unsigned int length)
#endif
{
	if (fUserHandler != NULL)
	{
		VStr255 text;
		text.FromBlock( chars, length*sizeof(XMLCh), VTC_UTF_16);
		fUserHandler->SetText( text);
	}
}


void SAX_Handlers::startElement( const XMLCh* const name, AttributeList&  attributes)
{
	// reuse previous element handler if any
	if 	(fChildHandler == NULL)
		fChildHandler = new SAX_Handlers( this, fUserParser);
	
	fParser->setDocumentHandler( fChildHandler);

 	if (fUserHandler != NULL && fChildHandler != NULL)
	{
		const VString vElementName( const_cast<XMLCh*>(name), (VSize) -1);
		IXMLHandler *userHandler = fUserHandler->StartElement( vElementName);
		fChildHandler->SetUserHandler( userHandler);

		if (userHandler != NULL)
		{
			int len = attributes.getLength();
			for( int index = 0; index < len; index++)
			{
				const VString vAttributeName( const_cast<XMLCh*>(attributes.getName(index)), (VSize) -1);
				const VString vAttributeValue( const_cast<XMLCh*>(attributes.getValue(index)), (VSize) -1);
	
				userHandler->SetAttribute( vAttributeName, vAttributeValue);
			}
			userHandler->Release();
		}
	}
}


void SAX_Handlers::endElement(const XMLCh* const name)
{
	if (fParentHandler)
		fParser->setDocumentHandler(fParentHandler);
	if (fUserHandler != NULL)
	{
		const VString vElementName( const_cast<XMLCh*>(name), (VSize) -1);
		fUserHandler->EndElement( vElementName);
	}
}



END_TOOLBOX_NAMESPACE

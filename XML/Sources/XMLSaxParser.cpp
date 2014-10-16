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
#include <xercesc/framework/MemBufInputSource.hpp>

#include "XMLSaxParser.h"
#include "XMLSaxHandler.h"


BEGIN_TOOLBOX_NAMESPACE



static bool SAXParse( VXMLParser *inUserParser, const xercesc::InputSource& inSource, IXMLHandler *inHandler, XMLParsingOptions inOptions)
{
	//
    //  Create a SAX parser object. Then, set it to validate or not.
    //
	xercesc::SAXParser parser;

	if (inOptions & XML_ValidateAlways)
	    parser.setValidationScheme( xercesc::SAXParser::Val_Always);
	else if (inOptions & XML_ValidateNever)
	    parser.setValidationScheme( xercesc::SAXParser::Val_Never);
    else
	    parser.setValidationScheme( xercesc::SAXParser::Val_Auto);

	parser.setLoadExternalDTD( (inOptions & XML_LoadExternalDTD) != 0);
    parser.setDoNamespaces( (inOptions & XML_DoNameSpaces) != 0);

	SAX_Handlers sax_handler( &parser, inUserParser);
	sax_handler.SetUserHandler( inHandler);

	parser.setDocumentHandler( static_cast<xercesc::DocumentHandler *>(&sax_handler));
	parser.setErrorHandler( static_cast<xercesc::ErrorHandler *>(&sax_handler));
	parser.setEntityResolver( static_cast<xercesc::EntityResolver *>(&sax_handler));
	parser.parse( inSource);
	
	return parser.getErrorCount() == 0;
}


//====================================================================================================



// Components custom implementation


bool VXMLParser::Parse( VFile *inFile, IXMLHandler *inHandler, XMLParsingOptions inOptions)
{
	XBOX::VString full_path;

	#if VERSIONMAC || VERSION_LINUX
	inFile->GetPath(full_path,FPS_POSIX);
	#else
	inFile->GetPath(full_path);
	#endif
	
	xercesc::LocalFileInputSource source(full_path.GetCPointer());

	return SAXParse( this, source, inHandler, inOptions);
}


bool VXMLParser::Parse( const void *inBuffer, VSize inBufferSize, IXMLHandler *inHandler, XMLParsingOptions inOptions)
{
	xercesc::MemBufInputSource source( (const XMLByte *) inBuffer, (unsigned int) inBufferSize, "frommem");

	return SAXParse( this, source, inHandler, inOptions);
}


bool VXMLParser::Parse( const VString& inText, IXMLHandler *inHandler, XMLParsingOptions inOptions)
{
	xercesc::MemBufInputSource source( (const XMLByte *) inText.GetCPointer(), inText.GetLength() * sizeof( UniChar), "frommem");
	#if BIGENDIAN
	source.setEncoding( xercesc::XMLUni::fgUTF16BEncodingString);
	#else
	source.setEncoding( xercesc::XMLUni::fgUTF16LEncodingString);
	#endif

	return SAXParse( this, source, inHandler, inOptions);
}




void VXMLParser::SetEntitiesLocalFolder( const VString& inSystemIDBase, VFolder *inFolder)
{
	CopyRefCountable( &fEntityLocalFolder, inFolder);
	fEntityBase = inSystemIDBase;
}


XBOX::VFile *VXMLParser::ResolveEntity( const VString& inSystemID)
{
	return ResolveEntityFromFolder( fEntityBase, fEntityLocalFolder, inSystemID);
}


XBOX::VFile *VXMLParser::ResolveEntityFromFolder( const VString& inEntityBase, VFolder *inEntityLocalFolder, const VString& inSystemID)
{
	VFile *file = NULL;
	if (inEntityLocalFolder != NULL)
	{
		if (inSystemID.BeginsWith( inEntityBase))
		{
			VString relativePath = inSystemID;
			relativePath.Exchange( inEntityBase, CVSTR( ""));

			// convert into native path to please VFile
			for( UniChar *p = relativePath.GetCPointerForWrite() ; *p ; ++p)
			{
				if (*p == '/')
					*p = FOLDER_SEPARATOR;
			}
			
			VFilePath path( inEntityLocalFolder->GetPath(), relativePath);
			if (path.IsFile())
				file = new VFile( path);
		}
	}
	return file;
}


void VXMLParser::DeInit()
{
	xercesc::XMLPlatformUtils::Terminate();
}


bool VXMLParser::Init()
{
	xercesc::XMLPlatformUtils::Initialize();
	return true;
}


END_TOOLBOX_NAMESPACE

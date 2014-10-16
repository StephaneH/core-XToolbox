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
#ifndef __XMLSaxParser__
#define __XMLSaxParser__

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class IXMLHandler;



typedef uLONG XMLParsingOptions;	// options for VXMLParser::Parse
enum {
	XML_ValidateAlways		= 1,	// Needs a DTD
	XML_ValidateNever		= 2,	// Ignore the DTD
	XML_DoNameSpaces		= 4,	// Enforce namespaces validation rules
	XML_LoadExternalDTD		= 8,	// Allow loading of external DTD
	XML_Default				= 0		// validate if there's a dtd mentionned. No external DTD loading, no namespace
};


class XTOOLBOX_API VXMLParser : public VObject
{
public:
							VXMLParser():fEntityLocalFolder( NULL)	{;}
							~VXMLParser()	{ ReleaseRefCountable( &fEntityLocalFolder);}

	static	bool			Init();
	static	void			DeInit();
	
			bool			Parse( VFile *inFile, IXMLHandler *inHandler, XMLParsingOptions inOptions);
			bool			Parse( const void *inBuffer, VSize inBufferSize, IXMLHandler *inHandler, XMLParsingOptions inOptions);
			bool			Parse( const VString& inText, IXMLHandler *inHandler, XMLParsingOptions inOptions);
			

			void			SetEntitiesLocalFolder( const VString& inSystemIDBase, VFolder *inFolder);
			VFile*			ResolveEntity( const VString& inSystemID);

	static	VFile*			ResolveEntityFromFolder( const VString& inEntityBase, VFolder *inEntityLocalFolder, const VString& inStystemID);

private:
			VString			fEntityBase;			// base url for locally cached entities
			VFolder*		fEntityLocalFolder;		// base folder for locally cached entities
};


END_TOOLBOX_NAMESPACE


#endif


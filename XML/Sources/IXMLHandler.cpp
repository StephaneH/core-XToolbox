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

#include "IXMLHandler.h"
#include "XMLSaxParser.h"

BEGIN_TOOLBOX_NAMESPACE


IXMLHandler::~IXMLHandler()
{
	ReleaseRefCountable( &fEntityLocalFolder);
}


IXMLHandler	*IXMLHandler::StartElement( const VString& inElementName)
{
	return NULL;
}


void IXMLHandler::EndElement(const XBOX::VString&)
{
}


void IXMLHandler::SetAttribute( const VString& inName, const VString& inValue)
{
}


void IXMLHandler::SetText( const VString& inText)
{
}


bool  IXMLHandler::WCharIsSpace( UniChar c )
{
  if (c == 32)
	  return true;
  return ( c < 32 ) &&
        ( ( ( ( ( 1L << '\t' ) |
        ( 1L << '\n' ) |
        ( 1L << '\r' ) |
        ( 1L << '\f' ) ) >> c ) & 1L ) != 0 );
}

/**
* Receive notification of a processing instruction (like xml-stylesheet tag)
*
* @param inTarget The processing instruction target. (for instance 'xml-stylesheet')
* @param inData The processing instruction data, or empty if 
*             none is supplied. (for instance 'href="mystyle.css" type="text/css"')
* @remarks
*	if tag is 'xml-stylesheet', 
*	internally call user handler 
*		IXMLHandler::StartStyleSheet, 
*		IXMLHandler::SetStyleSheetAttribute and 
*		IXMLHandler::EndStyleSheet
*/
void IXMLHandler::processingInstruction(const VString& inTarget, const VString& inData)
{
	if (inTarget.IsEmpty() || inData.IsEmpty())
		return;

	//check tag validity
	if (!inTarget.EqualToString( "xml-stylesheet", true))
		return;

	//call start user handler
	StartStyleSheet();

	//parse stylesheet attributes
	VString url;
	VString type;
	const UniChar *c = inData.GetCPointer();
	VString s;
	VString name;
	bool bParseValue = false;
	while (*c != 0)
	{
		while (WCharIsSpace(*c)) 
		{
			if (!s.IsEmpty())
			{
				if (s.BeginsWith("\"") && s.EndsWith("\""))
					s.SubString(2,s.GetLength()-2);
				else if (s.BeginsWith("'") && s.EndsWith("'"))
					s.SubString(2,s.GetLength()-2);
				s.RemoveWhiteSpaces();
				if (bParseValue)
				{
					//call setAttribute user handler
					if (!name.IsEmpty())
						SetStyleSheetAttribute( name, s);
					bParseValue = false;
				}
				else
					name = s;
				s.Clear();
			}
			c++;
		}
		if (*c == '=')
		{
			if (!s.IsEmpty())
				name = s;
			bParseValue = true;
			s.Clear();
		}
		else
			if (*c != 0)
				s.AppendUniChar(*c);
		if (*c != 0)
			c++;
	};
	if ((!s.IsEmpty()) && bParseValue)
	{
		if (s.BeginsWith("\"") && s.EndsWith("\""))
			s.SubString(2,s.GetLength()-2);
		else if (s.BeginsWith("'") && s.EndsWith("'"))
			s.SubString(2,s.GetLength()-2);
		s.RemoveWhiteSpaces();
		
		//call setAttribute user handler
		if (!name.IsEmpty())
			SetStyleSheetAttribute( name, s);
	}	

	//call end user handler
	EndStyleSheet();
}


/** start style sheet element */
void IXMLHandler::StartStyleSheet()
{
}
/** set style sheet attribute */
void IXMLHandler::SetStyleSheetAttribute( const VString& inName, const VString& inValue)
{
	DebugMsg( "unhandled stylesheet attribute, name = %S value = %S\n", &inName, &inValue);
}

/** end style sheet element */
void IXMLHandler::EndStyleSheet()
{
}


VFile *IXMLHandler::ResolveEntity( const VString& inSystemID)
{
	return VXMLParser::ResolveEntityFromFolder( fEntityBase, fEntityLocalFolder, inSystemID);
}


void IXMLHandler::SetEntitiesLocalFolder( const VString& inSystemIDBase, VFolder *inFolder)
{
	CopyRefCountable( &fEntityLocalFolder, inFolder);
	fEntityBase = inSystemIDBase;
}

/** return true if character is valid XML 1.1
(some control & special chars are not supported like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
bool IXMLHandler::IsXMLChar(UniChar inChar)
{
	return xercesc::XMLChar1_1::isXMLChar(inChar);
}

//==========================================================


IXMLHandler *VNullXMLHandler::StartElement( const VString& inElementName)
{
	Retain();
	return this;
}


void VNullXMLHandler::SetAttribute( const VString& inName, const VString& inValue)
{
}


void VNullXMLHandler::SetText( const VString& inText)
{
}


//==========================================================


END_TOOLBOX_NAMESPACE

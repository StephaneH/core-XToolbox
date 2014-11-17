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
#include "VInfoPlistTools.h"
#include "XMLSaxParser.h"
#include "XML/VXML.h"

BEGIN_TOOLBOX_NAMESPACE

class VInfoPlistSAXHandler : public VObject, public IXMLHandler
{
public:
	VInfoPlistSAXHandler(const VString& inKeyName ,VString& ioKeyValue) :
	fKeyValue(&ioKeyValue),
	fKeyName(inKeyName),
	fIsValueFound(false),
	fIsKeyFound(false),
	fGetNextText(false)
	{

	}

	~VInfoPlistSAXHandler(){}

	bool IsFound(){return fIsValueFound;}

	IXMLHandler*		StartElement(const VString& inElementName);
	void				SetText( const VString& inText);

private:
	VString*	fKeyValue;
	VString		fKeyName;
	bool		fIsValueFound;
	bool		fIsKeyFound;
	bool		fGetNextText;
};

IXMLHandler* VInfoPlistSAXHandler::StartElement(const VString& inElementName)
{
	IXMLHandler* result = this; 
	this->Retain();

	if(!fIsValueFound)
	{
		if(inElementName == "key" || inElementName == "string")
		{
			fGetNextText = true;
		}
	}
	return result;
}

void VInfoPlistSAXHandler::SetText( const VString& inText)
{
	if(fGetNextText)
	{
		if(fIsKeyFound)
		{
			*fKeyValue=inText;
			fIsValueFound = true;
		}
		else if(inText == fKeyName)
		{
			fIsKeyFound = true;	
		}

		fGetNextText = false;
	}
}

bool VInfoPlistTools::GetKeyValueAsString( const VFilePath& inInfoplistPath, const VString& inKeyName, VString& outKeyValue)
{
	bool result = false;
	VFile infoplist(inInfoplistPath);

	if(infoplist.Exists())
	{
		VString infoplistContent;	
		VError err = VE_OK;
		VFileStream stream(&infoplist);
		
		err = stream.OpenReading();
		if(err== VE_OK)
			err = stream.GuessCharSetFromLeadingBytes(VTC_UTF_8);	
		if(err== VE_OK)
			err = stream.GetText(infoplistContent);
		if(err== VE_OK)
			err = stream.CloseReading();

		if(err== VE_OK)
		{
			VInfoPlistSAXHandler vsaxhandler(inKeyName, outKeyValue);
			VXMLParser vSAXParser;
			vSAXParser.Parse(infoplistContent, &vsaxhandler, XML_ValidateNever);

			result = vsaxhandler.IsFound();
		}
	}

	return result;
}

bool VInfoPlistTools::GetKeyValueAsBool( const VFilePath& inInfoplistPath, const VString& inKeyName, bool& outKeyValue)
{
	bool result = false;

	VString stringValue;
	if(VInfoPlistTools::GetKeyValueAsString( inInfoplistPath, inKeyName, stringValue))
	{
		if(stringValue == "true")
		{
			outKeyValue = true;
			result = true;
		}
		else if(stringValue == "false")
		{
			outKeyValue = false;
			result = true;
		}
	}

	return result;
}

END_TOOLBOX_NAMESPACE

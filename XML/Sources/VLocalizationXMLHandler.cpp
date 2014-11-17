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
#include "VLocalizationXMLHandler.h"
#include "VLocalizationManager.h"
#include "XMLSaxParser.h"
#include "IXMLHandler.h"


BEGIN_TOOLBOX_NAMESPACE


//VLocalizationXMLHandler
//-----------------------
VLocalizationXMLHandler::VLocalizationXMLHandler(VLocalizationManager* localizationManager)
: fLocalizationManager(localizationManager)
, fXLIFFFileLanguageIsValid( false)
, fAvoidLanguageChecking( false)
{
}


IXMLHandler	* VLocalizationXMLHandler::StartElement(const VString& inElementName)
{
	//DebugMsg( "VLocalizationXMLHandler::Unhandled XML element, name = %S\n", &inElementName);
	if (inElementName.EqualToUSASCIICString( "file"))
		fElementName = inElementName;
	else
		fElementName.Clear();
	
	if (inElementName.EqualToUSASCIICString( "group") && (fXLIFFFileLanguageIsValid || fAvoidLanguageChecking))
		return new VLocalizationGroupHandler(fShouldOverwriteAnyExistentLocalizationValue, fLocalizationManager);

	if (inElementName.EqualToUSASCIICString( "trans-unit") && (fXLIFFFileLanguageIsValid || fAvoidLanguageChecking))
	{
		return new VLocalizationTransUnitHandler( fShouldOverwriteAnyExistentLocalizationValue, fLocalizationManager);
	}
	else
	{
		this->Retain();
		return this;
	}
}

void VLocalizationXMLHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if (fElementName.EqualToUSASCIICString( "file"))
	{
		if (inName.EqualToUSASCIICString( "target-language"))
		{
			VString localizationManagerRFC3066Code;
			if (VIntlMgr::GetRFC3066BisLanguageCodeWithDialectCode(fLocalizationManager->GetLocalizationLanguage(), localizationManagerRFC3066Code))
			{
				RFC3066CodeCompareResult rFC3066CodeCompareResult = VIntlMgr::Compare2RFC3066LanguageCodes(localizationManagerRFC3066Code, inValue);
				if (rFC3066CodeCompareResult == RFC3066_CODES_NOT_VALID || rFC3066CodeCompareResult == RFC3066_CODES_ARE_NOT_EQUAL)
				{
					fXLIFFFileLanguageIsValid = false;
				}
				else if (rFC3066CodeCompareResult == RFC3066_CODES_ARE_EQUAL)
				{
					fXLIFFFileLanguageIsValid = true;
					fShouldOverwriteAnyExistentLocalizationValue = true;
				}
				else if (rFC3066CodeCompareResult == RFC3066_CODES_LANGUAGE_REGION_VARIANT || rFC3066CodeCompareResult == RFC3066_CODES_GLOBAL_LANGUAGE_AND_SPECIFIC_LANGUAGE_REGION_VARIANT || rFC3066CodeCompareResult == RFC3066_CODES_SPECIFIC_LANGUAGE_REGION_AND_GLOBAL_LANGUAGE_VARIANT )
				{
					fXLIFFFileLanguageIsValid = true;
					fShouldOverwriteAnyExistentLocalizationValue = false;
				}
			}
		}
	}
}


void VLocalizationXMLHandler::SetText(const VString& inText)
{

}


void VLocalizationXMLHandler::SetAvoidLanguageChecking(bool inAvoidLanguageChecking)
{
	fAvoidLanguageChecking = inAvoidLanguageChecking;
}

//VLocalizationGroupHandler
//------------------------

VLocalizationGroupHandler::VLocalizationGroupHandler(bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* localizationManager)
: fLocalizationManager(localizationManager)
, fShouldOverwriteAnyExistentLocalizationValue(inShouldOverwriteAnyExistentLocalizationValue)
, fGroupBag( NULL)
, fGroupID( 0)
{
	fGroupResnamesStack.push(CVSTR(""));
}


VLocalizationGroupHandler::~VLocalizationGroupHandler()
{
	ReleaseRefCountable( &fGroupBag);
}


IXMLHandler* VLocalizationGroupHandler::StartElement( const VString& inElementName)
{
	IXMLHandler *handler = NULL;

	if (inElementName.EqualToUSASCIICString( "trans-unit"))
	{
		
		// optim: collect trans-unit attributes only for typed groups
		VValueBag *groupBag = fGroupRestype.IsEmpty() ? NULL : fGroupBag;

		handler = new VLocalizationTransUnitHandler( fGroupID, fGroupResnamesStack, groupBag, fShouldOverwriteAnyExistentLocalizationValue, fLocalizationManager);
	}
	else if (inElementName.EqualToUSASCIICString( "group"))
	{
		fGroupResnamesStack.push(CVSTR(""));
		this->Retain();
		handler = this;
	}

	return handler;
}


void VLocalizationGroupHandler::EndElement( const XBOX::VString&)
{
	if ( (fGroupBag != NULL) && !fGroupRestype.IsEmpty())
	{
		// ensure the resname attribute is set in bag
		if (!fGroupResnamesStack.top().IsEmpty())
			fGroupBag->SetString( "resname", fGroupResnamesStack.top());
		fLocalizationManager->InsertGroupBag( fGroupResnamesStack.top(), fGroupRestype, fGroupBag);
	}
	ReleaseRefCountable( &fGroupBag);
	fGroupRestype.Clear();
	fGroupResnamesStack.pop();
}

void VLocalizationGroupHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if (fGroupBag == NULL)
		fGroupBag = new VValueBag;

	if (inName.EqualToUSASCIICString( "id"))
	{
		sLONG idValue = inValue.GetLong();
		if (idValue != 0)
			fGroupID = idValue;
		//else
			//DebugMsg( "VLocalizationGroupHandler::Non-valid group id Value : %S\n", &inValue);
	}	
	else if (inName.EqualToUSASCIICString( "resname"))
	{
		if (!inValue.IsEmpty())
		{
			fGroupResnamesStack.top() = inValue;
		}
	}
	else if (inName.EqualToUSASCIICString( "restype"))
	{
		xbox_assert( fGroupRestype.IsEmpty() );	// no nested group
		fGroupRestype = inValue;
	}

	if (fGroupBag != NULL)
		fGroupBag->SetAttribute( inName, inValue);
}

void VLocalizationGroupHandler::SetText(const VString& inText)
{

}

//VLocalizationTransUnitHandler
//----------------------------
VLocalizationTransUnitHandler::VLocalizationTransUnitHandler( uLONG inGroupID, const std::stack<XBOX::VString>& inGroupResnamesStack, VValueBag *inGroupBag, bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* inLocalizationManager)
: fLocalizationManager( inLocalizationManager)
, fShouldOverwriteAnyExistentLocalizationValue( inShouldOverwriteAnyExistentLocalizationValue)
, fStringID( 0)
, fIsEntryRecorded( false)
, fGroupID( inGroupID)
, fGroupResnamesStack( inGroupResnamesStack)
, fGroupBag( RetainRefCountable( inGroupBag))
, fTransUnitBag( NULL)
, fExcluded( false)
{
	fTarget.SetNull();
	fSource.SetNull();
	
	if (fGroupBag != NULL)
		fTransUnitBag = new VValueBag;
}


VLocalizationTransUnitHandler::VLocalizationTransUnitHandler( bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* inLocalizationManager)
: fLocalizationManager( inLocalizationManager)
, fShouldOverwriteAnyExistentLocalizationValue( inShouldOverwriteAnyExistentLocalizationValue)
, fStringID( 0)
, fIsEntryRecorded( false)
, fGroupBag( NULL)
, fTransUnitBag( NULL)
, fExcluded( false)
{
	fTarget.SetNull();
	fSource.SetNull();
}


VLocalizationTransUnitHandler::~VLocalizationTransUnitHandler()
{
	ReleaseRefCountable( &fTransUnitBag);
	ReleaseRefCountable( &fGroupBag);
}


IXMLHandler* VLocalizationTransUnitHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "VLocalizationTransUnitHandler::Unhandled XML element, name = %S\n", &inElementName);
	fElementName = inElementName;
	if (inElementName.EqualToUSASCIICString( "source") || inElementName.EqualToUSASCIICString( "target"))
	{
		this->Retain();
		return this;
	}
	else if (inElementName.EqualToUSASCIICString( "alt-trans"))
	{
		
		return new VLocalizationAltTransHandler( fGroupID, fGroupResnamesStack, fTransUnitBag, fShouldOverwriteAnyExistentLocalizationValue, fLocalizationManager);
	}
	else
	{
		return NULL;
	}
}

void VLocalizationTransUnitHandler::EndElement( const XBOX::VString&)
{
	//DebugMsg( "VLocalizationTransUnitHandler::EndElement: %S\n", &fElementName);
	if (fElementName.EqualToUSASCIICString( "source"))
	{
		fElementName.Clear();
	}
	else if (fElementName.EqualToUSASCIICString( "target"))
	{
		fElementName.Clear();
	}
	//The last _should_ be the trans-unit element.
	else
	{
		fElementName.Clear();
		
		if (!fExcluded)
		{
			VString translatedString;
			if (!fTarget.IsNull())
				translatedString = fTarget;
			else if (!fSource.IsNull())
				translatedString = fSource;

			if (fTransUnitBag != NULL)
			{
				fTransUnitBag->SetString( "target", translatedString);
				fGroupBag->AddElement( "trans-unit", fTransUnitBag);
			}
			
			//As soon as the conditions are fulfilled, we record it in the Localization Manager and we won't record it another time. It implies that if the trans-unit has two <target> tags for example, the first <target> tag is the one we take in account.
			if (!fIsEntryRecorded)
			{
				// Value Insertion (STR#-like pattern)
				if (fStringID > 0 && fGroupID != 0)
				{			
					STRSharpCodes theSTRSharpCodes = STRSharpCodes(fGroupID, fStringID);
					fIsEntryRecorded = fLocalizationManager->InsertSTRSharpCodeAndString(theSTRSharpCodes, translatedString, fShouldOverwriteAnyExistentLocalizationValue);
				}

				// Value Insertion (resname value)
				if (!translatedString.IsEmpty())
				{
					VString resourceURL;
					if (fGroupResnamesStack.size() >= 2 && !fResName.IsEmpty())
					{
						std::stack<XBOX::VString> groupResnamesStackCopy = fGroupResnamesStack;
						VString lastGroupResname = groupResnamesStackCopy.top();
						groupResnamesStackCopy.pop();
						VString penultimateGroupResname = groupResnamesStackCopy.top();
						if (!lastGroupResname.IsEmpty() && !penultimateGroupResname.IsEmpty() && penultimateGroupResname[0] == CHAR_LEFT_SQUARE_BRACKET && penultimateGroupResname[penultimateGroupResname.GetLength()-1] == CHAR_RIGHT_SQUARE_BRACKET)
						{
							VString divider = VString(OO_SYNTAX_INTERNAL_DIVIDER);
							resourceURL += penultimateGroupResname;
							resourceURL += divider;
							resourceURL += lastGroupResname;
							resourceURL += divider;
							resourceURL += fResName;
						}
					}
					
					if (!resourceURL.IsEmpty())
					{
						if (fLocalizationManager->InsertObjectURLAndString(resourceURL, translatedString, fShouldOverwriteAnyExistentLocalizationValue))
							fIsEntryRecorded = true;
					}
					else
					{
						if (!fResName.IsEmpty())
						{
							if (fLocalizationManager->InsertObjectURLAndString(fResName, translatedString, fShouldOverwriteAnyExistentLocalizationValue))
								fIsEntryRecorded = true;
						}
						if (fGroupResnamesStack.size() >= 1)
						{
							std::stack<XBOX::VString> groupResnamesStackCopy = fGroupResnamesStack;
							while(groupResnamesStackCopy.size() > 0)
							{
								VString groupResname;
								groupResname = groupResnamesStackCopy.top();
								groupResnamesStackCopy.pop();
								if (fLocalizationManager->InsertIDAndStringInAGroup(fStringID, translatedString, groupResname, fShouldOverwriteAnyExistentLocalizationValue))
									fIsEntryRecorded = true;
							}
						}
					}
				}
			}
		}
	}
}

void VLocalizationTransUnitHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	if (fTransUnitBag != NULL)
	{
		fTransUnitBag->SetString( inName, inValue);
	}
	
	if (inName.EqualToUSASCIICString( "id"))
	{
		sLONG idValue = inValue.GetLong();
		if (idValue > 0)
			fStringID = (uLONG)idValue;
		//else
			//DebugMsg("VLocalizationTransUnitHandler::Non-valid trans-unit id Value : %S\n", &inValue);
	}
	else if (inName.EqualToUSASCIICString( "resname"))
	{
		fResName = inValue;
	}
	else if (inName.EqualToUSASCIICString( "d4:excludeIf"))
	{
		CheckPlatformTag( inValue, false, &fExcluded);
	}
	else if (inName.EqualToUSASCIICString( "d4:includeIf"))
	{
		CheckPlatformTag( inValue, true, &fExcluded);
	}
}


void VLocalizationTransUnitHandler::CheckPlatformTag( const VString& inTags, bool inIncludeIfTag, bool *ioExcluded)
{
	VectorOfVString tags;
	inTags.GetSubStrings( ' ', tags, false /*inReturnEmptyStrings*/, true /*inTrimSpaces*/);

	// same tags as in VContextualMenu::_AddDefaultTags
	bool mac = std::find( tags.begin(), tags.end(), CVSTR("mac")) != tags.end();
	bool win = std::find( tags.begin(), tags.end(), CVSTR("win")) != tags.end();
	bool lin = std::find( tags.begin(), tags.end(), CVSTR("linux")) != tags.end();

	if (mac || win || lin)
	{
		bool ok;
#if VERSIONMAC
		ok = mac;
#elif VERSIONWIN
		ok = win;
#elif VERSION_LINUX
		ok = lin;
#endif
		if (inIncludeIfTag && !ok)
			*ioExcluded = true;
		else if (!inIncludeIfTag && ok)
			*ioExcluded = true;
	}
}


void VLocalizationTransUnitHandler::SetText( const VString& inText)
{
	if (fElementName.EqualToUSASCIICString( "source"))
		fSource += inText;
	else if (fElementName.EqualToUSASCIICString( "target"))
		fTarget += inText;
}


//VLocalizationAltTransHandler
//----------------------------
VLocalizationAltTransHandler::VLocalizationAltTransHandler( uLONG inGroupID, const std::stack<XBOX::VString>& inGroupResnamesStack, VValueBag *inTransUnitBag, bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* inLocalizationManager)
//: fLocalizationManager( inLocalizationManager)
: fStringID( 0)
//, fIsEntryRecorded( false)
//, fGroupID( inGroupID)
, fGroupResnamesStack( inGroupResnamesStack)
, fTransUnitBag( RetainRefCountable( inTransUnitBag))
, fAltTransBag( NULL)
//, fShouldOverwriteAnyExistentLocalizationValue( inShouldOverwriteAnyExistentLocalizationValue)
, fExcluded( false)
{
	fTarget.SetNull();
	fSource.SetNull();

	if (fTransUnitBag != NULL)
		fAltTransBag = new VValueBag;
}


VLocalizationAltTransHandler::VLocalizationAltTransHandler( bool inShouldOverwriteAnyExistentLocalizationValue, VLocalizationManager* inLocalizationManager)
//: fLocalizationManager( inLocalizationManager)
: fStringID( 0)
//, fIsEntryRecorded( false)
, fTransUnitBag( NULL)
, fAltTransBag( NULL)
//, fShouldOverwriteAnyExistentLocalizationValue( inShouldOverwriteAnyExistentLocalizationValue)
, fExcluded( false)
{
	fTarget.SetNull();
	fSource.SetNull();
}


VLocalizationAltTransHandler::~VLocalizationAltTransHandler()
{
	ReleaseRefCountable( &fAltTransBag);
	ReleaseRefCountable( &fTransUnitBag);
}


IXMLHandler* VLocalizationAltTransHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "VLocalizationAltTransHandler::Unhandled XML element, name = %S\n", &inElementName);
	fElementName = inElementName;
	if (inElementName.EqualToUSASCIICString( "source") || inElementName.EqualToUSASCIICString( "target"))
	{
		this->Retain();
		return this;
	}
	else
	{
		return NULL;
	}
}

void VLocalizationAltTransHandler::EndElement( const XBOX::VString&)
{
	//DebugMsg( "VLocalizationAltTransHandler::EndElement: %S\n", &fElementName);
	if (fElementName.EqualToUSASCIICString( "source"))
	{
		fElementName.Clear();
	}
	else if (fElementName.EqualToUSASCIICString( "target"))
	{
		fElementName.Clear();
	}
	//The last _should_ be the trans-unit element.
	else
	{
		fElementName.Clear();

		if (!fExcluded)
		{
			VString translatedString;
			if (!fTarget.IsNull())
				translatedString = fTarget;
			else if (!fSource.IsNull())
				translatedString = fSource;

			if (fAltTransBag != NULL)
			{
				fAltTransBag->SetString( "target", translatedString);
				fTransUnitBag->AddElement( "alt-trans", fAltTransBag);
			}
			
			//As soon as the conditions are fulfilled, we record it in the Localization Manager and we won't record it another time. It implies that if the trans-unit has two <target> tags for example, the first <target> tag is the one we take in account.
			//if (!fIsEntryRecorded)
			//{
			//	// Value Insertion (STR#-like pattern)
			//	if (fStringID > 0 && fGroupID != 0)
			//	{			
			//		STRSharpCodes theSTRSharpCodes = STRSharpCodes(fGroupID, fStringID);
			//		fIsEntryRecorded = fLocalizationManager->InsertSTRSharpCodeAndString(theSTRSharpCodes, translatedString, fShouldOverwriteAnyExistentLocalizationValue);
			//	}

			//	// Value Insertion (resname value)
			//	if (!translatedString.IsEmpty())
			//	{
			//		VString resourceURL;
			//		if (fGroupResnamesStack.size() >= 2 && !fResName.IsEmpty())
			//		{
			//			std::stack<XBOX::VString> groupResnamesStackCopy = fGroupResnamesStack;
			//			VString lastGroupResname = groupResnamesStackCopy.top();
			//			groupResnamesStackCopy.pop();
			//			VString penultimateGroupResname = groupResnamesStackCopy.top();
			//			if (!lastGroupResname.IsEmpty() && !penultimateGroupResname.IsEmpty() && penultimateGroupResname[0] == CHAR_LEFT_SQUARE_BRACKET && penultimateGroupResname[penultimateGroupResname.GetLength()-1] == CHAR_RIGHT_SQUARE_BRACKET)
			//			{
			//				VString divider = VString(OO_SYNTAX_INTERNAL_DIVIDER);
			//				resourceURL += penultimateGroupResname;
			//				resourceURL += divider;
			//				resourceURL += lastGroupResname;
			//				resourceURL += divider;
			//				resourceURL += fResName;
			//			}
			//		}
			//		
			//		if (!resourceURL.IsEmpty())
			//		{
			//			if (fLocalizationManager->InsertObjectURLAndString(resourceURL, translatedString, fShouldOverwriteAnyExistentLocalizationValue))
			//				fIsEntryRecorded = true;
			//		}
			//		else
			//		{
			//			if (!fResName.IsEmpty())
			//			{
			//				if (fLocalizationManager->InsertObjectURLAndString(fResName, translatedString, fShouldOverwriteAnyExistentLocalizationValue))
			//					fIsEntryRecorded = true;
			//			}
			//			if (fGroupResnamesStack.size() >= 1)
			//			{
			//				std::stack<XBOX::VString> groupResnamesStackCopy = fGroupResnamesStack;
			//				while(groupResnamesStackCopy.size() > 0)
			//				{
			//					VString groupResname;
			//					groupResname = groupResnamesStackCopy.top();
			//					groupResnamesStackCopy.pop();
			//					if (fLocalizationManager->InsertIDAndStringInAGroup(fStringID, translatedString, groupResname, fShouldOverwriteAnyExistentLocalizationValue))
			//						fIsEntryRecorded = true;
			//				}
			//			}
			//		}
			//	}
			//}
		}
	}
}

void VLocalizationAltTransHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	if (fAltTransBag != NULL)
	{
		fAltTransBag->SetString( inName, inValue);
	}
	
	if (inName.EqualToUSASCIICString( "id"))
	{
		sLONG idValue = inValue.GetLong();
		if (idValue > 0)
			fStringID = (uLONG)idValue;
		//else
			//DebugMsg("VLocalizationAltTransHandler::Non-valid trans-unit id Value : %S\n", &inValue);
	}
	else if (inName.EqualToUSASCIICString( "resname"))
	{
		fResName = inValue;
	}
	else if (inName.EqualToUSASCIICString( "d4:excludeIf"))
	{
		CheckPlatformTag( inValue, false, &fExcluded);
	}
	else if (inName.EqualToUSASCIICString( "d4:includeIf"))
	{
		CheckPlatformTag( inValue, true, &fExcluded);
	}
}


void VLocalizationAltTransHandler::CheckPlatformTag( const VString& inTags, bool inIncludeIfTag, bool *ioExcluded)
{
	VectorOfVString tags;
	inTags.GetSubStrings( ' ', tags, false /*inReturnEmptyStrings*/, true /*inTrimSpaces*/);

	// same tags as in VContextualMenu::_AddDefaultTags
	bool mac = std::find( tags.begin(), tags.end(), CVSTR("mac")) != tags.end();
	bool win = std::find( tags.begin(), tags.end(), CVSTR("win")) != tags.end();
	bool lin = std::find( tags.begin(), tags.end(), CVSTR("linux")) != tags.end();

	if (mac || win || lin)
	{
		bool ok;
#if VERSIONMAC
		ok = mac;
#elif VERSIONWIN
		ok = win;
#elif VERSION_LINUX
		ok = lin;
#endif
		if (inIncludeIfTag && !ok)
			*ioExcluded = true;
		else if (!inIncludeIfTag && ok)
			*ioExcluded = true;
	}
}


void VLocalizationAltTransHandler::SetText( const VString& inText)
{
	if (fElementName.EqualToUSASCIICString( "source"))
		fSource += inText;
	else if (fElementName.EqualToUSASCIICString( "target"))
		fTarget += inText;
}

END_TOOLBOX_NAMESPACE

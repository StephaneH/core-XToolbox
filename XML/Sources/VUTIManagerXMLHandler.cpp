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
#include "VUTIManagerXMLHandler.h"

#include "4DUTType.h"

BEGIN_TOOLBOX_NAMESPACE

const VString kKeyTagName("key");
const VString kArrayTagName("array");
const VString kDictTagName("dict");
const VString kStringTagName("string");

VUTIManagerXMLHandler::VUTIManagerXMLHandler(VUTIManager* inUTIManager):fUTIManager(inUTIManager), fCanAnalyseText(false)
{

}

IXMLHandler	* VUTIManagerXMLHandler::StartElement(const VString& inElementName)
{
	//DebugMsg( "VUTIManagerXMLHandler::XML element, name = %S\n", &inElementName);
	IXMLHandler * xmlHandlerOfTheTag = NULL;
	
	if((fLastKeyFound == kUTExportedTypeDeclarationsKey4D || fLastKeyFound == kUTImportedTypeDeclarationsKey4D) && inElementName == kArrayTagName){
		return new VUTIManagerArrayHandler(fUTIManager);
	}
	else{
		if(inElementName == kKeyTagName)
			fCanAnalyseText = true;
		this->Retain();
		xmlHandlerOfTheTag = this;
		fElementName = inElementName;
	}
	
	fLastKeyFound = CVSTR("");
		
	return xmlHandlerOfTheTag;
}

void VUTIManagerXMLHandler::SetAttribute(const VString& inName, const VString& inValue)
{

}


void VUTIManagerXMLHandler::SetText(const VString& inText)
{
	if(!inText.IsEmpty() && fCanAnalyseText){
		//DebugMsg( "VUTIManagerXMLHandler::XML element, Text = \"%S\" \n", &inText);
		if(fElementName == kKeyTagName){
			fLastKeyFound.AppendString(inText);
		}
		//DebugMsg( "VUTIManagerXMLHandler::Last Key found, Key = %S\n", &fLastKeyFound);
	}
}

void VUTIManagerXMLHandler::EndElement(const XBOX::VString&)
{
	fCanAnalyseText = false;
}


#pragma mark VUTIManagerArrayHandler

VUTIManagerArrayHandler::VUTIManagerArrayHandler(VUTIManager* inUTIManager):fUTIManager(inUTIManager)
{

}

IXMLHandler* VUTIManagerArrayHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "VUTIManagerArrayHandler::XML element, name = %S\n", &inElementName);
	if(inElementName == kDictTagName){
		return new VUTIManagerUTIHandler(fUTIManager);
	}
	else 
		return NULL;
}

void VUTIManagerArrayHandler::SetText(const VString& inText)
{

}

void VUTIManagerArrayHandler::EndElement(const XBOX::VString&)
{
}

#pragma mark VUTIManagerUTIHandler

VUTIManagerUTIHandler::VUTIManagerUTIHandler(VUTIManager* inUTIManager):fUTIManager(inUTIManager),fCanAnalyseText(false)
{
	fCurrentUTI = new VUTI();
	assert(fCurrentUTI != NULL);
}

VUTIManagerUTIHandler::~VUTIManagerUTIHandler()
{
	ReleaseRefCountable(&fCurrentUTI);
}


IXMLHandler* VUTIManagerUTIHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "VUTIManagerUTIHandler::XML element, name = %S\n", &inElementName);
	
	IXMLHandler * xmlHandlerOfTheTag = NULL;
	
	fElementName = inElementName;
	fXMLTagsStack.push(inElementName);
	if(!fLastKeyFound.IsEmpty()){
		if(fElementName == kStringTagName && (fLastKeyFound == kUTTypeIdentifierKey4D || fLastKeyFound == kUTTypeDescriptionKey4D)){
			fKeyToProcess = fLastKeyFound;
		}
		else if(fElementName == kArrayTagName && fLastKeyFound == kUTTypeConformsToKey4D){
			xmlHandlerOfTheTag = new VUTIManagerUTIKeyArrayStringHandler(fUTIManager, fCurrentUTI, fLastKeyFound);
			if(fXMLTagsStack.size() >  0)
				fXMLTagsStack.pop();
		}
		else if(fElementName == kDictTagName && fLastKeyFound == kUTTypeTagSpecificationKey4D){
			xmlHandlerOfTheTag = new VUTIManagerUTIKeyUTISpecificationHandler(fUTIManager, fCurrentUTI);
			if(fXMLTagsStack.size() >  0)
				fXMLTagsStack.pop();
		}
		else{
			fKeyToProcess = CVSTR("");
		}
	}	
	else{
		fKeyToProcess = CVSTR("");
	}
	
	if(xmlHandlerOfTheTag == NULL){
		this->Retain();
		xmlHandlerOfTheTag = this;
	}
	
	
	if(inElementName == kKeyTagName || inElementName == kStringTagName)
		fCanAnalyseText = true;
	
	fLastKeyFound.Clear();
	
	return xmlHandlerOfTheTag;
}

void VUTIManagerUTIHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	
}

void VUTIManagerUTIHandler::SetText( const VString& inText)
{
	if(!inText.IsEmpty() && fCanAnalyseText){
		//DebugMsg( "VUTIManagerUTIHandler::XML element, Text = %S\n", &inText);
		if(fElementName == kKeyTagName){
			fLastKeyFound.AppendString(inText);
			//DebugMsg( "VUTIManagerUTIHandler::Key = %S\n", &inText);
		}
		else if(fElementName == kStringTagName && fKeyToProcess == kUTTypeIdentifierKey4D){
			fCurrentUTI->SetIdentifier(fCurrentUTI->GetIdentifier().AppendString(inText));
			//VString identifierDebug = fCurrentUTI->GetIdentifier();
			//DebugMsg( "VUTIManagerUTIHandler::UTI Identifier = %S\n", &identifierDebug);
		}
		else if(fElementName == kStringTagName && fKeyToProcess == kUTTypeDescriptionKey4D){
			fCurrentUTI->SetDescription(fCurrentUTI->GetDescription().AppendString(inText));
			//VString descriptionDebug = fCurrentUTI->GetDescription();
			//DebugMsg( "VUTIManagerUTIHandler::UTI Description = %S\n", &descriptionDebug);
		}
	}
}

void VUTIManagerUTIHandler::EndElement(const XBOX::VString&)
{
	if(fXMLTagsStack.size() == 0 && fUTIManager != NULL){
		fUTIManager->AddNewUTI(fCurrentUTI);
		VString identifier = fCurrentUTI->GetIdentifier();
		
		//DEBUG
		/*VString extension1;
		if(fCurrentUTI->GetExtensionsVector().size() > 0)
			extension1 = fCurrentUTI->GetExtensionsVector().at(0);
		DebugMsg( "******* VUTIManagerUTIHandler::UTI added *******\nName : %S, Extension 1 : %S\n", &identifier, &extension1);*/
		//DEBUG
		
	}
	
	fCanAnalyseText = false;
	
	if(fXMLTagsStack.size() >  0){
		//DebugMsg( "VUTIManagerUTIHandler::Element End : %S\n\n", &fXMLTagsStack.top());
		fXMLTagsStack.pop();
	}
}


#pragma mark VUTIManagerUTIKeyUTISpecificationHandler 

VUTIManagerUTIKeyUTISpecificationHandler::VUTIManagerUTIKeyUTISpecificationHandler(VUTIManager* inUTIManager, VUTI* inUTI):fUTIManager(inUTIManager), fCurrentUTI(inUTI), fCanAnalyseText(false)

{
	
}

IXMLHandler* VUTIManagerUTIKeyUTISpecificationHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::XML element, name = %S\n", &inElementName);
	
	IXMLHandler * xmlHandlerOfTheTag = NULL;
	fElementName = inElementName;
	
	if(inElementName == kKeyTagName || inElementName == kStringTagName)
		fCanAnalyseText = true;
		
	if(fElementName == kStringTagName && !fLastKeyFound.IsEmpty()){
		if(fLastKeyFound == kUTTagClassFilenameExtension4D || fLastKeyFound == kUTTagClassMIMEType4D || fLastKeyFound == kUTTagClassOSType4D){
			fKeyToProcess = fLastKeyFound;
			this->Retain();
			xmlHandlerOfTheTag = this;
		}
	}
	else if(fElementName == kArrayTagName && !fLastKeyFound.IsEmpty()){
		//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::ARRAY met for the key = %S\n", &fLastKeyFound);
		xmlHandlerOfTheTag = new VUTIManagerUTIKeyArrayStringHandler(fUTIManager, fCurrentUTI, fLastKeyFound);
	}
	
	if(xmlHandlerOfTheTag == NULL){
		this->Retain();
		xmlHandlerOfTheTag = this;
	}
	
	fLastKeyFound.Clear();
	
	return xmlHandlerOfTheTag;
}

void VUTIManagerUTIKeyUTISpecificationHandler::EndElement(const XBOX::VString&)
{
	if(!fLastExtensionFound.IsEmpty()){
		fCurrentUTI->AddExtension(fLastExtensionFound);
		//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New Extension ADDED = %S\n", &fLastExtensionFound);
	}
	else if(!fLastOSTypeFound.IsEmpty()){
		fCurrentUTI->AddOSType(fLastOSTypeFound);
		//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New Extension ADDED = %S\n", &fLastOSTypeFound);
	}
	else if(!fLastMIMETypeFound.IsEmpty()){
		fCurrentUTI->AddMIMEType(fLastMIMETypeFound);
		//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New Extension ADDED = %S\n", &fLastMIMETypeFound);
	}
		
	fCanAnalyseText = false;
	
	fLastExtensionFound.Clear();
	fLastOSTypeFound.Clear();
	fLastMIMETypeFound.Clear();
}

void VUTIManagerUTIKeyUTISpecificationHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	
}

void VUTIManagerUTIKeyUTISpecificationHandler::SetText( const VString& inText)
{
	if(!inText.IsEmpty() && fCanAnalyseText){
		//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::XML element, Text = %S\n", &inText);
		if(fElementName == kKeyTagName){
			fLastKeyFound.AppendString(inText);
			//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::Key = %S\n", &inText);
		}
		else if(fElementName == kStringTagName ){
			if(fKeyToProcess == kUTTagClassFilenameExtension4D){
				fLastExtensionFound.AppendString(inText);
				//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New Extension found= %S\n", &fLastExtensionFound);
			}
			else if(fKeyToProcess == kUTTagClassOSType4D){
				fLastOSTypeFound.AppendString(inText);
				//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New OSType found = %S\n", &fLastOSTypeFound);
			}
			else if(fKeyToProcess == kUTTagClassMIMEType4D){
				fLastMIMETypeFound.AppendString(inText);
				//DebugMsg( "VUTIManagerUTIKeyUTISpecificationHandler::UTI New MIME/type found = %S\n", &fLastMIMETypeFound);
			}
		}
	}
}



#pragma mark VUTIManagerUTIKeyArrayStringHandler 


VUTIManagerUTIKeyArrayStringHandler::VUTIManagerUTIKeyArrayStringHandler(VUTIManager* inUTIManager, VUTI* inUTI, VString inKey): fUTIManager(inUTIManager), fCurrentUTI(inUTI), fKeyName(inKey), fCanAnalyseText(false)
{
	
}
	
IXMLHandler* VUTIManagerUTIKeyArrayStringHandler::StartElement( const VString& inElementName)
{
	//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::XML element, name = %S\n", &inElementName);
	
	if(inElementName == kStringTagName)
		fCanAnalyseText = true;
	
	fElementName = inElementName;
	this->Retain();
	return this;
}

void VUTIManagerUTIKeyArrayStringHandler::EndElement(const XBOX::VString&)
{
	if(fElementName == kStringTagName){
		if(fKeyName == kUTTypeConformsToKey4D && !fConformToUTIFound.IsEmpty()){
			//One create a temporary UTI that we will process at the end, because parent UTI can be declared after the UTI we process.
			VUTI * parentUTI = new VUTI();
			parentUTI->SetTemporary(true);
			parentUTI->SetIdentifier(fConformToUTIFound);
			fCurrentUTI->AddParentUTI(parentUTI);
			ReleaseRefCountable(&parentUTI); // parentUTI is retained by the UTI
		}
		else if(fKeyName == kUTTagClassFilenameExtension4D && !fLastExtensionFound.IsEmpty()){
			fCurrentUTI->AddExtension(fLastExtensionFound);
			//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::UTI New Extension = %S\n", &fLastExtensionFound);
		}
		else if(fKeyName == kUTTagClassOSType4D && !fLastOSTypeFound.IsEmpty()){
			fCurrentUTI->AddOSType(fLastOSTypeFound);
			//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::UTI New OSType = %S\n", &fLastOSTypeFound);
		}
		else if(fKeyName == kUTTagClassMIMEType4D && !fLastMIMETypeFound.IsEmpty()){
			fCurrentUTI->AddMIMEType(fLastMIMETypeFound);
			//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::UTI New MIME/type = %S\n", &fLastMIMETypeFound);
		}
	}
	
	fConformToUTIFound.Clear();
	fLastExtensionFound.Clear();
	fLastOSTypeFound.Clear();
	fLastMIMETypeFound.Clear();
	
	fCanAnalyseText = false;
}

void VUTIManagerUTIKeyArrayStringHandler::SetAttribute( const VString& inName, const VString& inValue)
{
	
}

void VUTIManagerUTIKeyArrayStringHandler::SetText( const VString& inText)
{
	if(!inText.IsEmpty() && fCanAnalyseText){
		//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::XML element, Text = %S\n", &inText);
		if(fElementName == kStringTagName){
			if(fKeyName == kUTTypeConformsToKey4D){
				fConformToUTIFound.AppendString(inText);
			}
			else if(fKeyName == kUTTagClassFilenameExtension4D){
				fLastExtensionFound.AppendString(inText);
				//DebugMsg( "---VUTIManagerUTIKeyArrayStringHandler::UTI Extension = %S\n", &fLastExtensionFound);
			}
			else if(fKeyName == kUTTagClassOSType4D){
				fLastOSTypeFound.AppendString(inText);
			}
			else if(fKeyName == kUTTagClassMIMEType4D){
				fLastMIMETypeFound.AppendString(inText);
			}
		}
	}
}

END_TOOLBOX_NAMESPACE

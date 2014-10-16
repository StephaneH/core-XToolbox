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
#include "VUTIManager.h"
#include "VUTIManagerXMLHandler.h"
#include "XMLSaxParser.h"
#include "4DUTType.h"
#include "VLocalizationManager.h"

BEGIN_TOOLBOX_NAMESPACE


#pragma mark Static

VUTIManager * VUTIManager::fSingleton = NULL;

VUTIManager * VUTIManager::GetUTIManager ()
{
	if (fSingleton == NULL)
		fSingleton =  new VUTIManager;
	assert(fSingleton != NULL);
	return fSingleton;
}

void VUTIManager::Delete()
{
	if (fSingleton != NULL){
		ReleaseRefCountable(&fSingleton);
		fSingleton = NULL;
	}
}

bool VUTIManager::Init()
{
	return true;
}

bool VUTIManager::DeInit()
{
	if(fSingleton != NULL)
		fSingleton->Delete();
	
	return (fSingleton == NULL);
}

#pragma mark Constructor & destructor

VUTIManager::VUTIManager()
{
	fLocalizationManager = NULL;
	fSAXParser = new VXMLParser();
	if(testAssert(fSAXParser != NULL)){
		fSAXParser->Init();
	}
	fSAXHandler = new VUTIManagerXMLHandler(this);
	assert(fSAXHandler != NULL);
}


VUTIManager::~VUTIManager()
{
	delete fSAXHandler;
	fSAXHandler = NULL;
	if(fSAXParser != NULL){
		fSAXParser->DeInit();
		delete fSAXParser;
	}
	
	//Release the UTIs
	std::map< VString, VUTI * >::iterator mainUTIsIterator = fBaseUTIsMap.begin();
	while(mainUTIsIterator != fBaseUTIsMap.end()){
		VUTI * processedUTI = mainUTIsIterator->second;
		processedUTI->_Detach();
		ReleaseRefCountable(&processedUTI);
		++mainUTIsIterator;
	}
	fBaseUTIsMap.clear();
}


#pragma mark Loading

bool VUTIManager::LoadXMLFile(VFile * inFile)
{
	bool xMLFileHasBeenAnalysed = false;
	
	if(inFile == NULL || !inFile->Exists())
		return false;
	
	if(AnalyzeXMLFile(inFile) == VE_OK){
		xMLFileHasBeenAnalysed = true;
		ProcessExtractedUTIs();
	}
	
	return xMLFileHasBeenAnalysed;
}


VError VUTIManager::AnalyzeXMLFile(const VFile* inFileToAnalyze)
{
	fSAXParser->Parse( const_cast<VFile*>( inFileToAnalyze), fSAXHandler, XML_ValidateNever);
	return VE_OK;
}

VError VUTIManager::ProcessExtractedUTIs()
{
	//The purpose here is to do a check of UTIs we extracted from the file we processed.	
	
	//Take UTIs one by one
	std::map< VString, VUTI * >::iterator mainUTIsIterator = fBaseUTIsMap.begin();
	while(mainUTIsIterator != fBaseUTIsMap.end()){
		//For each UTI
		VUTI * processedUTI = mainUTIsIterator->second;		
		std::map< VString, VUTI * >::iterator secondaryUTIsIterator = fBaseUTIsMap.begin();
		while(secondaryUTIsIterator != fBaseUTIsMap.end()){
			VUTI * potentialChildUTI = secondaryUTIsIterator->second;			
			if(!this->UTTypeEqual(processedUTI->GetIdentifier(), potentialChildUTI->GetIdentifier())){
				// - Check if other UTIs are children of the the UTI
				std::map< VString, VUTI * > parentUTIs = potentialChildUTI->GetParentUTIsMap();
				std::map< VString, VUTI * >::iterator parentUTIsIterator = parentUTIs.begin();
				while(parentUTIsIterator != parentUTIs.end()){
					VUTI * potentialParentUTI = parentUTIsIterator->second;
					if(potentialParentUTI->IsTemporary() && UTTypeEqual(potentialParentUTI->GetIdentifier(), processedUTI->GetIdentifier())){
						// OK, now we have an UTI that seems interesting to be a child of the UTI we process
						// - Check if this UTI is not already inserted in one of the child UTI
						// - Check if this UTI is not already inserted in one of the parent UTI 
						// - Check if this UTI is not already a parent
						if(!processedUTI->IsChildOrChildOfAParent(potentialChildUTI) && !processedUTI->IsParent(potentialParentUTI)){
							//D: VString processedUTIID = processedUTI->GetIdentifier();
							//D: VString potentialChildUTIID = potentialChildUTI->GetIdentifier();
							//D: DebugMsg("Parent : %S, Child : %S\n", &processedUTIID, &potentialChildUTIID);
							// Remove the temporary UTIs
							potentialChildUTI->RemoveTemporaryUTI(processedUTI->GetIdentifier()); 
							// Make the links
							processedUTI->AddChildUTI(potentialChildUTI);
							potentialChildUTI->AddParentUTI(processedUTI);
						}		
						else
							int i = 1;				
					}
					++parentUTIsIterator;
				}
			}
			++secondaryUTIsIterator;
		}
		
		++mainUTIsIterator;
	}	
	
	
	VFileKindManager * vFileKindManager = VFileKindManager::Get();
	if(vFileKindManager != NULL){
		mainUTIsIterator = fBaseUTIsMap.begin();
		while(mainUTIsIterator != fBaseUTIsMap.end()){
			//Register each UTI as a VFileKind in the VFileKindManager
			VUTI * uTIWeWantToRegister = mainUTIsIterator->second;
			if(uTIWeWantToRegister != NULL){
				VString uTI = uTIWeWantToRegister->GetIdentifier();
				VString description = uTIWeWantToRegister->GetDescription();

				//Translate the description if needed
				VString localizedDescription;
				if(fLocalizationManager != NULL){
					if(!fLocalizationManager->LocalizeStringWithDotStringsKey(description, localizedDescription))
						localizedDescription = description;
				}
				else
					localizedDescription = description;
				
				VectorOfFileExtension extensions = uTIWeWantToRegister->GetExtensionsVector();	
				VectorOfFileKind fileKind = uTIWeWantToRegister->GetOSTypesVector();
				std::map< VString, VUTI * > mapOfParentUTIs = uTIWeWantToRegister->GetParentUTIsMap();
				VectorOfFileKind vectorOfParentUTIs;
				for (std::map< VString, VUTI * >::iterator parentUTIsIter = mapOfParentUTIs.begin() ; parentUTIsIter != mapOfParentUTIs.end() ; ++parentUTIsIter)
					vectorOfParentUTIs.push_back( parentUTIsIter->first);

				// sc 11/01/2011 pass only the extensions for the UTI (do not append parent UTIs extensions) and pass the parent UTIs list
				vFileKindManager->RegisterPrivateFileKind( uTI, fileKind, localizedDescription, extensions, vectorOfParentUTIs);
			}
			++mainUTIsIterator;
		}
	}
	
	
	return VE_OK;
}


bool VUTIManager::AddNewUTI(VUTI * inNewUTI)
{
	bool uTIHasBeenInserted = false;
	
	if(!inNewUTI->GetIdentifier().IsEmpty()){
		std::pair< std::map< VString, VUTI * >::iterator, bool> resultOfInsert = fBaseUTIsMap.insert(std::map< VString, VUTI * >::value_type(inNewUTI->GetIdentifier(), inNewUTI));
		uTIHasBeenInserted = resultOfInsert.second;
		if(uTIHasBeenInserted)
			inNewUTI->Retain();
	}
	
	return uTIHasBeenInserted;
}


uLONG VUTIManager::GetUTIsCount()
{
	return (uLONG)fBaseUTIsMap.size();
}

void VUTIManager::SetLocalizationManager(VLocalizationManager* inLocalizationManager)
{
	fLocalizationManager = inLocalizationManager;
}

#pragma mark Standard methods

bool VUTIManager::UTTypeEqual(const VString& inTypeIdentifier1, const VString& inTypeIdentifier2)
{
	bool uTIsAreEquals = false;
	
	if(inTypeIdentifier1 == inTypeIdentifier2)
		uTIsAreEquals = true;
		
	return uTIsAreEquals;
}


std::vector< VUTI * > * VUTIManager::UTTypeCreateAllIdentifiersForTag(const VString& inTagClass, const VString& inTag, const VString& inConformingToTypeIdentifier)
{
	if(!testAssert(!inTagClass.IsEmpty()))
		return NULL;
		
	if(!testAssert(!inTag.IsEmpty()))
		return NULL;
	
	std::vector< VUTI * > * allUTIForTag = new std::vector< VUTI * >;
	//Take UTIs one by one
	std::map< VString, VUTI * >::iterator mainUTIsIterator = fBaseUTIsMap.begin();
	while(mainUTIsIterator != fBaseUTIsMap.end()){
		VUTI * uTI = mainUTIsIterator->second;
		if(uTI == NULL)
			continue;
			
		if(inTagClass == kUTTagClassFilenameExtension4D){
			if(uTI->ConformsToExtension(inTag)){
				uTI->Retain();
				allUTIForTag->push_back(uTI);
			}
		}
		else if(inTagClass == kUTTagClassOSType4D){
			if(uTI->ConformsToOSType(inTag)){
				uTI->Retain();
				allUTIForTag->push_back(uTI);
			}
		}
		else if(inTagClass == kUTTagClassMIMEType4D){
			if(uTI->ConformsToMIMEType(inTag)){
				uTI->Retain();
				allUTIForTag->push_back(uTI);
			}
		}
		
		++mainUTIsIterator;
	}
	
	return allUTIForTag;
}


VUTI * VUTIManager::UTTypeCreatePreferredIdentifierForTag(const VString& inTagClass, const VString& inTag, const VString& inConformingToTypeIdentifier)
{
	if(!testAssert(!inTagClass.IsEmpty()))
		return NULL;
		
	if(!testAssert(!inTag.IsEmpty()))
		return NULL;

	std::vector< VUTI * > preferredUTIForTag;
	//Take UTIs one by one
	std::map< VString, VUTI * >::iterator mainUTIsIterator = fBaseUTIsMap.begin();
	while(mainUTIsIterator != fBaseUTIsMap.end()){
		VUTI * uTI = mainUTIsIterator->second;
		if(uTI == NULL)
			continue;
			
		if(inTagClass == kUTTagClassFilenameExtension4D){
			VUTI * highestParentThatConformsToExtension = uTI->GetHighestParentThatConformsToExtension(inTag);
			if(highestParentThatConformsToExtension != NULL && (inConformingToTypeIdentifier.IsEmpty() || highestParentThatConformsToExtension->ConformsToIdentifier(inConformingToTypeIdentifier))){
				std::vector< VUTI * >::iterator preferredUTIForTagIterator = find(preferredUTIForTag.begin(), preferredUTIForTag.end(), highestParentThatConformsToExtension);
				if(preferredUTIForTagIterator == preferredUTIForTag.end()) 
					preferredUTIForTag.push_back(highestParentThatConformsToExtension);
			}
		}
		else if(inTagClass == kUTTagClassOSType4D){
			VUTI * highestParentThatConformsToOSType = uTI->GetHighestParentThatConformsToOSType(inTag);
			if(highestParentThatConformsToOSType != NULL && (inConformingToTypeIdentifier.IsEmpty() || highestParentThatConformsToOSType->ConformsToIdentifier(inConformingToTypeIdentifier))){
				std::vector< VUTI * >::iterator preferredUTIForTagIterator = find(preferredUTIForTag.begin(), preferredUTIForTag.end(), highestParentThatConformsToOSType);
				if(preferredUTIForTagIterator == preferredUTIForTag.end()) 
					preferredUTIForTag.push_back(highestParentThatConformsToOSType);
			}
		}
		else if(inTagClass == kUTTagClassMIMEType4D){
			VUTI * highestParentThatConformsToMIMEType = uTI->GetHighestParentThatConformsToMIMEType(inTag);
			if(highestParentThatConformsToMIMEType != NULL && (inConformingToTypeIdentifier.IsEmpty() || highestParentThatConformsToMIMEType->ConformsToIdentifier(inConformingToTypeIdentifier))){
				std::vector< VUTI * >::iterator preferredUTIForTagIterator = find(preferredUTIForTag.begin(), preferredUTIForTag.end(), highestParentThatConformsToMIMEType);
				if(preferredUTIForTagIterator == preferredUTIForTag.end()) 
					preferredUTIForTag.push_back(highestParentThatConformsToMIMEType);
			}
		}
		
		++mainUTIsIterator;
	}
	
	VUTI * preferredUTI = NULL;
	
	//If there is an ambiguity, prefer public items
	if(preferredUTIForTag.size() > 1){
		bool publicTagsPresent = false;
		bool nonPublicTagsPresent = false;
		std::vector< VUTI * >::iterator preferredUTIForTagIterator = preferredUTIForTag.begin();
		while(preferredUTIForTagIterator != preferredUTIForTag.end()){
			if(*preferredUTIForTagIterator != NULL && (*preferredUTIForTagIterator)->IsPublic())
				publicTagsPresent = true;
			else
				nonPublicTagsPresent = true;
			++preferredUTIForTagIterator;
		} 
		
		//If there is public & non-public UTIs, remove the non-public
		if(publicTagsPresent && nonPublicTagsPresent){
			preferredUTIForTagIterator = preferredUTIForTag.begin();
			while(preferredUTIForTagIterator != preferredUTIForTag.end()){
				VUTI * uTI = *preferredUTIForTagIterator;
				if(uTI != NULL && !uTI->IsPublic()){
					ReleaseRefCountable(&uTI);
					preferredUTIForTag.erase(preferredUTIForTagIterator);
					preferredUTIForTagIterator = preferredUTIForTag.begin();
				}
				else{
					++preferredUTIForTagIterator;
				}
			}
		}
		
		//Take the first one and release the others
		if(preferredUTIForTag.size() > 0){
			preferredUTI = preferredUTIForTag.at(0);
			preferredUTIForTagIterator = preferredUTIForTag.begin();
			while(preferredUTIForTagIterator != preferredUTIForTag.end()){
				if(preferredUTIForTagIterator != preferredUTIForTag.begin()){
					VUTI * uTI = *preferredUTIForTagIterator;
				 	if(uTI != NULL){
						ReleaseRefCountable(&uTI);
						preferredUTIForTag.erase(preferredUTIForTagIterator);
						preferredUTIForTagIterator = preferredUTIForTag.begin();
					}
					else
						++preferredUTIForTagIterator;
				}	
				else
					++preferredUTIForTagIterator;
			}
		}
	}
	else if(preferredUTIForTag.size() == 1)
		preferredUTI = preferredUTIForTag.at(0);
	
	return preferredUTI;
}

bool VUTIManager::UTTypeConformsTo(VFile * inFile, const VString& inConformsToTypeIdentifier)
{
	//Parameters checking
	if(!testAssert(inFile != NULL))
		return false;
	if(!testAssert(inFile->Exists()))
		return false;
	if(!testAssert(!inConformsToTypeIdentifier.IsEmpty()))
		return false;
		
	VUTI * extensionUTI = NULL;
	VUTI * oSTypeUTI = NULL;
	
	//Extract all the properties we can from the file
	VString fileExtension;
	inFile->GetExtension(fileExtension);
	if(!fileExtension.IsEmpty())
		extensionUTI = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension4D, fileExtension, CVSTR(""));
	
#if VERSIONMAC
	OsType fileOSType;
	if(inFile->MAC_GetKind(&fileOSType) == VE_OK){
		VString oSTypeString;
		oSTypeString.FromOsType(fileOSType);
		//D: DebugMsg("OS Type UTI : \"%S\" - inConformsToTypeIdentifier : \"%S\"\n", &oSTypeString, &inConformsToTypeIdentifier);
		oSTypeUTI = UTTypeCreatePreferredIdentifierForTag(kUTTagClassOSType4D, oSTypeString, CVSTR(""));
	}
#endif
	
	// Take the lowest UTI in the tree to define the file UTI.
	VUTI * fileUTI = NULL;
	if(extensionUTI != NULL){
		if(oSTypeUTI != NULL && !UTTypeEqual(extensionUTI->GetIdentifier(), oSTypeUTI->GetIdentifier())){
			if(oSTypeUTI->IsParent(extensionUTI))
				fileUTI = oSTypeUTI;
			else if(extensionUTI->IsParent(oSTypeUTI))
				fileUTI = extensionUTI;
			else{
				//This is a complicated case, so in case of doubt we only take in account the extension.
				fileUTI = extensionUTI;
			}
		}
		else
			fileUTI = extensionUTI;
	}
	else
		fileUTI = oSTypeUTI;
	
	
	bool fileConformsToUTI = false;
	if(fileUTI != NULL)
		fileConformsToUTI = fileUTI->ConformsToIdentifier(inConformsToTypeIdentifier);
	
	ReleaseRefCountable(&oSTypeUTI);
	ReleaseRefCountable(&extensionUTI);
	
	return fileConformsToUTI;
}

END_TOOLBOX_NAMESPACE

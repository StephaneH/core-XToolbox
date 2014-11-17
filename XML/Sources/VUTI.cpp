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
#include "VUTI.h"

BEGIN_TOOLBOX_NAMESPACE

const VString kPublicUTIIdentifierPrefix("public");

#pragma mark Constructor & Destructor

VUTI::VUTI()
{
	fIsDynamic = false;
	fIsTemporary = false;
}


VUTI::~VUTI()
{
	_Detach();
}

void VUTI::_Detach()
{
	//Release all the parent UTIs
	std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
	while(parentUTIsIterator != fParentUTIsMap.end())
	{
		VUTI * parentUTI = parentUTIsIterator->second;
		fParentUTIsMap.erase(parentUTIsIterator);
		parentUTIsIterator = fParentUTIsMap.begin();
		ReleaseRefCountable(&parentUTI);
	}
	
	//Release all the children 
	std::map< VString, VUTI * >::iterator childUTIsIterator = fChildrenUTIsMap.begin();
	while(childUTIsIterator != fChildrenUTIsMap.end()){
		VUTI * childUTI = childUTIsIterator->second;
		fChildrenUTIsMap.erase(childUTIsIterator);
		childUTIsIterator = fChildrenUTIsMap.begin();
	 	ReleaseRefCountable(&childUTI);
	}
}
#pragma mark Simple attributes

VString VUTI::GetIdentifier()
{
	return fIdentifier;
}


void VUTI::SetIdentifier(VString inIdentifier)
{
	fIdentifier = inIdentifier;
}


bool VUTI::ConformsToIdentifier(const VString& inIdentifier)
{
	bool conformsToIdentifier = false;
	if(!inIdentifier.IsEmpty()){
		if(inIdentifier == this->GetIdentifier()) {
			conformsToIdentifier = true;
		}
		else{
			std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
			while(parentUTIsIterator != fParentUTIsMap.end()){
				VUTI * parentUTI = parentUTIsIterator->second;
			//Exclude temporary values from the process
				if(parentUTI != NULL && !parentUTI->IsTemporary()){
					conformsToIdentifier |= parentUTI->ConformsToIdentifier(inIdentifier);
					if(conformsToIdentifier == true)
						break;
				}
				++parentUTIsIterator;
			}
		}
	}
	return conformsToIdentifier;
}

VString VUTI::GetDescription()
{
	return fDescription;
}


void VUTI::SetDescription(VString inDescription)
{
	fDescription = inDescription;
}


VString VUTI::GetReference()
{
	return fReference;
}


void VUTI::SetReference(VString inReference)
{
	fReference = inReference;
}


VFilePath VUTI::GetImagePath()
{
	return fImagePath;
}


void VUTI::SetImagePath(VFilePath inImagePath)
{
	fImagePath = inImagePath;
}



#pragma mark Types




std::vector< VString > VUTI::GetExtensionsVector()
{
	return fExtensionsVector;
}

void VUTI::GetAllExtensionsVector(std::vector< VString >& outAllExtensionsVector)
{		
	//Copy the UTI extensions
	std::vector< VString >::iterator extensionsVectorIterator = fExtensionsVector.begin();
	while(extensionsVectorIterator != fExtensionsVector.end()){
		std::vector< VString >::iterator tempExtensionsVectorIterator = find(outAllExtensionsVector.begin(), outAllExtensionsVector.end(), *extensionsVectorIterator);
		if(tempExtensionsVectorIterator == outAllExtensionsVector.end())
			outAllExtensionsVector.push_back(*extensionsVectorIterator);
		++extensionsVectorIterator;
	}
	
	//Then the parent ones
	std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
	while(parentUTIsIterator != fParentUTIsMap.end()){
		VUTI * parentUTI = parentUTIsIterator->second;
		//Exclude temporary values from the process
		if(parentUTI != NULL && !parentUTI->IsTemporary()){
			parentUTI->GetAllExtensionsVector(outAllExtensionsVector);
		}
		++parentUTIsIterator;
	}
}


void VUTI::SetExtensionsVector(std::vector< VString > inExtensionsVector)
{
	fExtensionsVector = inExtensionsVector;
}


void VUTI::AddExtension(VString inExtension)
{
	std::vector< VString >::iterator extensionsVectorIterator = find(fExtensionsVector.begin(), fExtensionsVector.end(), inExtension);
	if(extensionsVectorIterator == fExtensionsVector.end()) 
		fExtensionsVector.push_back(inExtension);
}

bool VUTI::ConformsToExtension(const VString& inExtension)
{
	bool conformsToExtension = false;
	VUTI * highestParentThatConformsToExtension = GetHighestParentThatConformsToExtension(inExtension);
	if(highestParentThatConformsToExtension != NULL)
		conformsToExtension = true;
	ReleaseRefCountable(&highestParentThatConformsToExtension);
	return conformsToExtension;
}

VUTI * VUTI::GetHighestParentThatConformsToExtension(const VString& inExtension)
{
	VUTI * highestParentThatConformsToExtension = NULL;
	std::vector< VString >::iterator extensionsVectorIterator = find(fExtensionsVector.begin(), fExtensionsVector.end(), inExtension);
	if(extensionsVectorIterator != fExtensionsVector.end()) {
		this->Retain();
		highestParentThatConformsToExtension = this;
	}
	else{
		std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
		while(parentUTIsIterator != fParentUTIsMap.end()){
			VUTI * parentUTI = parentUTIsIterator->second;
			//Exclude temporary values from the process
			if(parentUTI != NULL && !parentUTI->IsTemporary()){
				highestParentThatConformsToExtension = parentUTI->GetHighestParentThatConformsToExtension(inExtension);
				if(highestParentThatConformsToExtension != NULL)
					break;
			}
			++parentUTIsIterator;
		}
	}
	return highestParentThatConformsToExtension;
}


std::vector< VString > VUTI::GetOSTypesVector()
{
	return fOSTypesVector;
}


void VUTI::SetOSTypesVector(std::vector< VString > inOSTypesVector)
{
	fOSTypesVector = inOSTypesVector;
}


void VUTI::AddOSType(VString inOSType)
{
	std::vector< VString >::iterator oSTypesVectorIterator = find(fOSTypesVector.begin(), fOSTypesVector.end(), inOSType);
	if(oSTypesVectorIterator == fOSTypesVector.end()) 
		fOSTypesVector.push_back(inOSType);
}

bool VUTI::ConformsToOSType(const VString& inOSType)
{
	bool conformsToOSType = false;
	VUTI * highestParentThatConformsToOSType = GetHighestParentThatConformsToOSType(inOSType);
	if(highestParentThatConformsToOSType != NULL)
		conformsToOSType = true;
	ReleaseRefCountable(&highestParentThatConformsToOSType);
	return conformsToOSType;
}


VUTI * VUTI::GetHighestParentThatConformsToOSType(const VString& inOSType)
{
	VUTI * highestParentThatConformsToOSType = NULL;
	std::vector< VString >::iterator oSTypesVectorIterator = find(fOSTypesVector.begin(), fOSTypesVector.end(), inOSType);
	if(oSTypesVectorIterator != fOSTypesVector.end()){
		this->Retain();
		highestParentThatConformsToOSType = this;
	} 
	else{
		std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
		while(parentUTIsIterator != fParentUTIsMap.end()){
			VUTI * parentUTI = parentUTIsIterator->second;
			//Exclude temporary values from the process
			if(parentUTI != NULL && !parentUTI->IsTemporary()){
				highestParentThatConformsToOSType = parentUTI->GetHighestParentThatConformsToOSType(inOSType);
				if(highestParentThatConformsToOSType != NULL)
					break;
			}
			++parentUTIsIterator;
		}
	}
	return highestParentThatConformsToOSType;
}



std::vector< VString > VUTI::GetMIMETypesVector()
{
	return fMIMETypesVector;
}


void VUTI::SetMIMETypesVector(std::vector< VString > inMIMETypesVector)
{
	fMIMETypesVector = inMIMETypesVector;
}


void VUTI::AddMIMEType(VString inMIMEType)
{
	std::vector< VString >::iterator mIMETypesVectorIterator = find(fMIMETypesVector.begin(), fMIMETypesVector.end(), inMIMEType);
	if(mIMETypesVectorIterator == fMIMETypesVector.end()) 
		fMIMETypesVector.push_back(inMIMEType);
}

bool VUTI::ConformsToMIMEType(const VString& inMIMEType)
{
	bool conformsToMIMEType = false;
	VUTI * highestParentThatConformsToMIMEType = GetHighestParentThatConformsToMIMEType(inMIMEType);
	if(highestParentThatConformsToMIMEType != NULL)
		conformsToMIMEType = true;
	ReleaseRefCountable(&highestParentThatConformsToMIMEType);
	return conformsToMIMEType;
}	

VUTI * VUTI::GetHighestParentThatConformsToMIMEType(const VString& inMIMEType)
{
	VUTI * highestParentThatConformsToMIMEType = NULL;
	std::vector< VString >::iterator mIMETypeVectorIterator = find(fMIMETypesVector.begin(), fMIMETypesVector.end(), inMIMEType);
	if(mIMETypeVectorIterator != fMIMETypesVector.end()) {
		this->Retain();
		highestParentThatConformsToMIMEType = this;	
	}
	else{
		std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
		while(parentUTIsIterator != fParentUTIsMap.end()){
			VUTI * parentUTI = parentUTIsIterator->second;
			//Exclude temporary values from the process
			if(parentUTI != NULL && !parentUTI->IsTemporary()){
				highestParentThatConformsToMIMEType = parentUTI->GetHighestParentThatConformsToMIMEType(inMIMEType);
				if(highestParentThatConformsToMIMEType != NULL)
					break;
			}
			++parentUTIsIterator;
		}
	}
	return highestParentThatConformsToMIMEType;
}

std::map< VString, VUTI * > VUTI::GetParentUTIsMap()
{
	return fParentUTIsMap;
}


void VUTI::SetParentUTIsMap(std::map< VString, VUTI * > inParentUTIsMap)
{
	fParentUTIsMap = inParentUTIsMap;
}


void VUTI::AddParentUTI(VUTI * inParentUTI)
{
	if(!testAssert(inParentUTI != NULL))
		return;
	
	//Here we can't make a check of the UTI being equal to or being a subset of another UTI (because there are a lot of parameters we are not aware of, so we can't know if two UTIs are equals). So only pass an UTI if you are sure of what you do. Typically this should only be accessed by VUTIManager.
	if(!inParentUTI->GetIdentifier().IsEmpty()){
		std::pair< std::map< VString, VUTI * >::iterator, bool> resultOfInsert = fParentUTIsMap.insert(std::map< VString, VUTI * >::value_type(inParentUTI->GetIdentifier(), inParentUTI));
		if(resultOfInsert.second == true)
			inParentUTI->Retain();
	}
	
	return;
}

bool VUTI::IsParent(VUTI * inUTI)
{
	if(!testAssert(inUTI != NULL))
		return false;
	
	bool isParent = false;
	
	std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
	while(parentUTIsIterator != fParentUTIsMap.end()){
		VUTI * parentUTI = parentUTIsIterator->second;
		//Exclude temporary values from the process
		if(!parentUTI->IsTemporary()){
			isParent |= VUTIManager::GetUTIManager()->UTTypeEqual(inUTI->GetIdentifier(), parentUTI->GetIdentifier());
			if(isParent)
				break;
			isParent |= parentUTI->IsParent(inUTI);
			if(isParent)
				break;
		}
		++parentUTIsIterator;
	}
	return isParent;
}

bool VUTI::CouldBeAParent(VUTI * inUTI)
{
	if(!testAssert(inUTI != NULL))
		return false;
		
	bool couldBeAParent = false;
	std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
	while(parentUTIsIterator != fParentUTIsMap.end()){
		VUTI * parentUTI = parentUTIsIterator->second;
		couldBeAParent |= VUTIManager::GetUTIManager()->UTTypeEqual(inUTI->GetIdentifier(), parentUTI->GetIdentifier());
		if(couldBeAParent)
			break;
		couldBeAParent |= parentUTI->IsParent(inUTI);
		if(couldBeAParent)
				break;
		++parentUTIsIterator;
	}
	return couldBeAParent;
}


std::map< VString, VUTI * > VUTI::GetChildrenUTIsMap()
{
	return fChildrenUTIsMap;
}

void VUTI::SetChildrenUTIsMap(std::map< VString, VUTI * > inChildrenUTIsMap)
{
	fChildrenUTIsMap = inChildrenUTIsMap;
}

void VUTI::AddChildUTI(VUTI * inChildUTI)
{
	if(!testAssert(inChildUTI != NULL))
		return;
		
	if(!inChildUTI->GetIdentifier().IsEmpty()){
		std::pair< std::map< VString, VUTI * >::iterator, bool> resultOfInsert = fChildrenUTIsMap.insert(std::map< VString, VUTI * >::value_type(inChildUTI->GetIdentifier(), inChildUTI));
		if(resultOfInsert.second == true)
			inChildUTI->Retain();
	}
}

bool VUTI::IsChild(VUTI * inUTI)
{
	if(!testAssert(inUTI != NULL))
		return false;
	
	bool isChild = false;
	std::map< VString, VUTI * >::iterator childUTIsIterator = fChildrenUTIsMap.begin();
	while(childUTIsIterator != fChildrenUTIsMap.end()){
		VUTI * childUTI = childUTIsIterator->second;
		//Exclude temporary values from the process
		if(!childUTI->IsTemporary()){
			isChild |= VUTIManager::GetUTIManager()->UTTypeEqual(inUTI->GetIdentifier(), childUTI->GetIdentifier());
			if(isChild)
				break;
			isChild |= childUTI->IsChild(inUTI);
			if(isChild)
				break;
		}
		++childUTIsIterator;
	}
	return isChild;
}


bool VUTI::CouldBeAChild(VUTI * inUTI)
{
	if(!testAssert(inUTI != NULL))
		return false;
		
	bool couldBeAChild = false;
	
	std::map< VString, VUTI * >::iterator childUTIsIterator = fChildrenUTIsMap.begin();
	while(childUTIsIterator != fChildrenUTIsMap.end()){
		VUTI * childUTI = childUTIsIterator->second;
		couldBeAChild |= VUTIManager::GetUTIManager()->UTTypeEqual(inUTI->GetIdentifier(), childUTI->GetIdentifier());
		if(couldBeAChild)
			break;
		couldBeAChild |= childUTI->IsChild(inUTI);
		if(couldBeAChild)
			break;
		++childUTIsIterator;
	}
	return couldBeAChild;
}


bool VUTI::IsChildOrChildOfAParent(VUTI * inUTI)
{
	if(!testAssert(inUTI != NULL))
		return false;
	
	bool isChildOrChildOfAParent = false;
	if(IsChild(inUTI))
		isChildOrChildOfAParent = true;
	else{
		std::map< VString, VUTI * >::iterator parentUTIsIterator = fParentUTIsMap.begin();
		while(parentUTIsIterator != fParentUTIsMap.end()){
			VUTI * parentUTI = parentUTIsIterator->second;
			//Exclude temporary values from the process
			if(!parentUTI->IsTemporary()){
				isChildOrChildOfAParent |= parentUTI->IsChildOrChildOfAParent(inUTI);
				if(isChildOrChildOfAParent)
					break;
			}
			++parentUTIsIterator;
		}
	}
	return isChildOrChildOfAParent;
}


void VUTI::RemoveTemporaryUTI(VString inUTIIdentifier)
{
	std::map< VString, VUTI * >::iterator parentUTIIterator = fParentUTIsMap.find(inUTIIdentifier);
	if(parentUTIIterator != fParentUTIsMap.end()){
		if(parentUTIIterator->second->IsTemporary())
			fParentUTIsMap.erase(parentUTIIterator);
	}
	
	std::map< VString, VUTI * >::iterator childUTIIterator = fChildrenUTIsMap.find(inUTIIdentifier);
	if(childUTIIterator != fChildrenUTIsMap.end()){
		if(childUTIIterator->second->IsTemporary())
			fChildrenUTIsMap.erase(childUTIIterator);
	}
}


bool VUTI::IsPublic()
{
	return fIdentifier.BeginsWith(kPublicUTIIdentifierPrefix);
}

#pragma mark Dynamic case

bool VUTI::IsDynamic()
{
	return fIsDynamic;
}


void VUTI::SetDynamic(bool inIsDynamic)
{
	fIsDynamic = inIsDynamic;
}


#pragma mark Temporary

bool VUTI::IsTemporary()
{
	return fIsTemporary;
}


void VUTI::SetTemporary(bool inIsTemporary)
{
	fIsTemporary = inIsTemporary;
}

END_TOOLBOX_NAMESPACE

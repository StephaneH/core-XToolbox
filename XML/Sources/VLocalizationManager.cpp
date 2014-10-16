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
#include "VLocalizationManager.h"
#include "VLocalizationXMLHandler.h"
#include "XMLSaxParser.h"

BEGIN_TOOLBOX_NAMESPACE

static const VString kXliffExtension(L"xlf");
static const VString kStringsExtension(L"strings");

#pragma mark Public

VLocalizationManager::VLocalizationManager(DialectCode inDialectCode): fCurrentDialectCode(inDialectCode)
{
	fSAXParser = new VXMLParser();
	fSAXParser->Init();
	fSAXHandler = new VLocalizationXMLHandler(this);
	fLocalizedStringsSet = new StringsSet();
}

VLocalizationManager::~VLocalizationManager()
{
	delete fSAXHandler;
	if (fSAXParser != NULL)
	{
		fSAXParser->DeInit();
		delete fSAXParser;
	}
	delete fLocalizedStringsSet;
}

bool VLocalizationManager::ClearLocalizations()
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);

	delete fLocalizedStringsSet;
	fStringsRelativeToSTRSharpCodes.clear();
	fStringsRelativeToObjects.clear();
	fStringsAndIDsRelativeToGroups.clear();
	fStringsRelativeToDotStrings.clear();
	
	fLocalizedStringsSet = new StringsSet();

	fGroupBagsByResname.clear();
	fGroupBagsByRestype.clear();
	
	return true;
}

bool VLocalizationManager::DoesNeedAnUpdate()
{
	bool needAnUpdate = false;
	
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	
	//Check the last modification date of every file
	FilePathAndTimeMap newFilesProcessedAndLastModificationTime;
	FilePathAndTimeMap::iterator fileIterator = fFilesProcessedAndLastModificationTime.begin();
	while(fileIterator != fFilesProcessedAndLastModificationTime.end()){
		VFile * fileWeAnalyse = new VFile(fileIterator->first);
		if(fileWeAnalyse !=  NULL && fileWeAnalyse->Exists()){
			VTime lastModification;
			if(fileWeAnalyse->GetTimeAttributes(&lastModification, NULL, NULL) == VE_OK){
				newFilesProcessedAndLastModificationTime.insert(FilePathAndTimeMap::value_type(fileWeAnalyse->GetPath(), lastModification));
				if(lastModification != fileIterator->second){
					needAnUpdate = true;
				}
			}
			ReleaseRefCountable(&fileWeAnalyse);
		}
		else{
			needAnUpdate = true;
		}
		++fileIterator;
	}
	fFilesProcessedAndLastModificationTime = newFilesProcessedAndLastModificationTime;
	
	
	//Check if the user hasn't added a file in one of the localizations folders
	if(!needAnUpdate){
		std::vector<VFilePath>::iterator folderPathIterator = fFilesAndFoldersProcessed.begin();
		while(folderPathIterator != fFilesAndFoldersProcessed.end()){
			if(folderPathIterator->IsFolder()){
				VFolder * folderWeAnalyse = new VFolder(*folderPathIterator);
				if(folderWeAnalyse != NULL && folderWeAnalyse->Exists()){
					VFileIterator xLIFFFileIterator(folderWeAnalyse, FI_WANT_FILES);
					while(xLIFFFileIterator.IsValid()){
						VFile * fileToAnalyse = &(*xLIFFFileIterator);
						if(fileToAnalyse->MatchExtension(kXliffExtension)){
							if(fFilesProcessedAndLastModificationTime.find(fileToAnalyse->GetPath()) == fFilesProcessedAndLastModificationTime.end()){
								//If we don't find the file in our list, the folder should be re-parsed to parse the new one.
								needAnUpdate = true;
							}
						}
						++xLIFFFileIterator;
					}
				}
				else 
					needAnUpdate = true;
				
				ReleaseRefCountable(&folderWeAnalyse);
				if(needAnUpdate)
					break;
			}
			++folderPathIterator;
		}
	}
	
	return needAnUpdate;
}

bool VLocalizationManager::UpdateIfNeeded()
{
	if(DoesNeedAnUpdate()){
		//Cleaning...
		ClearLocalizations();

		//We reload each folder or file processed, in the right order
		std::vector<VFilePath>::iterator filesAndFoldersProcessedIterator = fFilesAndFoldersProcessed.begin();
		while (filesAndFoldersProcessedIterator != fFilesAndFoldersProcessed.end()) {
			if(&(*filesAndFoldersProcessedIterator) != NULL && filesAndFoldersProcessedIterator->IsValid() && !filesAndFoldersProcessedIterator->GetPath().IsEmpty()){
				if(filesAndFoldersProcessedIterator->IsFolder()){
					VFolder * folderToReload = new VFolder(*filesAndFoldersProcessedIterator);
					if(folderToReload && folderToReload->Exists())
						ScanAndLoadFolder(folderToReload);
					ReleaseRefCountable(&folderToReload);
				}
				else if(filesAndFoldersProcessedIterator->IsFile()){
					VFile * fileToReload = new VFile(*filesAndFoldersProcessedIterator);
					if(fileToReload && fileToReload->Exists())
						LoadFile(fileToReload);
					ReleaseRefCountable(&fileToReload);
				}	
			}
			++filesAndFoldersProcessedIterator;
		}

		return true;
	}

	return false;
}

VError VLocalizationManager::LoadFile(VFile* inFileToAdd, bool inForceLoading)
{
	return _LoadFile(inFileToAdd, false, inForceLoading);
}

VError VLocalizationManager::_LoadFile(VFile* inFileToAdd, bool actuallyLoadingOfAFolder, bool inForceLoading)
{
	if (!testAssert(inFileToAdd != NULL))
		return VE_INVALID_PARAMETER;
	
	if (!inFileToAdd->Exists())
		return VE_FILE_NOT_FOUND;

	//Extensions comparison
	if (inFileToAdd->MatchExtension(kXliffExtension))
	{
		//The file seems to be a XLIFF file, let's analyse it
		if (AnalyzeXLIFFFile(inFileToAdd, inForceLoading) == VE_OK)
		{			
			//We need to know the successfully loaded files
			if (!actuallyLoadingOfAFolder)
			{
				bool alreadyExists = false;
				for( std::vector<VFilePath>::iterator i = fFilesAndFoldersProcessed.begin() ; (i != fFilesAndFoldersProcessed.end()) && !alreadyExists ; ++i)
				{
					if (i->IsFile())
					{
						VFile f( *i);
						alreadyExists = inFileToAdd->IsSameFile( &f);
					}
				}
				//If the file isn't in the list, let's add it
				if (!alreadyExists)
				{
					fFilesAndFoldersProcessed.push_back(inFileToAdd->GetPath());
				}
			}
			//We also need the modification time of every file
			VTime lastModificationTime;
			inFileToAdd->GetTimeAttributes(&lastModificationTime);
			std::pair< FilePathAndTimeMap::iterator, bool > resultOfInsert = fFilesProcessedAndLastModificationTime.insert(FilePathAndTimeMap::value_type(inFileToAdd->GetPath(), lastModificationTime));
			if (!resultOfInsert.second)
			{
				resultOfInsert.first->second = lastModificationTime;
			}
		}
		return VE_OK;
	}
	else if (inFileToAdd->MatchExtension(kStringsExtension))
	{
		AnalyzeDotStringsFile(inFileToAdd);	
	}
	return VE_FILE_BAD_KIND;
}

VError VLocalizationManager::ScanAndLoadFolder( VFolder* inFolderToScan, bool inForceLoading)
{
	if (!testAssert(inFolderToScan != NULL))
		return VE_INVALID_PARAMETER;
	
	if (!inFolderToScan->Exists())
		return VE_FILE_NOT_FOUND;

	for( VFileIterator i(inFolderToScan, FI_WANT_FILES) ; i.IsValid() ; ++i) 
	{
		//The XML Localization Manager automatically checks if the file is valid
		_LoadFile( &*i, true, inForceLoading);
	}

	bool alreadyExists = false;
	for( std::vector<VFilePath>::iterator i = fFilesAndFoldersProcessed.begin() ;  (i != fFilesAndFoldersProcessed.end()) && !alreadyExists ; ++i)
	{
		if (i->IsFolder())
		{
			VFolder f( *i);
			alreadyExists = inFolderToScan->IsSameFolder( &f);
		}
	}

	//If the folder isn't in the list, let's add it
	if (!alreadyExists)
	{
		fFilesAndFoldersProcessed.push_back( inFolderToScan->GetPath());
	}

	return VE_OK;
}

bool VLocalizationManager::LoadDefaultLocalizationFolders( VFolder * inFolderThatContainsLocalizationFolders)
{
	if (inFolderThatContainsLocalizationFolders == NULL || !inFolderThatContainsLocalizationFolders->Exists())
		return false;

	// also parse xliff files we may find in the resources folder for non optionnally localized strings such as constants.
	ScanAndLoadFolder( inFolderThatContainsLocalizationFolders, true);

	VectorOfVFolder localizationFolders;
	if (!VIntlMgr::GetLocalizationFoldersWithDialect(fCurrentDialectCode, inFolderThatContainsLocalizationFolders->GetPath(), localizationFolders))
		return false;

	bool oneFileAtLeastWasLoaded = false;
	for( VectorOfVFolder::const_iterator i = localizationFolders.begin() ; i != localizationFolders.end() ; ++i)
	{
		if (ScanAndLoadFolder(*i) == VE_OK)
			oneFileAtLeastWasLoaded = true;
	}

	return oneFileAtLeastWasLoaded;
}

bool VLocalizationManager::LoadDefaultLocalizationFoldersForComponentOrPlugin( VFolder * inComponentOrPluginFolder)
{
	if(inComponentOrPluginFolder == NULL || !inComponentOrPluginFolder->Exists())
		return false;
	
	//Go to the right Folder (./Contents/Resources/)
	VFilePath componentOrPluginFolderPath = inComponentOrPluginFolder->GetPath();
	componentOrPluginFolderPath.ToSubFolder(CVSTR("Contents")).ToSubFolder(CVSTR("Resources"));
	VFolder * localizationsFolder = new VFolder(componentOrPluginFolderPath);
	
	bool result = LoadDefaultLocalizationFolders(localizationsFolder);
	
	ReleaseRefCountable(&localizationsFolder);
	
	return result;
}

//Localization
bool VLocalizationManager::LocalizeStringWithKey(const VString& inKeyToLookUp, VString& outLocalizedString)
{
	bool result = false;
	
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	OOSyntaxStringAndStringMap::iterator urlToStringHashMapIterator = fStringsRelativeToObjects.find(inKeyToLookUp);
	if(urlToStringHashMapIterator != fStringsRelativeToObjects.end()){
		outLocalizedString = *(urlToStringHashMapIterator->second);
		result = true;
	}
	
	return result;
}

bool VLocalizationManager::LocalizeStringWithSTRSharpCodes(const STRSharpCodes& inSTRSharpCodesToLookUp, VString& outLocalizedString)
{
	bool result = false;
	
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	STRSharpCodeAndStringMap::iterator stringsMapsRelativeToSTRSharpCodesIterator = fStringsRelativeToSTRSharpCodes.find(inSTRSharpCodesToLookUp);
	if(stringsMapsRelativeToSTRSharpCodesIterator != fStringsRelativeToSTRSharpCodes.end()){
		outLocalizedString = *(stringsMapsRelativeToSTRSharpCodesIterator->second);
		result = true;
	}

	return result;
}


bool VLocalizationManager::LocalizeGroupOfStringsWithAStrSharpID( sLONG inID, std::vector<VString>& outLocalizedStrings)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);

	outLocalizedStrings.clear();
	try
	{
		uLONG index = 1;
		do
		{
			STRSharpCodeAndStringMap::iterator i = fStringsRelativeToSTRSharpCodes.find( STRSharpCodes( inID, index));
			if (i == fStringsRelativeToSTRSharpCodes.end())
				break;
			outLocalizedStrings.push_back( *(i->second));
			++index;
		} while( true);
	}
	catch(...)
	{
		outLocalizedStrings.clear();
		xbox_assert( false);
	}
	
	return !outLocalizedStrings.empty();
}


bool VLocalizationManager::GetOrderedStringsOfGroup(const VString& groupName, std::vector<VString>& outLocalizedStringsVector)
{
	outLocalizedStringsVector.clear();

	bool result = false;
	
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	GroupToIDAndStringsMap::iterator groupsMapIterator = fStringsAndIDsRelativeToGroups.find(groupName);
	if(groupsMapIterator != fStringsAndIDsRelativeToGroups.end()){
		if(groupsMapIterator->second.size() > 0){
			
			// Thanks to the map, which is automatically sorted relatively to the id, outLocalizedStringsVector is sorted  
			std::map< uLONG, VString * >::iterator stringsIterator = groupsMapIterator->second.begin();
			while(stringsIterator !=  groupsMapIterator->second.end()){
				VString newString(*(stringsIterator->second));
				outLocalizedStringsVector.push_back(newString);
				++stringsIterator;
			}
			
			result = true;
		}
	}

	return result;
}

bool VLocalizationManager::LocalizeStringWithDotStringsKey(const VString& inKeyToLookUp, VString& outLocalizedString)
{
	bool result = false;
	
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	DotStringsAndStringsMap::iterator dotStringsKeyToStringHashMapIterator = fStringsRelativeToDotStrings.find(inKeyToLookUp);
	if(dotStringsKeyToStringHashMapIterator != fStringsRelativeToDotStrings.end()){
		outLocalizedString = *(dotStringsKeyToStringHashMapIterator->second);
		result = true;
	}
	
	return result;
}

bool VLocalizationManager::LocalizeErrorMessage( VError inErrorCode, VString& outLocalizedMessage)
{
	// build resname: ERR_DB4D_42
	char	cresname[16];
	OsType comp = COMPONENT_FROM_VERROR( inErrorCode);
	sLONG err = ERRCODE_FROM_VERROR( inErrorCode);
	sprintf( cresname, "ERR_%c%c%c%c_%d", (char) (comp >> 24), (char) (comp  >> 16), (char) (comp >> 8), (char) comp, err);

	// lookup localized string
	return LocalizeStringWithKey( VString( cresname), outLocalizedMessage);
}


//Localization language
DialectCode VLocalizationManager::GetLocalizationLanguage()
{
	return fCurrentDialectCode;
}


bool VLocalizationManager::InsertSTRSharpCodeAndString(const STRSharpCodes inSTRSharpCodeToAdd, VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	
	//We verify if we can overwrite an existent value
	STRSharpCodeAndStringMap::iterator sTRSharpMapIterator = fStringsRelativeToSTRSharpCodes.find(inSTRSharpCodeToAdd);
	
	if (!(sTRSharpMapIterator != fStringsRelativeToSTRSharpCodes.end() && !inShouldOverwriteExistentValue))
	{
		VString * stringToInsert = new VString(inLocalizedStringToAdd);
		
		std::pair<StringsSet::DeletableHashSetIterator, bool> resultOfInsert = fLocalizedStringsSet->fHashset->insert(stringToInsert);
		
		if (!resultOfInsert.second)
			delete stringToInsert;
		
		if (sTRSharpMapIterator != fStringsRelativeToSTRSharpCodes.end())
		{
			sTRSharpMapIterator->second = *(resultOfInsert.first);
		}
		else
		{
			fStringsRelativeToSTRSharpCodes.insert(STRSharpCodeAndStringMap::value_type(inSTRSharpCodeToAdd, *(resultOfInsert.first)));
		}
	}
	return true;
}

bool VLocalizationManager::InsertObjectURLAndString(const VString& inObjectURL, const VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	//We verify if we can overwrite an existent value
	OOSyntaxStringAndStringMap::iterator objectsMapIterator = fStringsRelativeToObjects.find(inObjectURL);
	
	if(!(objectsMapIterator != fStringsRelativeToObjects.end() && inShouldOverwriteExistentValue == false)){
		VString * stringToInsert = new VString(inLocalizedStringToAdd);
		std::pair<StringsSet::DeletableHashSetIterator, bool> resultOfInsert = fLocalizedStringsSet->fHashset->insert(stringToInsert);
		if(resultOfInsert.second == false)
			delete stringToInsert;
		
		if(objectsMapIterator != fStringsRelativeToObjects.end()){
			objectsMapIterator->second = *(resultOfInsert.first);
		}
		else{
			fStringsRelativeToObjects.insert(OOSyntaxStringAndStringMap::value_type(inObjectURL, *(resultOfInsert.first)));
		}
	}

	return true;
}

bool VLocalizationManager::InsertIDAndStringInAGroup(uLONG inID, const VString& inLocalizedString, const VString& inGroup, bool inShouldOverwriteExistentValue)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	
	//Find if the group is already inserted, if not insert it
	GroupToIDAndStringsMap::iterator groupsMapIterator = fStringsAndIDsRelativeToGroups.find(inGroup);
	if(groupsMapIterator == fStringsAndIDsRelativeToGroups.end()){
		std::map< uLONG, VString* > resnameToStringsMap;
		std::pair<GroupToIDAndStringsMap::iterator, bool> insertResult = fStringsAndIDsRelativeToGroups.insert(GroupToIDAndStringsMap::value_type(inGroup, resnameToStringsMap));
		groupsMapIterator = insertResult.first;
	}
	
	VString * stringToInsert = new VString(inLocalizedString);
	std::pair<StringsSet::DeletableHashSetIterator, bool> resultOfInsert = fLocalizedStringsSet->fHashset->insert(stringToInsert);
	if(resultOfInsert.second == false){
		//The string already exists, we can use it
		delete stringToInsert;
		groupsMapIterator->second[inID] = *(resultOfInsert.first);
	}
	else
		groupsMapIterator->second[inID] = stringToInsert;

	return true;
}

bool VLocalizationManager::InsertDotStringsKeyAndString(const VString& inDotStringsKey, const VString& inLocalizedStringToAdd, bool inShouldOverwriteExistentValue)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	//We verify if we can overwrite an existent value
	DotStringsAndStringsMap::iterator dotStringsMapIterator = fStringsRelativeToDotStrings.find(inDotStringsKey);
	
	if(!(dotStringsMapIterator != fStringsRelativeToDotStrings.end() && inShouldOverwriteExistentValue == false)){
		VString * stringToInsert = new VString(inLocalizedStringToAdd);
		std::pair<StringsSet::DeletableHashSetIterator, bool> resultOfInsert = fLocalizedStringsSet->fHashset->insert(stringToInsert);
		if(resultOfInsert.second == false)
			delete stringToInsert;
		
		if(dotStringsMapIterator != fStringsRelativeToDotStrings.end()){
			dotStringsMapIterator->second = *(resultOfInsert.first);
		}
		else{
			fStringsRelativeToDotStrings.insert(DotStringsAndStringsMap::value_type(inDotStringsKey, *(resultOfInsert.first)));
		}
	}

	return true;
}


void VLocalizationManager::InsertGroupBag( const VString& inGroupResname, const VString& inGroupRestype, const VValueBag *inBag)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);

	#if 0
	VString dump;
	inBag->DumpXML( dump, "xml", true);
	DebugMsg( dump);
	#endif
	
	if (!inGroupResname.IsEmpty())
	{
		xbox_assert( fGroupBagsByResname.find( inGroupResname) == fGroupBagsByResname.end());
		fGroupBagsByResname.insert( MapOfBagByName::value_type( inGroupResname, inBag));
	}

	fGroupBagsByRestype.insert( MapOfBagByRestype::value_type( inGroupRestype, inBag));
}


const VValueBag *VLocalizationManager::RetainGroupBag( const VString& inGroupResname)
{
	VTaskLock fReadWriteLocker(&fReadWriteCriticalSection);
	
	MapOfBagByName::iterator i = fGroupBagsByResname.find( inGroupResname);
	return (i == fGroupBagsByResname.end()) ? NULL : i->second.Retain();
}


void VLocalizationManager::RetainGroupBagsByRestype( const VString& inResType, std::vector<VRefPtr<const VValueBag> >& outBags)
{
	std::pair<MapOfBagByRestype::iterator,MapOfBagByRestype::iterator> range = fGroupBagsByRestype.equal_range( inResType);
	for( MapOfBagByRestype::iterator i = range.first ; i != range.second ; ++i)
		outBags.push_back( i->second);
}

#pragma mark Protected

VError VLocalizationManager::AnalyzeXLIFFFile(VFile* inFileToAnalyze, bool inForceLoading)
{
	fSAXHandler->SetAvoidLanguageChecking(inForceLoading);
	fSAXParser->Parse( const_cast<VFile*>( inFileToAnalyze), fSAXHandler, XML_ValidateNever);
	fSAXHandler->SetAvoidLanguageChecking(false);
	return VE_OK;
}

VError VLocalizationManager::AnalyzeDotStringsFile(VFile* inFileToAnalyze)
{
	if(!testAssert(inFileToAnalyze != NULL))
		return VE_INVALID_PARAMETER;
	if(!inFileToAnalyze->Exists())
		return VE_FILE_NOT_FOUND;
	
	VFileStream dotStringsFileStream = VFileStream (inFileToAnalyze);
	VError streamError;

	//Open the stream
	if(VE_OK != (streamError = dotStringsFileStream.OpenReading()))
		return streamError;

	//The char set _should_ be UTF 16 Big Endian
	if(VE_OK != (streamError = dotStringsFileStream.GuessCharSetFromLeadingBytes (VTC_UTF_16_BIGENDIAN)))
		return streamError;

	dotStringsFileStream.SetCarriageReturnMode(eCRM_LF);

	//Read line by line
	VString dotStringsTextLine;
	while(VE_OK == dotStringsFileStream.GetTextLine(dotStringsTextLine, false)){
		if(dotStringsTextLine.GetLength() <= 0)
			continue;
		//Remove the  trailing ;
		dotStringsTextLine.Remove(dotStringsTextLine.GetLength(), 1);
		std::vector< VString > dotStringsLineItems;
		VString equalSign = VString(CHAR_EQUALS_SIGN);
		dotStringsTextLine.GetSubStrings(equalSign, dotStringsLineItems); //The limit of that is we can't put = char in the strings...
		//We divide the line in key/value
		if(dotStringsLineItems.size() == 2){
			VString rawKey = dotStringsLineItems[0];
			VString rawValue = dotStringsLineItems[1];
			if(rawKey.GetLength() > 0 && rawValue.GetLength() > 0){
				//Delete the surrounding white spaces
				rawKey.RemoveWhiteSpaces();
				rawValue.RemoveWhiteSpaces();
				//Remove the surrounding quotes 
				if(rawKey[0] == CHAR_QUOTATION_MARK){
					rawKey.Remove(1, 1);
					rawKey.Remove(rawKey.GetLength(), 1);
				}
				if(rawValue[0] == CHAR_QUOTATION_MARK){
					rawValue.Remove(1, 1);
					rawValue.Remove(rawValue.GetLength(), 1);
				}
				//Insert the pair
				if(rawKey.GetLength() > 0 && rawValue.GetLength() > 0){
					InsertDotStringsKeyAndString(rawKey, rawValue, true);
				}
			}
		}
	}	
	
	//Close the stream
	dotStringsFileStream.CloseReading();
	
	return VE_OK;
}


END_TOOLBOX_NAMESPACE



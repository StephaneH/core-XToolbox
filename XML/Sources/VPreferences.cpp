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

#include "XMLSaxParser.h"
#include "XMLForBag.h"
#include "VPreferences.h"

BEGIN_TOOLBOX_NAMESPACE

static const VString	kDUMP_XML_PREFIX("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
static const VString	kDUMP_XML_MAIN_ELEMENT("preferences");
static const char *		kSTAMP_ATTRIBUTE("stamp");

VPreferences::VPreferences( VValueBag *inBag, IPreferencesDelegate *inDelegate, VPreferences *inDefaultPreferences)
: fBag( RetainRefCountable( inBag))
, fDirty( false)
, fStamp(0)
, fSignalPrefsHaveChanged(VSignal::ESM_Asynchonous)
, fDelegate( RetainRefCountable( inDelegate))
, fDefaultPreferences( RetainRefCountable( inDefaultPreferences))
{
	xbox_assert( (inBag != NULL) || (inDelegate != NULL) );	// a delegate is required to read bag

	if (fBag == NULL)
	{
		fBag = fDelegate->DoReadPreferences();
		if (fBag == NULL)
			fBag = new VValueBag;
	}

	if (fBag != NULL)
	{
		if (fBag->GetLong(kSTAMP_ATTRIBUTE, fStamp))
		{
			if(fStamp > (kMAX_sLONG - 1000))
			{
				fStamp = 1;
				fBag->SetLong(kSTAMP_ATTRIBUTE, fStamp);
			}
		}
		else
		{
			fStamp = 1;
			fBag->SetLong(kSTAMP_ATTRIBUTE, fStamp);
		}
	}
}


VPreferences::~VPreferences()
{
	ReleaseRefCountable( &fBag);
	ReleaseRefCountable( &fDefaultPreferences);
	ReleaseRefCountable( &fDelegate);
}


/*
	static
*/
VPreferences* VPreferences::Create( IPreferencesDelegate *inDelegate, VPreferences *inDefaultPreferences)
{
	return new VPreferences( NULL, inDelegate, inDefaultPreferences);
}

			
/*
	static
*/
VPreferences* VPreferences::Create( VFile *inFile, VFile *inDefaultPrefsFile /*, const VString *inXmlMainElement*/)
{
	VString xmlMainElement( kDUMP_XML_MAIN_ELEMENT);
//	if ( (inXmlMainElement != NULL) && !inXmlMainElement->IsEmpty())
//		xmlMainElement = *inXmlMainElement;
	
	VFilePreferencesDelegate *delegate = new VFilePreferencesDelegate( inFile, xmlMainElement);
	
	VPreferences *preferences = Create( delegate, inDefaultPrefsFile);
	
	ReleaseRefCountable( &delegate);
	
	return preferences;
}


/*
	static

*/
VPreferences *VPreferences::Create( const VString& inPreferencesFileName, const VString& inPreferencesFileExtension)
{
	VFile *file = RetainPreferencesFile( inPreferencesFileName, inPreferencesFileExtension, true);
	VFile *defaultValuesFile = RetainDefaultPreferencesFile( inPreferencesFileExtension);

	VPreferences *preferences;
	if (testAssert( file != NULL))
	{
		preferences = VPreferences::Create( file, defaultValuesFile);
	}
	else
	{
		// allocate a detached VPreferences to degrade gracefully
		preferences = VPreferences::Create();
	}

	ReleaseRefCountable( &file);
	ReleaseRefCountable( &defaultValuesFile);
	
	return preferences;
}


/*
 static
*/
VPreferences* VPreferences::Create( IPreferencesDelegate *inDelegate, VFile *inDefaultPrefsFile)
{
	VPreferences *preferences = NULL;
	
	if (testAssert(inDelegate != NULL))
	{		
		VPreferences *defaultPreferences = NULL;
		if (inDefaultPrefsFile != NULL)
		{
			VValueBag *defaultPrefsBag = VFilePreferencesDelegate::ReadFile( inDefaultPrefsFile, kDUMP_XML_MAIN_ELEMENT);
			if (defaultPrefsBag != NULL)
				defaultPreferences = new VPreferences( defaultPrefsBag, NULL, NULL);
		}

		preferences = new VPreferences( NULL, inDelegate, defaultPreferences);
		
		ReleaseRefCountable( &defaultPreferences);
	}
	return preferences;
}

			
/*
	static
*/
VPreferences *VPreferences::Create( const VValueBag &inSourceBag)
{
	VValueBag *bag = inSourceBag.Clone();
	
	VPreferences *preferences = (bag == NULL) ? NULL : new VPreferences( bag, NULL, NULL);
	
	ReleaseRefCountable( &bag);
	
	return preferences;
}

		
/*
	static
*/
VPreferences *VPreferences::Create()
{
	VValueBag *bag = new VValueBag;
	VPreferences *preferences = new VPreferences( bag, NULL, NULL);
	ReleaseRefCountable( &bag);

	return preferences;
}


/*
	static
*/
XBOX::VFile *VPreferences::RetainPreferencesFile( const VString& inPreferencesFileName, const VString& inPreferencesFileExtension, bool inCreateFolderIfNotFound)
{
	VFile *file = NULL;

	VString name( inPreferencesFileName);
	if (name.IsEmpty())
	{
		name = "Preferences.";
		name += inPreferencesFileExtension;
	}

	// create preferences in user folder
	VFolder *folder = VProcess::Get()->RetainProductSystemFolder( eFK_UserPreferences, true);
	if (testAssert( folder != NULL))
	{
		file = new VFile( *folder, name);
	}
	ReleaseRefCountable( &folder);
	return file;
}


/*
	static
*/
XBOX::VFile *VPreferences::RetainDefaultPreferencesFile( const VString& inPreferencesFileExtension)
{
	// locate default values file in application Resources folder
	XBOX::VString defaultValuesName("default.");
	defaultValuesName += inPreferencesFileExtension;
	VFolder *resourcesFolder = VProcess::Get()->RetainFolder( VProcess::eFS_Resources);
	VFile *defaultValuesFile = new XBOX::VFile( *resourcesFolder, defaultValuesName);
	if ( (defaultValuesFile != NULL) && !defaultValuesFile->Exists())
		ReleaseRefCountable( &defaultValuesFile);
	
	return defaultValuesFile;
}


void VPreferences::DumpXML( VString& ioDump, bool inWithIndentation) const
{
	VTaskLock lock( &fMutex);

	ioDump.Clear();

	if (fBag != NULL)
	{
		ioDump = kDUMP_XML_PREFIX;
		fBag->DumpXML( ioDump, kDUMP_XML_MAIN_ELEMENT, inWithIndentation );
	}
}


void VPreferences::SetBag( VValueBag *inBag)
{
	if (testAssert( inBag != NULL))
	{
		VTaskLock lock( &fMutex);

		CopyRefCountable( &fBag, inBag);

		fDirty = true;
	}
}


void VPreferences::_UpdateBagIfNecessary() const
{
	if (fDelegate != NULL)
	{
		VValueBag *bag = fDelegate->DoReadPreferencesIfNecessary();
		if (bag != NULL)
		{
			if (fBag != NULL)
				const_cast<VPreferences*>( this)->fBag->Release();
			const_cast<VPreferences*>( this)->fBag = bag;
		}
	}
}


const VValueBag* VPreferences::RetainBag( bool inUpdateIfNecessary) const
{
	VTaskLock lock( &fMutex);
	
	if (inUpdateIfNecessary)
		_UpdateBagIfNecessary();
	
	return RetainRefCountable( fBag);
}


const VValueBag* VPreferences::RetainBag( const VString& inPath, bool inUpdateIfNecessary) const
{
	VValueBag::StKeyPath keys;
	VValueBag::GetKeysFromPath( inPath, keys);
	return RetainBag( keys, inUpdateIfNecessary);
}


const VValueBag* VPreferences::RetainBag( const VValueBag::StKeyPath& inPath, bool inUpdateIfNecessary) const
{
	VTaskLock lock( &fMutex);
	
	if (inUpdateIfNecessary)
		_UpdateBagIfNecessary();
	
	const VValueBag *bag = fBag->GetUniqueElementByPath( inPath);

	return RetainRefCountable( bag);
}


const VValueBag *VPreferences::RetainDefaultPrefsBag( const VString& inPath) const
{
	VValueBag::StKeyPath keys;
	VValueBag::GetKeysFromPath( inPath, keys);
	return RetainDefaultPrefsBag( keys);
}


const VValueBag *VPreferences::RetainDefaultPrefsBag( const VValueBag::StKeyPath& inPath) const
{
	const VValueBag *bag = NULL;
	
	if (fDefaultPreferences != NULL)
	{
		bag = fDefaultPreferences->RetainBag( inPath, false);
		if (bag == NULL)
			bag = fDefaultPreferences->RetainDefaultPrefsBag( inPath);
	}
	
	return bag;
}


bool VPreferences::ReplaceBag( const VValueBag::StKeyPath& inPath, VValueBag *inBag)
{
	VTaskLock lock( &fMutex);

	bool ok = fBag->ReplaceElementByPath( inPath, inBag);
	
	fDirty = true;
	return ok;
}


void VPreferences::AddElementForPath( const VValueBag::StKeyPath& inContainerPath, const VValueBag::StKey& inElementName, VValueBag *inElementBag)
{
	VTaskLock lock( &fMutex);

	if (testAssert(inElementBag != NULL))
	{
		VValueBag *mainPathBag = fBag->GetElementByPathAndCreateIfDontExists( inContainerPath);
		if (testAssert(mainPathBag != NULL))
		{
			mainPathBag->AddElement( inElementName, inElementBag);
		}
	}
	fDirty = true;
}


void VPreferences::AddOrReplaceSubElement( const VValueBag::StKeyPath& inContainerPath,
										  const VValueBag::StKey& inElementName,
										  const VValueBag::StKey& inAttributeName,
										  const VValueSingle& inAttributeValue,
										  VValueBag *inBag,
										  bool inForceAdd)
{
	VTaskLock lock( &fMutex);

	if(testAssert(inBag != NULL))
	{
		VValueBag *mainPathBag = fBag->GetElementByPathAndCreateIfDontExists( inContainerPath);

		if(testAssert(mainPathBag != NULL))
		{
			VBagArray* bagArray = mainPathBag->GetElements( inElementName);

			if (!inForceAdd && bagArray != NULL)
			{
				bool	found = false;
				VIndex	index_found = 0;
				VIndex	nb_bag = bagArray->GetCount();

				VValueSingle *value_same_type = inAttributeValue.Clone();
				if (value_same_type != NULL)
				{
					for (VIndex bag_index = 1; bag_index <= nb_bag; ++bag_index)
					{
						const VValueBag *bag = bagArray->RetainNth(bag_index);
						if (bag != NULL)
						{
							bag->GetAttribute(inAttributeName,*value_same_type);
							if (*value_same_type == inAttributeValue)
							{
								found = true;
								index_found = bag_index;
							}
							bag->Release();
						}

						if(found)
							break;
					}
					delete value_same_type;
				}

				if(found)
					bagArray->DeleteNth(index_found);
			}
			
			if(bagArray == NULL)
			{
				bagArray = new VBagArray;
				mainPathBag->SetElements(inElementName, bagArray);
				bagArray->Release();
			}
			bagArray->AddTail(inBag);
		}
	}

	fDirty = true;
}

void VPreferences::RemoveElementsForPath( const VValueBag::StKeyPath& inContainerPath, const VValueBag::StKey& inElementName)
{
	VTaskLock lock( &fMutex);

	VValueBag *mainPathBag = fBag->GetUniqueElementByPath( inContainerPath);

	if (mainPathBag != NULL)
	{
		mainPathBag->RemoveElements(inElementName);
	}

	fDirty = true;
}


void VPreferences::Flush( bool inForceWriting) const
{
	VTaskLock lock( &fMutex);
	
	if (fDirty || inForceWriting)
	{
		if (fDirty)
		{
			++const_cast<VPreferences*>( this)->fStamp;
			if (fBag != NULL)
				fBag->SetLong(kSTAMP_ATTRIBUTE, fStamp);
		}

		if (fDelegate != NULL)
		{
			if (testAssert(fBag != NULL))
				fDelegate->DoWritePreferences( *fBag);
		}
	}
			
	if (fDirty)
		TellPrefsHaveChanged();
	
	const_cast<VPreferences*>( this)->fDirty = false;
}


void VPreferences::_CheckStampOverflow() const
{
	if (fBag != NULL)
	{
		fBag->GetLong(kSTAMP_ATTRIBUTE, const_cast<VPreferences*>( this)->fStamp);
		if (fStamp > (kMAX_sLONG - 1000))
		{
			const_cast<VPreferences*>( this)->fStamp = 1;
			fBag->SetLong(kSTAMP_ATTRIBUTE, fStamp);
		}
	}
}


void VPreferences::UpdateIfNecessary() const
{
	VTaskLock lock( &fMutex);
	
	_UpdateBagIfNecessary();
	_CheckStampOverflow();
}


void VPreferences::Update() const
{
	VTaskLock lock( &fMutex);
	
	if (fDelegate != NULL)
	{
		VValueBag *bag = fDelegate->DoReadPreferences();
		if (bag != NULL)
		{
			if (fBag != NULL)
				const_cast<VPreferences*>( this)->fBag->Release();
			const_cast<VPreferences*>( this)->fBag = bag;
		}
	}
	
	_CheckStampOverflow();
}


//======================================================================================================


VFilePreferencesDelegate::VFilePreferencesDelegate( XBOX::VFile *inXMLFile, const VString& inRootElementKind)
: fFile( RetainRefCountable( inXMLFile))
, fRootElementKind( inRootElementKind)
{
	xbox_assert( !inRootElementKind.IsEmpty());
}


VFilePreferencesDelegate::~VFilePreferencesDelegate()
{
	ReleaseRefCountable( &fFile);
}


bool VFilePreferencesDelegate::DoWritePreferences( const VValueBag& inPreferences)
{
	bool ok = WriteFile( fFile, fRootElementKind, &inPreferences);
	if (ok)
		fFile->GetTimeAttributes( &fStamp, NULL, NULL);
	return ok;
}


VValueBag *VFilePreferencesDelegate::DoReadPreferencesIfNecessary()
{
	VValueBag *bag = NULL;
	VTime time;
	if ( (fFile != NULL) && fFile->Exists() && (fFile->GetTimeAttributes( &time, NULL, NULL) == VE_OK) )
	{
		if (time > fStamp)
		{
			bag = DoReadPreferences();
			if (bag != NULL)
			{
				fStamp = time;
			}
		}
	}
	return bag;
}


VValueBag *VFilePreferencesDelegate::DoReadPreferences()
{
	return ReadFile( fFile, fRootElementKind);
}


/*
	static
*/
VValueBag *VFilePreferencesDelegate::ReadXML( const XBOX::VString& inXML)
{
	StErrorContextInstaller errorContext( false);

	VValueBag *bag = NULL;
	if (!inXML.IsEmpty())
	{
		VXMLParser parser;
		bag = new VValueBag;
		VXMLBagHandler_UniqueElement handler( bag, kDUMP_XML_MAIN_ELEMENT);
		bool ok = parser.Parse( inXML, &handler, XML_ValidateNever);
		if (!testAssert( ok))
			ReleaseRefCountable( &bag);
	}

	return bag;
}


/*
	static
*/
VValueBag *VFilePreferencesDelegate::ReadFile( VFile *inFile, const VString& inRootElementKind)
{
	if (!testAssert( inFile != NULL))
		return NULL;
	
	StErrorContextInstaller errorContext( false);

	VValueBag *bag = NULL;
	if (inFile->Exists())
	{
		VXMLParser parser;
		bag = new VValueBag;
		VXMLBagHandler_UniqueElement handler( bag, inRootElementKind);
		bool ok = parser.Parse( inFile, &handler, XML_ValidateNever);
		if (!testAssert( ok))
			ReleaseRefCountable( &bag);
	}

	return bag;
}

/*
	static
*/
bool VFilePreferencesDelegate::WriteFile( VFile *inFile, const VString& inXmlMainElement, const VValueBag *inBag)
{
	if ( (inFile == NULL) || (inBag == NULL) )
		return false;

	StErrorContextInstaller errorContext( false);

	// make sure parent folder exists
	VFolder *folder = inFile->RetainParentFolder();
	folder->CreateRecursive();
	ReleaseRefCountable( &folder);

	VError err = WriteBagToFileInXML( *inBag, inXmlMainElement, inFile, false /* inWithIndentation */);
	
	return err == VE_OK;
}




#if 0
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeByte, VByte, sBYTE, 0);
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeWord, VWord, sWORD, 0);
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeLong, VLong, sLONG, 0);
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeLong8, VLong8, sLONG8, 0);
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeReal, VReal, Real, 0.0);
CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/db4d", zeBool, VBoolean, bool, false);
CREATE_PATHBAGKEY_WITH_DEFAULT( "4d/db4d", zeUUID, VUUID, VUUID::sNullUUID);
CREATE_PATHBAGKEY_WITH_DEFAULT( "4d/db4d", zeString, VString, "none");
CREATE_PATHBAGKEY_WITH_DEFAULT( "4d/db4d", zeTime, VTime, VTime(0));

static void test()
{
	VPreferences *pref = NULL;
	
	pref->SetValue( zeByte, (sBYTE) 1);
	sBYTE b = pref->GetValue( zeByte);
	
	pref->SetValue( zeWord, (sWORD) 1);
	sWORD w = pref->GetValue( zeWord);
	
	pref->SetValue( zeLong, (sLONG) 1);
	sLONG l = pref->GetValue( zeLong);
	
	pref->SetValue( zeLong8, (sLONG8) 1);
	sLONG8 l8 = pref->GetValue( zeLong8);
	
	pref->SetValue( zeReal, 1.1);
	Real r = pref->GetValue( zeReal);
	
	pref->SetValue( zeBool, true, true);
	bool bo = pref->GetValue( zeBool);

	pref->SetValue( zeUUID, VUUID::sNullUUID);
	VUUID uid = pref->GetValue( zeUUID);

	VString str;
	VString	hop(L"coucou");
	pref->SetValue( zeString, CVSTR("truc"));
	pref->SetValue( zeString, hop, true);
	pref->GetValue( zeString, str);
	
	VTime t;
	VTime::Now(t);
	pref->SetValue( zeTime, t);
	pref->GetValue( zeTime, t);
}
#endif


END_TOOLBOX_NAMESPACE


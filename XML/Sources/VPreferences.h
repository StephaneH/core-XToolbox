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
#ifndef __VPreferences__
#define __VPreferences__

BEGIN_TOOLBOX_NAMESPACE

/*	Preferences are stored as xml using VValueBag as intermediate.
		
	To access values, you can use VPreferences::RetainBag that gives you access to internal bag,
	or you can, preferably, use bag keys in following ways:
	
	namespace Pref4DBackup
	{
		CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( "4d/backup", automatic, VBoolean, bool, false);
	};
	
	namespace Pref4D
	{
		CREATE_PATHBAGKEY_WITH_DEFAULT( "4d", gadget_uuid, VUUID, VUUID::sNullUUID);
		CREATE_PATHBAGKEY_WITH_DEFAULT( "4d", default_user, VString, "none");
	};

	void UsePrefs( VPreferences *inPreferences)
	{
		// reload prefs cache if the pref file has changed
		inPreferences->Update();
		
		bool value = inPreferences->GetValue( Pref4DBackup::automatic);
		inPreferences->SetValue( Pref4DBackup::automatic, !value);

		VUUID gadgetUUID = inPreferences->GetValue( Pref4D::gadget_uuid);
		pref->SetValue( Pref4D::gadget_uuid, VUUID::sNullUUID);

		VString s;
		inPreferences->SetValue( Pref4D::default_user, "truc");
		bool foundPref = inPreferences->GetValue( Pref4D::default_user, s);

		// flush prefs to disk
		inPreferences->Flush();
	}
 
	When a key is not in the bag, GetValue() returns the default value stored in the DefaultValues bag, if used (not mandatory)	
 

	If you want to be signaled when the prefs are modified:
		1/ Connect to the VSignal_PrefsHadChanged. It's a VSignalT_1 that uses a VRefPtr<VPreferences>
			XBOX::VPreferences *prefs = ...get the appropriate prefs
			prefs->GetSignalPrefsHadChanged()->Connect(this, aValidTask, &TheClass::Signaled_PrefsHaveChanged);

		2/ Handle this signal. It's a VSignalT_1, that sends the preferencs themselves:
			TheClass::Signaled_PrefsHaveChanged(XBOX::VRefPtr<VPreferences> inPrefs)
			{
				MyDocument::SetDisplayThis(inPrefs->GetValue(MyPrefsFamily::can_do_that));
				MyDocument::UpdateAllOpenDocs();
			}

	   (3/ Disconnect when necessary)
	
 */

 /* @brief	IPreferencesDelegate
	
	An interface class be used when:
		-> you use something else than a VFile to store your prefs
		-> You want to modify the place where VPreferences must read a default value.
*/
class XTOOLBOX_API IPreferencesDelegate : public IRefCountable
{
public:
	/** @brief	Delegate for VPreferences::Flush()
				The mutex is locked before calling the delegate.
	 */
	virtual	bool				DoWritePreferences( const VValueBag& inPreferences) = 0;
	
	/** @brief	Delegate for VPreferences::Update()
				The mutex is locked before calling the delegate.
	 */
	virtual	VValueBag*			DoReadPreferences() = 0;
	virtual	VValueBag*			DoReadPreferencesIfNecessary() = 0;
};


class XTOOLBOX_API VFilePreferencesDelegate : public VObject, public IPreferencesDelegate
{
public:
								VFilePreferencesDelegate( XBOX::VFile *inFile, const VString& inRootElementKind);

	virtual	bool				DoWritePreferences( const VValueBag& inPreferences);
	virtual	VValueBag*			DoReadPreferences();
	virtual	VValueBag*			DoReadPreferencesIfNecessary();

	static	VValueBag*			ReadXML( const XBOX::VString& inXML);
	static	VValueBag*			ReadFile( VFile *inFile, const VString& inXmlMainElement);
	static	bool				WriteFile( VFile *inFile, const VString& inXmlMainElement, const VValueBag *inBag);

private:
								~VFilePreferencesDelegate();
	
			VFile*				fFile;
			VTime				fStamp;		// the modification time of the file at the time the bag was loaded
			VString				fRootElementKind;
};


class XTOOLBOX_API VPreferences : public VObject, public IRefCountable
{
	typedef VSignalT_1< VRefPtr<const VPreferences> >		VSignal_PrefsHaveChanged;
public:

			/**	@brief	custom preferences file. To be associated with default values stored on disk
						if inXmlMainElement is NULL or empty, "preferences" is used as default xml main element name */
	static	VPreferences*		Create( VFile *inFile, VFile *inDefaultPrefsFile);

			/** @brief	create a preferences file in user preferences system folder **/
			/** default file is default.extension in resource folder **/
	static	VPreferences*		Create( const VString& inPreferencesFileName, const VString& inPreferencesFileExtension);
	
			/**	@brief	preference storage is handled by the delegate that fills/flush the bag.
						if inXmlMainElement is NULL or empty, "preferences" is used as default xml main element name */
	static	VPreferences*		Create( IPreferencesDelegate *inDelegate, VFile *inDefaultPrefsFile);

	static	VPreferences*		Create( IPreferencesDelegate *inDelegate, VPreferences *inDefaultPreferences);

			/** @brief	To be used for for const preferences.
						NOTE: inBag is duplicated (cloned), not retained.
						preferences are not saved by "me".
			*/
	static	VPreferences*		Create( const VValueBag& inBag);

			/**	@brief	preference file wihout backstore, without delegate and without references to default values.
						preferences won't ever be saved.
			*/
	static	VPreferences*		Create();

			/**	@brief	returns the root preferences bag.
						pass inUpdateIfNecessary = true if you want to reload the bag in the case the pref file has changed.
			*/
			const VValueBag*	RetainBag( bool inUpdateIfNecessary) const;

			/**	@brief	returns the part of the preferences hierarchy corresponding to specified path.
						the path contains a list of element names separated by '/'.
						ex: bag = RetainBag( "4D/backup", false) returns first element "backup" of first element 4D of root.
						pass inUpdateIfNecessary = true if you want to reload the bag in the case the pref file has changed.
			*/
			const VValueBag*	RetainBag( const VString& inPath, bool inUpdateIfNecessary) const;
			const VValueBag*	RetainBag( const VValueBag::StKeyPath& inPath, bool inUpdateIfNecessary) const;
	
			// replace all elements at path with given bag.
			// Given bag will be retained. You should stop using it after passing it.
			// You canpass NULL bag to remove it.
			bool				ReplaceBag( const VValueBag::StKeyPath& inPath, VValueBag *inBag);


			/**	@brief	facility to scan the default preferences chain looking for a particular element.
						May be NULL
			*/
			const VValueBag*	RetainDefaultPrefsBag( const VString& inPath) const;
			const VValueBag*	RetainDefaultPrefsBag( const VValueBag::StKeyPath& inPath) const;

			/**	@brief	Flush writes the bag on disk if:
							- SetValue() has been called since last Flush.
							- and a storage file is used
							- Special: if inForceWriting is true, the preferences are flushed, the file is created if it did'nt exist, etc.,
										whatever the dirty flag.
						
						If a delegate is used, nothing is written here, but IPreferencesDelegate::DoWritePreferences() is called instead.
			 
						Once data flushed, TellPrefsHaveChanged() signal is triggered (only if the dirtyflag is set).
						At the end of the call, the dirty flag is reset.
			*/
			void				Flush(bool inForceWriting = false) const;
			
			/**	@brief	Reload the preferences from file if its modification date has changed
						If a delegate is used, IPreferencesDelegate::DoReadPreferencesIfNecessary() is called instead.
			*/
			void				UpdateIfNecessary() const;

			/**	@brief	Update the preferences from file
						If a delegate is used, IPreferencesDelegate::DoReadPreferences() is called instead.
			*/
			void				Update() const;


			/**	@brief	Signaling something has changed */
			VSignal_PrefsHaveChanged * GetSignalPrefsHaveChanged() const	{ return &const_cast<VPreferences*>( this)->fSignalPrefsHaveChanged; }
			void				TellPrefsHaveChanged() const				{ const_cast<VPreferences*>( this)->fSignalPrefsHaveChanged(this); }

			/** @brief	When you need to access several values that are linked one to each other, it could be
						safer to ensure that the VPreferences you're using are not modified between two calls:
							prefs->GetValue(an ip address);
							prefs->GetValue(a port number);
							. . .
							prefs->GetValue(use ssl or not);

						In this situation, lock the preferences before getting the values, and don't forget to unlock them
							prefs->Lock();
								prefs->GetValue(an ip address);
								prefs->GetValue(a port number);
								. . .
								prefs->GetValue(use ssl or not);
							prefs->Unlock();

						Note: one can also use the VPreferencesLocker object
			*/
			void				Lock() const			{ fMutex.Lock(); }
			void				Unlock() const			{ fMutex.Unlock(); }

			template<class T, class V>
			bool				HasAttribute(const StBagKeyWithDefault<T,V>& inKey, bool inOnlyInMainBag = false) const
								{
									VTaskLock lock( &fMutex);

									if (inKey.HasAttributeUsingPath(fBag))
										return true;
									
									if (!inOnlyInMainBag && (fDefaultPreferences != NULL) )
										return fDefaultPreferences->HasAttribute( inKey, false);

									return false;
								}

			/**	@brief	Get an attribute from an element in the preferences.
						The element path, the attribute name and the default value are specified in the StBagKeyWithDefault object.
						If the element is not found, the default value (found in fDefaultPrefsBag or returned by the delegate) is returned.
			*/
			template<class T, class V>
			T					GetValue( const StBagKeyWithDefault<T,V>& inKey) const
                                {
                                    VTaskLock lock( &fMutex);

									bool	found;

									T	theValue = inKey.GetUsingPath(fBag, &found);
									
									if (!found && (fDefaultPreferences != NULL) )
									{
										theValue = fDefaultPreferences->GetValue( inKey);
									}

									return theValue;
                                }

			/** @brief	Get an attribute from an element in the preferences.
						The element path, the attribute name and the default value are specified in the StBagKeyWithDefault object.
						If the attribute is found, returns true
						Else if the attribute is not found:
							-> If it is found in fDefaultPrefsBag or returned by the delegate, the routine returns true
							-> Else, returns false
			*/
			template<class T, class V>
			bool				GetValue( const StBagKeyWithDefault<T,V>& inKey, V& outValue) const
								{
									VTaskLock lock( &fMutex);
									
									bool	found = inKey.GetUsingPath( fBag, outValue);
										
									if (!found && (fDefaultPreferences != NULL) )
									{
										found = fDefaultPreferences->GetValue( inKey, outValue);
									}
									
									return found;
								}
	
			/** @brief	Set an attribute of an element in the preferences only if the value is not the default value
						The element path, the attribute name and the default value are specified in the StBagKeyWithDefault object.
						The needed elements are created if inOnlyIfNotDefault is not passed of false. Else, they are created only if inValue != default value.
						You need to call Flush to write the preferences.
			*/
			template<class T, class V>
			void				SetValue( const StBagKeyWithDefault<T,V>& inKey, const T& inValue, bool inOnlyIfNotDefault = false)
								{
									VTaskLock lock( &fMutex);

									if (inOnlyIfNotDefault && (fDefaultPreferences != NULL) )
									{
										T	theDefault = fDefaultPreferences->GetValue( inKey);
										if (theDefault != inValue)
										{
											inKey.SetForcedUsingPath( fBag, inValue);
											fDirty = true;
										}
									}
									else
									{
										inKey.SetForcedUsingPath( fBag, inValue);
										fDirty = true;
									}
								}

			/** @brief	Add an element in a subbag.
						For example, say the preferences xml is as follows:
								. . .
								<com.4d>
									<windows>
										<window name="ObjectLib" . . ./>
										<window name="FormEdit" . . ./>
										. . .
									</windows>
									. . .
								</com.4d>
						
						With AddOrReplaceSubElement(), you can add or modify a <window .../>

						WARNINGS
							- inBag must not be NULL
							- inBag is retained by the routine, so it can (should) be released by the caller when needed.
							- inPath + inElementName + inAttributeName must be unique for inAttributeValue. If it's not the case,
							  the routine updates the first found item
			*/
			template<class T, class V>
			void				AddOrReplaceSubElement( const StBagKeyWithDefault<T,V>& inContainerPath, const VValueBag::StKey& inAttributeName, const VValueSingle& inAttributeValue, VValueBag *inBag, bool inForceAdd = false)
								{ AddOrReplaceSubElement( inContainerPath.GetPath(), inContainerPath.GetKey(), inAttributeName, inAttributeValue, inBag, inForceAdd);}

			void				AddOrReplaceSubElement( const VValueBag::StKeyPath& inContainerPath, const VValueBag::StKey& inElementName, const VValueBag::StKey& inAttributeName, const VValueSingle& inAttributeValue, VValueBag *inBag, bool inForceAdd = false);
			

			// Add a new element in element specified by path (will be created if needed)
			template<class T, class V>
			void				AddElementForPath( const StBagKeyWithDefault<T,V>& inContainerPath, VValueBag *inElementBag)
								{ AddElementForPath( inContainerPath.GetPath(), inContainerPath.GetKey(), inElementBag);}

			void				AddElementForPath( const VValueBag::StKeyPath& inContainerPath, const VValueBag::StKey& inElementName, VValueBag *inElementBag);

			// Delete all the elements in element specified by path
			template<class T, class V>
			void				RemoveElementsForPath( const StBagKeyWithDefault<T,V>& inContainerPath)
								{ RemoveElementsForPath( inContainerPath.GetPath(), inContainerPath.GetKey());}

			void				RemoveElementsForPath( const VValueBag::StKeyPath& inContainerPath, const VValueBag::StKey& inElementName);

			/** @brief	Dirty flag accessor*/
			bool				IsDirty() const							{ return fDirty; }

			/** @brief	Export all the bag as xml, including required header ("<?xml version... etc>")*/
			void				DumpXML( VString& ioDump, bool inWithIndentation) const;

			/** @brief	Replace the settings bag. inBag must not be NULL and will be retained.
						The dirty flag is always set after the call, and the bag is not Flush().
			*/
			void				SetBag( VValueBag *inBag);

			/** @brief	Return the stamp of the last flush(), incremented if the prefs was dirty*/
			sLONG				GetStamp() const						{ return fStamp; }

			const XBOX::VPreferences*	GetDefaultPreferences() const	{ return fDefaultPreferences;}
			
	static	XBOX::VFile*		RetainPreferencesFile( const VString& inPreferencesFileName, const VString& inPreferencesFileExtension, bool inCreateFolderIfNotFound);
	static	XBOX::VFile*		RetainDefaultPreferencesFile( const VString& inPreferencesFileExtension);

private:
			/** @brief	inFile may be null. In this case, inTime is ignored */
								VPreferences( VValueBag *inBag, IPreferencesDelegate *inDelegate, VPreferences *inDefaultPreferences);
								~VPreferences();
			/** @brief	We don't want user of this interface to duplicate VPreferences (or they need to code it...). So, thos constructors are declared but not coded */
								VPreferences( const VPreferences&);
								VPreferences& operator=( const VPreferences&);
			void				_UpdateBagIfNecessary() const;
			void				_CheckStampOverflow() const;
			
			VValueBag*					fBag;
			VPreferences*				fDefaultPreferences;
			IPreferencesDelegate*		fDelegate;
			bool						fDirty;	// the bag needs to be saved on disk
			sLONG						fStamp; // Each time Flush() is called and the fDirty is true, the stamp is incremented
	mutable	VCriticalSection			fMutex;
			VSignal_PrefsHaveChanged	fSignalPrefsHaveChanged;
};

/* @brief	basic utility class for locking/unlocking preferences */
class XTOOLBOX_API VPreferencesLocker
{
public:
	VPreferencesLocker( const VPreferences *inPrefs) : fPrefs(inPrefs)
	{
		if (testAssert(fPrefs != NULL))
			fPrefs->Lock();
	}
	~VPreferencesLocker()
	{
		if (fPrefs != NULL)
			fPrefs->Unlock();
	}

private:
	const VPreferences*		fPrefs;
};

END_TOOLBOX_NAMESPACE

#endif
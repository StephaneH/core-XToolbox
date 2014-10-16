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
#ifndef __VJS_WEB_STORAGE__
#define __VJS_WEB_STORAGE__

#include "VJSClass.h"
#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSStructuredClone.h"

// Web Storage specification (http://www.w3.org/TR/webstorage/) implementation.
// Only sessionStorage attribute is implemented (no localStorage).

BEGIN_TOOLBOX_NAMESPACE

// Storage object, subclass it to implement sessionStorage or localStorage attributes.

class XTOOLBOX_API VJSStorageObject : public XBOX::VObject, public XBOX::IRefCountable	
{
public:

	virtual sLONG	NumberKeysValues () = 0;
	
	virtual bool	HasKey (const XBOX::VString &inKey) = 0;
	virtual void	GetKeyValue (const XBOX::VString &inKey, XBOX::VJSValue *outValue) = 0;
	virtual void	GetKeyValue (sLONG inIndex, XBOX::VJSValue *outValue) = 0;
	virtual void	GetKeyFromIndex (sLONG inIndex, XBOX::VString *outKey) = 0;

	virtual void	SetKeyValue (const XBOX::VString &inKey, const XBOX::VJSValue& inValue) = 0;

	virtual bool	RemoveKeyValue (const XBOX::VString &inKey) = 0;
	virtual void	Clear () = 0;

	virtual void	GetKeys (XBOX::VJSParms_getPropertyNames &ioParms) = 0;

	// Return true if lock is successful. TryLock() and Lock() can be called recursively, Unlock() 
	// must then be matched. If lock hasn't been acquired, Unlock() will do nothing.
	
	virtual bool	TryLock () = 0;
	virtual void	Lock () = 0;
	virtual void	Unlock () = 0;

	// Force unlock at termination.

	virtual void	ForceUnlock () = 0;
};

// Object to implement the sessionStorage attribute.

class XTOOLBOX_API VJSSessionStorageObject : public VJSStorageObject
{
public:

					VJSSessionStorageObject ();
	virtual			~VJSSessionStorageObject ();
	
	virtual sLONG	NumberKeysValues ();
	
	virtual bool	HasKey (const XBOX::VString &inKey);
	virtual void	GetKeyValue (const XBOX::VString &inKey, XBOX::VJSValue *outValue);
	virtual void	GetKeyValue (sLONG inIndex, XBOX::VJSValue *outValue);
	virtual void	GetKeyFromIndex (sLONG inIndex, XBOX::VString *outKey);

	virtual void	SetKeyValue (const XBOX::VString &inKey, const XBOX::VJSValue& inValue);

	virtual bool	RemoveKeyValue (const XBOX::VString &inKey);
	virtual void	Clear ();

	virtual void	GetKeys (XBOX::VJSParms_getPropertyNames &ioParms);

	virtual bool	TryLock ();
	virtual void	Lock ();
	virtual void	Unlock ();

	virtual void	ForceUnlock ();

			void	SetKeyVValueSingle( const XBOX::VString &inKey,  const XBOX::VValueSingle& inValue);

			/*
				SetKeyVBagArray() behaviour

				- each bag of the array will be an array item in JavaScript and can be also added as a property using the "____property_name_in_jsarray" private attribute
				- example:	- have a bag array which contains one bag with two attributes: "name" is "userSettings" and "____property_name_in_jsarray" is "userSettings"
							- append the array to the storage: storage.SetKeyVBagArray( L"settings", bagArray)
							- then, in JavaScript, we can write:

									var array = storage.getItem( 'settings');
									var result = array[0].name; => result is 'userSettings'
									result = array.userSettings.name; => result is 'userSettings' because array.userSettings and array[0] are same object
			*/
			void	SetKeyVBagArray( const XBOX::VString &inKey, const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays = true);

			void	SetKeyVValueBag( const XBOX::VString &inKey, const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays = true);

			void	FillWithVValueBag( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays = true);
	
private:

	typedef unordered_map_VString<VRefPtr<VJSStructuredClone> >	SMap;
	
	XBOX::VCriticalSection	fMutex;
	SMap					fKeysValues;
};

// Storage interface implementation (see section 4.1 "The Storage interface" of specification).

class XTOOLBOX_API VJSStorageClass : public XBOX::VJSClass<VJSStorageClass, VJSStorageObject>
{
public:

	static const uLONG	kSpecificKey	= ('J' << 24) | ('S' << 16) | ('S' << 8) | 'S'; 

	static void			GetDefinition (ClassDefinition &outDefinition);

private:

	enum {
		
		kNumberMethods	= 8,

	};

	static const char	*kMethodNames[kNumberMethods];

	typedef XBOX::VJSClass<VJSStorageClass, VJSSessionStorageObject> inherited;

	static	void		_Initialize( const XBOX::VJSParms_initialize& inParms, VJSStorageObject *inStorageObject);
	static	void		_Finalize( const XBOX::VJSParms_finalize& inParms, VJSStorageObject *inStorageObject);
	static void			_HasProperty (XBOX::VJSParms_hasProperty &ioParms, VJSStorageObject *inStorageObject);
	static void			_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSStorageObject *inStorageObject);
	static bool			_SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSStorageObject *inStorageObject);
	static void			_GetPropertyNames (XBOX::VJSParms_getPropertyNames &ioParms, VJSStorageObject *inStorageObject);
	static void			_DeleteProperty (XBOX::VJSParms_deleteProperty &ioParms, VJSStorageObject *inStorageObject);
	
	static void			_key (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_getItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_setItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_removeItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_clear (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);

	// Add-ons, not in specification.

	static void			_tryLock (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_lock (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
	static void			_unlock(VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject);
};

END_TOOLBOX_NAMESPACE

#endif
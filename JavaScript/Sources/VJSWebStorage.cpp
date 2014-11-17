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
#include "VJavaScriptPrecompiled.h"

#include "VJSWebStorage.h"

#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSSessionStorageObject::VJSSessionStorageObject ()
{
}

VJSSessionStorageObject::~VJSSessionStorageObject ()
{
	xbox_assert(fMutex.GetOwnerTaskID() == XBOX::NULL_TASK_ID);
	
	Clear();
}

sLONG VJSSessionStorageObject::NumberKeysValues () 
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	return (sLONG) fKeysValues.size();
}

bool VJSSessionStorageObject::HasKey (const XBOX::VString &inKey)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	return fKeysValues.find(inKey) != fKeysValues.end();
}

void VJSSessionStorageObject::GetKeyValue (const XBOX::VString &inKey, XBOX::VJSValue *outValue)
{
	xbox_assert(outValue != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	SMap::iterator							it;

	if ((it = fKeysValues.find(inKey)) == fKeysValues.end())

		outValue->SetNull();

	else

		*outValue = it->second->MakeValue(outValue->GetContext());
}

void VJSSessionStorageObject::GetKeyValue (sLONG inIndex, XBOX::VJSValue *outValue)
{
	xbox_assert(outValue != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	
	if (inIndex < 0 || inIndex >= fKeysValues.size())

		outValue->SetNull();

	else {

		SMap::iterator	it;
		sLONG			i;

		// Iterative, slow! 

		for (it = fKeysValues.begin(), i = 0; i < inIndex; it++, i++)

			;

		*outValue = it->second->MakeValue(outValue->GetContext());

	}
}

void VJSSessionStorageObject::GetKeyFromIndex (sLONG inIndex, XBOX::VString *outKey)
{
	xbox_assert(outKey != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	
	if (inIndex < 0 || inIndex >= fKeysValues.size())

		outKey->Clear();

	else {

		SMap::iterator	it;
		sLONG			i;

		// Iterative, slow! 

		for (it = fKeysValues.begin(), i = 0; i < inIndex; it++, i++)

			;

		*outKey = it->first;
		
	}
}

void VJSSessionStorageObject::SetKeyValue (const XBOX::VString &inKey, const XBOX::VJSValue& inValue)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	VJSStructuredClone						*clone;
	
	if ((clone = VJSStructuredClone::RetainClone(inValue)) != NULL) {
	
		fKeysValues[inKey] = VRefPtr<VJSStructuredClone>(clone);
		ReleaseRefCountable( &clone);

		//**	TODO: Notify set event.
		
	}
}

bool VJSSessionStorageObject::RemoveKeyValue (const XBOX::VString &inKey)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	SMap::iterator							it;
	
	if ((it = fKeysValues.find(inKey)) != fKeysValues.end()) {

		fKeysValues.erase(it);

		//** TODO: Notify remove event.	 

		return true;

	} else

		return false;
}

void VJSSessionStorageObject::Clear ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	SMap::iterator							it;

	fKeysValues.clear();

	//** TODO: Notify clear event.
}

void VJSSessionStorageObject::GetKeys (XBOX::VJSParms_getPropertyNames &ioParms)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	SMap::iterator							it;

	for (it = fKeysValues.begin(); it != fKeysValues.end(); it++)

		ioParms.AddPropertyName(it->first);
}

bool VJSSessionStorageObject::TryLock ()
{
	return fMutex.TryToLock();
}

void VJSSessionStorageObject::Lock ()
{
	fMutex.Lock();
}

void VJSSessionStorageObject::Unlock ()
{
	// If not owner of the mutex, do nothing.

	if (fMutex.GetOwnerTaskID() == XBOX::VTask::GetCurrentID())
	
		fMutex.Unlock();
}

void VJSSessionStorageObject::ForceUnlock ()
{
	if (fMutex.GetOwnerTaskID() == XBOX::VTask::GetCurrentID())
	
		while (fMutex.GetUseCount())

			fMutex.Unlock();
}

void VJSSessionStorageObject::SetKeyVValueSingle( const XBOX::VString &inKey,  const XBOX::VValueSingle& inValue)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	VJSStructuredClone *clone = VJSStructuredClone::RetainCloneForVValueSingle( inValue);
	if (clone != NULL)
	{
		fKeysValues[inKey] = VRefPtr<VJSStructuredClone>(clone);
		ReleaseRefCountable( &clone);
	}
}

void VJSSessionStorageObject::SetKeyVBagArray( const XBOX::VString &inKey, const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	VJSStructuredClone *clone = VJSStructuredClone::RetainCloneForVBagArray( inBagArray, inUniqueElementsAreNotArrays);
	if (clone != NULL)
	{
		fKeysValues[inKey] = VRefPtr<VJSStructuredClone>(clone);
		ReleaseRefCountable( &clone);
	}
}

void VJSSessionStorageObject::SetKeyVValueBag( const XBOX::VString &inKey, const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	VJSStructuredClone *clone = VJSStructuredClone::RetainCloneForVValueBag( inBag, inUniqueElementsAreNotArrays);
	if (clone != NULL)
	{
		fKeysValues[inKey] = VRefPtr<VJSStructuredClone>(clone);
		ReleaseRefCountable( &clone);
	}
}

void VJSSessionStorageObject::FillWithVValueBag( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays)
{
	// inspired from VValueBag::GetJSONString
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	// Iterate the attributes
	VString attName;
	VIndex attCount = inBag.GetAttributesCount();
	for (VIndex attIndex = 1 ; attIndex <= attCount ; ++attIndex)
	{
		const VValueSingle *attValue = inBag.GetNthAttribute( attIndex, &attName);
		if ((attName != L"____objectunic") && (attName != L"____property_name_in_jsarray"))
		{
			VJSStructuredClone *clone = NULL;

			if (attValue != NULL)
			{
				clone = VJSStructuredClone::RetainCloneForVValueSingle( *attValue);
			}
			else
			{
				VString emptyString;
				clone = VJSStructuredClone::RetainCloneForVValueSingle( emptyString);
			}

			if (clone != NULL)
			{
				VValueBag::StKey CDataBagKey( attName);
				if (CDataBagKey.Equal( VValueBag::CDataAttributeName()))
					attName = "__cdata";

				fKeysValues[attName] = VRefPtr<VJSStructuredClone>(clone);
				ReleaseRefCountable( &clone);
			}
		}
	}

	// Iterate the elements
	VString elementName;
	VIndex elementNamesCount = inBag.GetElementNamesCount();
	for (VIndex elementNamesIndex = 1 ; elementNamesIndex <= elementNamesCount ; ++elementNamesIndex)
	{
		const VBagArray* bagArray = inBag.GetNthElementName( elementNamesIndex, &elementName);
		if (bagArray != NULL)
		{
			VJSStructuredClone *clone = NULL;

			if ((bagArray->GetCount() == 1) && inUniqueElementsAreNotArrays)
			{
				clone = VJSStructuredClone::RetainCloneForVValueBag( *bagArray->GetNth(1), inUniqueElementsAreNotArrays);
			}
			else if (bagArray->GetNth(1)->GetAttribute("____objectunic") != NULL)
			{
				clone = VJSStructuredClone::RetainCloneForVValueBag( *bagArray->GetNth(1), inUniqueElementsAreNotArrays);
			}
			else
			{
				clone = VJSStructuredClone::RetainCloneForVBagArray( *bagArray, inUniqueElementsAreNotArrays);
			}

			if (clone != NULL)
			{
				fKeysValues[elementName] = VRefPtr<VJSStructuredClone>(clone);
				ReleaseRefCountable( &clone);
			}
		}
	}
}

const char *VJSStorageClass::kMethodNames[kNumberMethods] = {

	// Order is not important.

	"key",
	"getItem",
	"setItem",
	"removeItem",
	"clear",		

	"tryLock",
	"lock",
	"unlock",

};

void VJSStorageClass::GetDefinition (ClassDefinition &outDefinition)
{
	static inherited::StaticFunction	functions[] =
	{
		{	"key",			js_callStaticFunction<_key>,		JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeDontEnum	},
		{	"getItem",		js_callStaticFunction<_getItem>,	JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		{	"setItem",		js_callStaticFunction<_setItem>,	JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		{	"removeItem",	js_callStaticFunction<_removeItem>,	JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		{	"clear",		js_callStaticFunction<_clear>,		JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},

		{	"tryLock",		js_callStaticFunction<_tryLock>,	JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		{	"lock",			js_callStaticFunction<_lock>,		JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		{	"unlock",		js_callStaticFunction<_unlock>,		JS4D::PropertyAttributeDontDelete |	JS4D::PropertyAttributeDontEnum	},
		
		{	0,				0,									0																	},
	};
			
	outDefinition.className			= "Storage";	
	outDefinition.staticFunctions	= functions;
    outDefinition.hasProperty		= js_hasProperty<_HasProperty>;
    outDefinition.getProperty		= js_getProperty<_GetProperty>;
    outDefinition.setProperty		= js_setProperty<_SetProperty>;
    outDefinition.deleteProperty	= js_deleteProperty<_DeleteProperty>;
	outDefinition.getPropertyNames	= js_getPropertyNames<_GetPropertyNames>;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
}

void VJSStorageClass::_Initialize( const XBOX::VJSParms_initialize& inParms, VJSStorageObject *inStorageObject)
{
	inStorageObject->Retain();
}

void VJSStorageClass::_Finalize( const XBOX::VJSParms_finalize& inParms, VJSStorageObject *inStorageObject)
{
	inStorageObject->Release();
}

void VJSStorageClass::_HasProperty (XBOX::VJSParms_hasProperty &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		ioParms.ReturnFalse();

	else {

		if (inStorageObject->HasKey(name)
		|| name.EqualToUSASCIICString("length"))

			ioParms.ReturnTrue();

		else {

			sLONG	i;
		
			for (i = 0; i < kNumberMethods; i++) 

				if (name.EqualToUSASCIICString(kMethodNames[i])) 

					ioParms.ReturnTrue();

			ioParms.ReturnFalse();

		}

	}
}

void VJSStorageClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		ioParms.ReturnNullValue();

	else if (name.EqualToUSASCIICString("length")) {

		XBOX::VJSValue	length(ioParms.GetContext());

		length.SetNumber(inStorageObject->NumberKeysValues());

		ioParms.ReturnValue(length);

	} else {
		
		XBOX::VJSValue	value(ioParms.GetContext());

		value.SetNull();

		sLONG	i;

		for (i = 0; i < kNumberMethods; i++) 

			if (name.EqualToUSASCIICString(kMethodNames[i])) {

				ioParms.ForwardToParent();				
				break;

			}

		if (i == kNumberMethods) {

			inStorageObject->GetKeyValue(name, &value);
			ioParms.ReturnValue(value);

		}

	}
}

bool VJSStorageClass::_SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);
 
	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		return true;

	if (name.EqualToUSASCIICString("length")) 

		return true;

	sLONG	i;

	for (i = 0; i < kNumberMethods; i++)
	{
		if (name.EqualToUSASCIICString(kMethodNames[i]))

			return true;
	}
	inStorageObject->SetKeyValue(name, ioParms.GetPropertyValue());
	return true;
}

void VJSStorageClass::_GetPropertyNames (XBOX::VJSParms_getPropertyNames &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	inStorageObject->GetKeys(ioParms);
}

void VJSStorageClass::_DeleteProperty (XBOX::VJSParms_deleteProperty &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (!ioParms.GetPropertyName(name))

		ioParms.ReturnFalse();

	else 

		ioParms.ReturnBoolean(inStorageObject->RemoveKeyValue(name));
}

void VJSStorageClass::_key (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	sLONG	index;

	if (ioParms.CountParams() && ioParms.IsNumberParam(1) && ioParms.GetLongParam(1, &index)) {

		XBOX::VString	key;

		inStorageObject->GetKeyFromIndex(index, &key);
		if (!key.IsEmpty())

			ioParms.ReturnString(key);

		else

			ioParms.ReturnNullValue();	// Index is out of bound.

	} else

		ioParms.ReturnNullValue();
}

void VJSStorageClass::_getItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (ioParms.CountParams() && ioParms.IsStringParam(1) && ioParms.GetStringParam(1, name)) {

		XBOX::VJSValue	value(ioParms.GetContext());

		inStorageObject->GetKeyValue(name, &value);

		ioParms.ReturnValue(value);

	} else

		ioParms.ReturnNullValue();
}

void VJSStorageClass::_setItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (ioParms.CountParams() && ioParms.IsStringParam(1) && ioParms.GetStringParam(1, name)) {

		XBOX::VJSValue	value(ioParms.GetContext());
		
		if (ioParms.CountParams() < 2)
		
			value.SetUndefined();

		else

			value = ioParms.GetParamValue(2);

		inStorageObject->SetKeyValue(name, value);

	}	
}

void VJSStorageClass::_removeItem (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	XBOX::VString	name;

	if (ioParms.CountParams() && ioParms.IsStringParam(1) && ioParms.GetStringParam(1, name))

		inStorageObject->RemoveKeyValue(name);
}

void VJSStorageClass::_clear (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	inStorageObject->Clear();
}

void VJSStorageClass::_tryLock (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	ioParms.ReturnBool(inStorageObject->TryLock());
}

void VJSStorageClass::_lock (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	inStorageObject->Lock();
}

void VJSStorageClass::_unlock (VJSParms_callStaticFunction &ioParms, VJSStorageObject *inStorageObject)
{
	xbox_assert(inStorageObject != NULL);

	inStorageObject->Unlock();
}
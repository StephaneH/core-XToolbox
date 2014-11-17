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

#include "VJSEnvironment.h"


USING_TOOLBOX_NAMESPACE

bool VJSEnvironmentObject::HasVariable(const VString& inVariableName, VError& outError) const
{
	bool r = XEnvironmentVariables::HasVariable(inVariableName, outError);

	return r;
}

VString VJSEnvironmentObject::GetVariable(const VString& inVariableName, VError& outError) const
{
	VString variableValue = XEnvironmentVariables::GetVariable(inVariableName, outError);
	
	return variableValue;
}

void VJSEnvironmentObject::GetKeys(VectorOfString& outKeys, VError& outError) const
{
	XEnvironmentVariables::GetKeys(outKeys, outError);
}

bool VJSEnvironmentObject::SetVariable(const VString& inVariableName, VString& inVariableValue, VError& outError) const
{
	bool r = XEnvironmentVariables::SetVariable(inVariableName, inVariableValue, outError);

	return r;
}

bool VJSEnvironmentObject::DeleteVariable(const VString& inVariableName, VError& outError) const
{
	bool r = XEnvironmentVariables::DeleteVariable(inVariableName, outError);

	return r;
}

// ---------------------------------------------------------------------
// ------------------- VJSEnvironmentClass -----------------------------
// ---------------------------------------------------------------------


void VJSEnvironmentClass::GetDefinition(ClassDefinition& outDefinition)
{
	static inherited::StaticFunction	functions[] =
	{
		{ 0, 0, 0 }
	};

	outDefinition.className = "Environment";
	outDefinition.staticFunctions = functions;
	outDefinition.hasProperty = js_hasProperty<_HasProperty>;
	outDefinition.getProperty = js_getProperty<_GetProperty>;
	outDefinition.setProperty = js_setProperty<_SetProperty>;
	outDefinition.deleteProperty = js_deleteProperty<_DeleteProperty>;
	outDefinition.getPropertyNames = js_getPropertyNames<_GetPropertyNames>;
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
}


void VJSEnvironmentClass::Initialize(const VJSParms_initialize& inParms, VJSEnvironmentObject* inEnvironmentObject)
{
	RetainRefCountable(inEnvironmentObject);
}


void  VJSEnvironmentClass::Finalize(const VJSParms_finalize& inParms, VJSEnvironmentObject* inEnvironmentObject)
{
	ReleaseRefCountable(&inEnvironmentObject);
}


void VJSEnvironmentClass::_HasProperty(VJSParms_hasProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject)
{
	xbox_assert(inEnvironmentObject != NULL);

	VString name;

	if (!ioParms.GetPropertyName(name))
	{
		ioParms.ReturnFalse();
	}
	else
	{
		VError err = VE_OK;

		bool res = inEnvironmentObject->HasVariable(name, err);

		if (err != VE_OK)
		{
			vThrowError(err);
		}

		if (res)
		{
			ioParms.ReturnTrue();
		}
		else
		{
			ioParms.ReturnFalse();
		}
	}
}

void VJSEnvironmentClass::_GetProperty(VJSParms_getProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject)
{
	xbox_assert(inEnvironmentObject != NULL);

	VString name;

	if (!ioParms.GetPropertyName(name))
	{
		ioParms.ReturnNullValue();
	}
	else
	{
		VError err = VE_OK;

		VString res = inEnvironmentObject->GetVariable(name, err);

		if (err != VE_OK)
		{
			vThrowError(err);
		}

		ioParms.ReturnString(res);
	}
}

bool VJSEnvironmentClass::_SetProperty(VJSParms_setProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject)
{
	xbox_assert(inEnvironmentObject != NULL);

	VString name;

	if (!ioParms.GetPropertyName(name))
	{
		return false;
	}

	VString value;

	if (!ioParms.GetPropertyValue().GetString(value))
	{
		return false;
	}

	VError err = VE_OK;

	bool res = inEnvironmentObject->SetVariable(name, value, err);

	if (err != VE_OK)
	{
		vThrowError(err);
	}

	return res;
}

void VJSEnvironmentClass::_DeleteProperty(VJSParms_deleteProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject)
{
	xbox_assert(inEnvironmentObject != NULL);

	VString name;

	if (!ioParms.GetPropertyName(name))
	{
		ioParms.ReturnFalse();
	}
	else
	{
		VError err = VE_OK;

		bool res = inEnvironmentObject->DeleteVariable(name, err);

		if (err != VE_OK)
		{
			vThrowError(err);
		}

		if (res)
		{
			ioParms.ReturnTrue();
		}
		else
		{
			ioParms.ReturnFalse();
		}
	}
}

void VJSEnvironmentClass::_GetPropertyNames(VJSParms_getPropertyNames &ioParms, VJSEnvironmentObject* inEnvironmentObject)
{
	xbox_assert(inEnvironmentObject != NULL);

	VectorOfString keys;

	VError err = VE_OK;

	inEnvironmentObject->GetKeys(keys, err);

	if (err != VE_OK)
	{
		vThrowError(err);
	}

	for (VectorOfString::iterator it = keys.begin(), end = keys.end(); it != end; ++it)
	{
		ioParms.AddPropertyName(*it);
	}
}


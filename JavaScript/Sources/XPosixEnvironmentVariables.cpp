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

#include "XPosixEnvironmentVariables.h"

/*
the global variable environ is a variable available on linux
that gives the environment variables as a null terminated list of char*
every member in this list is under this form property_name=property_value
*/

/*
on Mac OSX we define environ variable
*/

#if VERSION_LINUX
extern char** environ;
//this variable will be initialized at C library runtime load
#endif

#if VERSIONMAC
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif

USING_TOOLBOX_NAMESPACE


bool XPosixEnvironmentVariables::HasVariable(const VString& inVariableName, VError& outError)
{
    outError = VE_OK;
    
	VStringConvertBuffer inVariableNameBuffer(inVariableName, VTC_UTF_8);

	const char* val = getenv(inVariableNameBuffer.GetCPointer());

	if (val != NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}

VString XPosixEnvironmentVariables::GetVariable(const VString& inVariableName, VError& outError)
{
    outError = VE_OK;
    
	VString variableValue = CVSTR("");

	VStringConvertBuffer inVariableNameBuffer(inVariableName, VTC_UTF_8);

	const char* variableBuffer = getenv(inVariableNameBuffer.GetCPointer());

	if (variableBuffer != NULL)
	{
		variableValue.FromBlock(variableBuffer, ::strlen(variableBuffer), VTC_UTF_8);
	}

	return variableValue;
}

void XPosixEnvironmentVariables::GetKeys(VectorOfString& outKeys, VError& outError)
{
    outError = VE_OK;
    
	for (sLONG i = 0; environ[i] != NULL; ++i)
	{
		const char* var = environ[i];

		const char* s = strchr(var, '=');

		const sLONG varLength = s ? s - var : strlen(var);

		VString key(var, varLength, VTC_UTF_8);

		outKeys.push_back(key);
	}
}

bool XPosixEnvironmentVariables::SetVariable(const VString& inVariableName, VString& inVariableValue, VError& outError)
{
    outError = VE_OK;
    
	VStringConvertBuffer inVariableNameBuffer(inVariableName, VTC_UTF_8);

	VStringConvertBuffer inVariableValueBuffer(inVariableValue, VTC_UTF_8);

	int rc = setenv(inVariableNameBuffer.GetCPointer(), inVariableValueBuffer.GetCPointer(), 1);

	if (rc == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool XPosixEnvironmentVariables::DeleteVariable(const VString& inVariableName, VError& outError)
{
    outError = VE_OK;
    
	VStringConvertBuffer inVariableNameBuffer(inVariableName, VTC_UTF_8);

	bool rc = (getenv(inVariableNameBuffer.GetCPointer()) != NULL);

	if (rc)
	{
		int r = unsetenv(inVariableNameBuffer.GetCPointer());

		return (r == 0);
	}
	else
	{
		return false;
	}
}

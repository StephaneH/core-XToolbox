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

#include "XWinEnvironmentVariables.h"
#include "VJSErrors.h"

/*
	The theoretical maximum length of an environment variable is around 33,760 characters.
	However, you are unlikely to attain that theoretical maximum in practice.
	All environment variables must live together in a single environment block,
	which itself has a limit of 32767 characters.
	But that count is the sum over all environment variable names and values, so you could,
	I guess, hit that theoretical maximum length if you deleted all the environment variables
	and then set a single variable called X with that really huge 32,760 character value.

	for more information about this limit please refer to this link:
		http://blogs.msdn.com/b/oldnewthing/archive/2010/02/03/9957320.aspx

*/

#define MAX_ENV_VARIABLE_SIZE 32767

USING_TOOLBOX_NAMESPACE

bool XWinEnvironmentVariables::HasVariable(const VString& inVariableName, VError& outError)
{
	WCHAR buffer[MAX_ENV_VARIABLE_SIZE];
	DWORD result = GetEnvironmentVariableW((LPCWSTR)inVariableName.GetCPointer(), buffer, MAX_ENV_VARIABLE_SIZE);
	DWORD lastError = GetLastError();
	outError = VE_OK;

	if ((result == 0) && (lastError == ERROR_ENVVAR_NOT_FOUND))
	{
		return false;
	}
	else if (lastError == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		outError = VE_ENVV_ENV_VARIABLE_HAS_PROPERTY_ERROR;

		return false;
	}
}

VString XWinEnvironmentVariables::GetVariable(const VString& inVariableName, VError& outError)
{
	WCHAR buffer[MAX_ENV_VARIABLE_SIZE];
	DWORD count = GetEnvironmentVariableW((LPCWSTR)inVariableName.GetCPointer(), buffer, MAX_ENV_VARIABLE_SIZE);

	VString variableValue = CVSTR("");

	DWORD lastError = GetLastError();

	if (lastError == ERROR_SUCCESS)
	{
		outError = VE_OK;

		if ((count > 0) && (count < MAX_ENV_VARIABLE_SIZE))
		{
			variableValue.FromBlock(buffer, (2 * count), VTC_UTF_16);
		}
	}
	else
	{
		outError = VE_ENVV_ENV_VARIABLE_GET_PROPERTY_ERROR;
	}

	return variableValue;
}

void XWinEnvironmentVariables::GetKeys(VectorOfString& outKeys, VError& outError)
{
	WCHAR* environment = GetEnvironmentStringsW();

	if (environment == NULL)
	{
		outError = VE_ENVV_ENV_VARIABLE_GET_KEYS_ERROR;

		return;
	}

	WCHAR* p = environment;

	int i = 0;

	while (*p != '\0')
	{
		WCHAR *s = NULL;

		// If the key starts with '=' it is a hidden environment variable.
		if (*p == L'=')
		{
			p += wcslen(p) + 1;
			continue;
		}
		else
		{
			s = wcschr(p, L'=');
		}

		if (!s)
		{
			s = p + wcslen(p);
		}

		uLONG keyLength = s - p;

		void* keyBuffer = (void*)p;

		VString key(keyBuffer, (2 * keyLength), VTC_UTF_16);

		outKeys.push_back(key);

		p = s + wcslen(s) + 1;
	}

	FreeEnvironmentStringsW(environment);
}

bool XWinEnvironmentVariables::SetVariable(const VString& inVariableName, VString& inVariableValue, VError& outError)
{
	bool res = true;

	outError = VE_OK;

	// Environment variables that start with '=' are read-only
	if (inVariableName.GetLength() > 0 && inVariableName[0] != L'=')
	{
		BOOL b = SetEnvironmentVariableW(inVariableName.GetCPointer(), inVariableValue.GetCPointer());

		//to not have conversion issues with conversion from BOOL to bool
		if (b == TRUE)
		{
			res = true;
		}
		else
		{
			res = false;
		}
	}
	else
	{
		res = false;
	}

	return res;
}

bool XWinEnvironmentVariables::DeleteVariable(const VString& inVariableName, VError& outError)
{
	bool res = true;

	outError = VE_OK;

	// Environment variables that start with '=' are read-only
	if (inVariableName.GetLength() > 0 && inVariableName[0] != L'=')
	{
		BOOL b = SetEnvironmentVariableW(inVariableName.GetCPointer(), NULL);

		if (b == FALSE)
		{
			DWORD lastError = GetLastError();

			if ((GetEnvironmentVariableW(inVariableName.GetCPointer(), NULL, NULL) == 0) && (lastError == ERROR_ENVVAR_NOT_FOUND))
			{
				res = true;
			}
			else if (lastError == ERROR_SUCCESS)
			{
				res = false;
			}
			else
			{
				res = false;

				outError = VE_ENVV_ENV_VARIABLE_DEL_PROPERTY_ERROR;
			}

		}
		else
		{
			res = true;
		}
	}
	else
	{
		res = false;
	}

	return res;
}

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
#include "VKernelIPCPrecompiled.h"
#include "VEnvironmentVariables.h"

VString & VEnvironmentVariables::operator[](const VString &inVarName)
{
	if(!inVarName.IsEmpty())
		return fEnvVars[inVarName];
	
	fEmptyValue.Clear();
	return fEmptyValue;
}

void VEnvironmentVariables::SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inNewValues)
{
	fEnvVars.clear();
	AddEnvironmentVariables(inNewValues);
}

void VEnvironmentVariables::AddEnvironmentVariables(const EnvVarNamesAndValuesMap &inValuesToAdd)
{
	if(!inValuesToAdd.empty())
	{
		EnvVarNamesAndValuesMap::const_iterator	envVarIterator = inValuesToAdd.begin();
		while(envVarIterator != inValuesToAdd.end())
		{
			if(!envVarIterator->first.IsEmpty())
				fEnvVars[ envVarIterator->first ] = envVarIterator->second;
			
			++envVarIterator;
		}
	}
}

void VEnvironmentVariables::SetEnvironmentVariable(const VString &inEnvName, const VString &inEnvValue)
{
	if(!inEnvName.IsEmpty())
		fEnvVars[inEnvName] = inEnvValue;
}

bool VEnvironmentVariables::GetEnvironmentVariableValue(const VString &inEnvName, VString &outEnvValue, bool inAndRemoveIt)
{
	bool	gotIt = false;
	
	outEnvValue.Clear();
	if(!inEnvName.IsEmpty() && !fEnvVars.empty())
	{
		EnvVarNamesAndValuesMap::iterator	envVarIt = fEnvVars.find(inEnvName);
		if(envVarIt != fEnvVars.end())
		{
			outEnvValue = envVarIt->second;
			gotIt = true;
			if(inAndRemoveIt)
				fEnvVars.erase(envVarIt);
		}
	}
	
	return gotIt;
}

void VEnvironmentVariables::RemoveEnvironmentVariable(const VString &inEnvName)
{
	if(!inEnvName.IsEmpty() && !fEnvVars.empty())
	{
		EnvVarNamesAndValuesMap::iterator	envVarIt = fEnvVars.find(inEnvName);
		if(envVarIt != fEnvVars.end())
			fEnvVars.erase(envVarIt);
	}
}

void VEnvironmentVariables::Clear()
{
	fEnvVars.clear();
}
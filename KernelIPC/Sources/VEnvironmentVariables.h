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
#ifndef __VEnvironmentVariables__
#define __VEnvironmentVariables__

BEGIN_TOOLBOX_NAMESPACE

/*	Utility map, that uses the variable name as the key, in a case sensitive way, because envir. variales are handled case-sensitively.
*/
typedef std::map<VString, VString, VStringLessCompareStrict>	EnvVarNamesAndValuesMap;

/*	VEnvironmentVariables encapsulates a EnvVarNamesAndValuesMap

*/
class XTOOLBOX_API VEnvironmentVariables : public VObject, public IRefCountable
{
public:
					VEnvironmentVariables()			{}
	virtual			~VEnvironmentVariables()		{}
	
	/** @brief For operations on the map directly ( quick usage)
	
		Example:
			XBOX::VEnvironmentVariables	*ev = curtu->fDBLanguageContext->GetEnvironmentVariables();
			//. . .
			if(ev != NULL && !ev->empty())
			{
				XBOX::EnvVarNamesAndValuesMap::const_iterator	envVarIterator = ev->begin();
				while(envVarIterator != ev->end())
				{
					//. . . envVarIterator->first
					//. . . envVarIterator->second
					
					++envVarIterator;
				}
			}
		
		Other example:
			XBOX::VEnvironmentVariables	theEv;
			
			theEv["PHP_FCGI_CHILDREN"] = "10";
			theEv["_4D_PHP_SCRIPT_PATH"] = thePath;
			// . . .
			if(theEv["DO_THIS"] == "true")
			{
				// . . .
			}
	*/
	bool			empty() const		{ return fEnvVars.empty(); }
	size_t			size() const		{ return fEnvVars.size(); }
	EnvVarNamesAndValuesMap::iterator			begin()	{ return fEnvVars.begin(); }
	EnvVarNamesAndValuesMap::const_iterator		end()	{ return fEnvVars.end(); }
	
	/** @brief	If inVarName is empty, the returned ref. is fEmptyValue */
	VString&		operator[] (const VString &inVarName);
	
	/** @brief	Replace all values and remove emty var. name, if any. */
	void			SetEnvironmentVariables(const EnvVarNamesAndValuesMap &inNewValues);
	
	/** @brief Add values. If a variable is already defined, its value is replaced by the new one */
	void			AddEnvironmentVariables(const EnvVarNamesAndValuesMap &inValuesToAdd);
	
	/** @brief Create or modify an environement variable. Does nothing if inEnvName is "" */
	void			SetEnvironmentVariable(const VString &inEnvName, const VString &inEnvValue);
	
	/** @brief Return true if inEnvName was found (to deambigate the fact that an envir. var can have an empty value) */
	bool			GetEnvironmentVariableValue(const VString &inEnvName, VString &outEnvValue, bool inAndRemoveIt = false);

	/** @brief RemoveEnvironmentVariable() */
	void			RemoveEnvironmentVariable(const VString &inEnvName);
	
	/** @brief Get a copy all the variables */
	inline void		GetVariables(EnvVarNamesAndValuesMap &outVars)	{ outVars = fEnvVars; }

	
	/** @brief Clear the EnvVarNamesAndValuesMap (ie after LAUNCH EXTERNAL PROCESS or PHP EXECUTE) */
	void			Clear();
		
private:
	EnvVarNamesAndValuesMap		fEnvVars;
	VString						fEmptyValue;
};

END_TOOLBOX_NAMESPACE

#endif
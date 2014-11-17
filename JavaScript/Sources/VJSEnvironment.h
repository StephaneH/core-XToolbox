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

#ifndef __VJS_ENVIRONMENT__
#define __VJS_ENVIRONMENT__

#include "VJSClass.h"
#include "VJSValue.h"

/*
	environment variables has two main diffent implementations:
		- one on windows
		- second on posix systems
	so the implementation is separated in two different classes:
		- XWinEnvironmentVariables
		- XPosixEnvironmentVariables
*/

#if VERSIONWIN
#include "XWinEnvironmentVariables.h"
#else
#include "XPosixEnvironmentVariables.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

class VJSEnvironmentObject : public VObject, public IRefCountable
{
public:
	VJSEnvironmentObject() {}
	~VJSEnvironmentObject() {}

	bool HasVariable(const VString& inVariableName, VError& outError) const;
	VString GetVariable(const VString& inVariableName, VError& outError) const;
	void GetKeys(VectorOfString& outKeys, VError& outError) const;
	bool SetVariable(const VString& inVariableName, VString& inVariableValue, VError& outError) const;
	bool DeleteVariable(const VString& inVariableName, VError& outError) const;

private:
};

class VJSEnvironmentClass : public VJSClass<VJSEnvironmentClass, VJSEnvironmentObject>
{
public:
	typedef VJSClass<VJSEnvironmentClass, VJSEnvironmentObject> inherited;

	static void GetDefinition(ClassDefinition& outDefinition);

	static void Initialize(const VJSParms_initialize& inParms, VJSEnvironmentObject* inEnvironmentObject);
	static void Finalize(const VJSParms_finalize& inParms, VJSEnvironmentObject* inEnvironmentObject);

private:
	static void _HasProperty(VJSParms_hasProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject);
	static void _GetProperty(VJSParms_getProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject);
	static bool _SetProperty(VJSParms_setProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject);
	static void _DeleteProperty(VJSParms_deleteProperty &ioParms, VJSEnvironmentObject* inEnvironmentObject);
	static void _GetPropertyNames(VJSParms_getPropertyNames &ioParms, VJSEnvironmentObject* inEnvironmentObject);
};

END_TOOLBOX_NAMESPACE

#endif

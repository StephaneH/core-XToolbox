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

#ifndef __VJS_MODULE__
#define __VJS_MODULE__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

// VJSModuleState is not the state of a module but the state of the module system of a JavaScript context.

class XTOOLBOX_API VJSModuleState : public XBOX::VObject
{
friend class VJSRequireClass;

public:

	// If the require() function path is not specified, then find it in walib folder.

	static void						SetRequireFunctionPath (const XBOX::VString &inFullPath);

	static VJSModuleState			*CreateModuleState (const XBOX::VJSContext &inContext);
	virtual							~VJSModuleState ()	{}
	
	// Retrieve ObjectRef to JavaScript code for require() function.

	XBOX::VJSObject					GetRequireFunction()	{	return fRequireFunction;	}

	// Load a script file, inFullPath must be a POSIX path.

	static XBOX::VError				LoadScript (const XBOX::VString &inFullPath, XBOX::VURL *outURL, XBOX::VString *outScript);

private:

	static XBOX::VCriticalSection	sMutex;
	static bool						sIsCodeLoaded;
	static XBOX::VString			sRequireFunctionPath;
	static XBOX::VString			sRequireFunctionScript;	

	std::vector<XBOX::VFilePath>	fCurrentPaths;			// Stack of execution paths.
	XBOX::VJSObject					fRequireFunction;

									VJSModuleState (const VJSContext& inContext);

	// Retrieve the full path of requireFunction.js. 

	static void						_GetRequireFunctionPath (XBOX::VString *outFullPath);
};

// The require object is implemented using both C++ and JavaScript. 
// The actual require() function is implemented using JavaScript.

class XTOOLBOX_API VJSRequireClass : public XBOX::VJSClass<VJSRequireClass, VJSModuleState>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	MakeObject (const XBOX::VJSContext &inContext);
	
private:

	static void				_Initialize (const VJSParms_initialize &inParms, VJSModuleState *inModuleState);
	static bool				_SetProperty (XBOX::VJSParms_setProperty &ioParms, VJSModuleState *inModuleState);
	static void				_CallAsFunction (VJSParms_callAsFunction &ioParms);

	static void				_evaluate (VJSParms_callStaticFunction &ioParms, VJSModuleState *inModuleState);
	static void				_getCurrentPath (VJSParms_callStaticFunction &ioParms, VJSModuleState *inModuleState);
};

END_TOOLBOX_NAMESPACE

#endif

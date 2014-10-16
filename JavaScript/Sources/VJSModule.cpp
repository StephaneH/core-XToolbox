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

#if USE_V8_ENGINE

#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "VJSModule.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection	VJSModuleState::sMutex;
bool					VJSModuleState::sIsCodeLoaded;
XBOX::VString			VJSModuleState::sRequireFunctionPath	= CVSTR("");
XBOX::VString			VJSModuleState::sRequireFunctionScript	= CVSTR("");

void VJSModuleState::SetRequireFunctionPath (const XBOX::VString &inFullPath)
{
	sRequireFunctionPath = inFullPath;
}

VJSModuleState *VJSModuleState::CreateModuleState (const XBOX::VJSContext &inContext)
{
	VJSModuleState	*moduleState;

	if ((moduleState = new VJSModuleState(inContext)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		return NULL;

	}

	XBOX::VError	error;
	XBOX::VString	fullPath;
	XBOX::VURL		url;
	
	error = XBOX::VE_OK;
	if (!sRequireFunctionPath.IsEmpty())
	
		fullPath = sRequireFunctionPath;
	
	else
	
		_GetRequireFunctionPath(&fullPath);
	
	// 4D doesn't have require.js
	if (fullPath.IsEmpty())
		return NULL;
	
	// If needed, load the JavaScript code of the require() function.

	if (!sIsCodeLoaded) {

		// Avoid race condition.

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	
		if (!sIsCodeLoaded) {

			error = LoadScript(fullPath, &url, &sRequireFunctionScript);

			// If loaded successfully, then check for syntax.

			if (error == XBOX::VE_OK && !inContext.CheckScriptSyntax(sRequireFunctionScript, &url, NULL))

				error = XBOX::VE_JVSC_SYNTAX_ERROR;

			// Even if loading or syntax check fails, set boolean as true, so as not to try again.

			sIsCodeLoaded = true;

		}
	
	}

	// Make require() function.

	if (error != XBOX::VE_OK) {

		if (error == XBOX::VE_JVSC_SCRIPT_NOT_FOUND || error == XBOX::VE_JVSC_SYNTAX_ERROR)

			XBOX::vThrowError(error, fullPath);

		else

			XBOX::vThrowError(error);
		
	} else {
		
		XBOX::VJSValue				result(inContext);
		XBOX::VJSException			exception;

		if (!inContext.EvaluateScript(sRequireFunctionScript, &url, &result, exception))
		
			error = XBOX::vThrowError(XBOX::VE_JVSC_REQUIRE_FUNCTION_EVALUATION, fullPath);

		else {

			// Must return a function object for require().

			xbox_assert(result.IsFunction());

			moduleState->fRequireFunction = result.GetObject((VJSException*)NULL);

			// ~VJSGlobalContext() will call JS4D::UnprotectValue().

			moduleState->fRequireFunction.Protect();

		}

	}

	if (error != XBOX::VE_OK) {

		delete moduleState;
		moduleState = NULL;

	}

	return moduleState;
}

XBOX::VError VJSModuleState::LoadScript (const XBOX::VString &inFullPath, XBOX::VURL *outURL, XBOX::VString *outScript)
{
	xbox_assert(outURL != NULL && outScript != NULL);

	outURL->FromFilePath(inFullPath, eURL_POSIX_STYLE, CVSTR("file://"));
	outScript->Clear();

	XBOX::VError	error;
	XBOX::VFilePath	path;
					
	if (outURL->GetFilePath(path) && path.IsFile()) {

		XBOX::VFile			file(path);			
		XBOX::VFileStream	stream(&file);
						
		if ((error = stream.OpenReading()) == XBOX::VE_OK
		&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

			stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
			error = stream.GetText(*outScript);

		}
		stream.CloseReading();	// Ignore closing error, if any.

		if (error == XBOX::VE_STREAM_EOF)

			error = XBOX::VE_OK;	// WAK0079581: An empty module file is ok (require() will return an empty object).

	} else

		error = XBOX::VE_JVSC_SCRIPT_NOT_FOUND;

	return error;
}

VJSModuleState::VJSModuleState (const VJSContext& inContext)
:fRequireFunction(inContext)
{
}

void VJSModuleState::_GetRequireFunctionPath (XBOX::VString *outFullPath)
{
	xbox_assert(outFullPath != NULL);
#if 0 // testV8
	outFullPath->FromString("C:/WStmp/depot/XToolbox/Main/Tests/V8/requireFunction.js");
#else
	XBOX::VFolder	*walibFolder;
	
	if ((walibFolder = XBOX::VProcess::Get()->RetainFolder('walb')) != NULL) {

		XBOX::VFilePath	path(walibFolder->GetPath(), "WAF/Core/Native/requireFunction.js", FPS_POSIX);
			
		path.GetPosixPath(*outFullPath);

		XBOX::ReleaseRefCountable<XBOX::VFolder>(&walibFolder);

	} else {

		// Fail safe, should never happen.

		outFullPath->Clear();

	}
#endif
}

void VJSRequireClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSRequireClass, void>::StaticFunction functions[] =
	{
		{	"_evaluate",		js_callStaticFunction<_evaluate>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete	},
		{	"_getCurrentPath",	js_callStaticFunction<_getCurrentPath>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete	},
		{	0,					0,										0																										},
	};

    outDefinition.className			= "require";
	outDefinition.staticFunctions	= functions;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.callAsFunction	= js_callAsFunction<_CallAsFunction>;

}


XBOX::VJSObject	VJSRequireClass::MakeObject (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	constructedObject(inContext);
	VJSModuleState	*moduleState;

	if ((moduleState = VJSModuleState::CreateModuleState(inContext)) == NULL) {

		// Creation of the VJSModuleState object failed, error has already been thrown.
				
	} else if (!moduleState->GetRequireFunction().HasRef()) {

		// Loading of the require() function JavaScript implementation has failed.
		// Error has already be thrown by constructor, just delete created object.

		delete moduleState;

	} else
		constructedObject = VJSRequireClass::CreateInstance(inContext, moduleState);

	return constructedObject;
}

void VJSRequireClass::_Initialize (const VJSParms_initialize &inParms, VJSModuleState *inModuleState)
{
	XBOX::VJSObject	thisObject(inParms.GetObject());
	XBOX::VJSArray	arrayObject(inParms.GetContext());
	XBOX::VFolder	*folder;

	folder  = XBOX::VFolder::RetainSystemFolder(eFK_Executable, false);
	xbox_assert(folder != NULL);	
	
#if VERSIONMAC
	
	XBOX::VFolder	*parentFolder = folder->RetainParentFolder();
	
	XBOX::VFilePath	path(parentFolder->GetPath(), "Modules/", FPS_POSIX);
	
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&parentFolder);
	
#else
	
	XBOX::VFilePath	path(folder->GetPath(), "Modules/", FPS_POSIX);
	
#endif	

	XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

	XBOX::VString	defaultPath;

	path.GetPosixPath(defaultPath);

	arrayObject.PushString(defaultPath);
	thisObject.SetProperty("paths", arrayObject, JS4D::PropertyAttributeDontDelete);
}

void VJSRequireClass::_CallAsFunction (VJSParms_callAsFunction &ioParms)
{
	XBOX::VString	idString;

	if (!ioParms.GetStringParam(1, idString)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	XBOX::VJSContext		context(ioParms.GetContext());
	VJSModuleState			*moduleState;
	VJSObject				object(context);

	xbox_assert(ioParms.GetFunction().IsOfClass(VJSRequireClass::Class()));

	moduleState = ioParms.GetFunction().GetPrivateData<VJSRequireClass>();
	xbox_assert(moduleState != NULL);

	object = moduleState->GetRequireFunction();
	if (!object.HasRef())

		return;

	// Read the execution path

	XBOX::VFilePath	path;

	XBOX::VString	urlString;
#if USE_V8_ENGINE
	urlString = context.GetCurrentScriptUrl();
	if (urlString.GetLength()) {
#else
	JS4D::StringRef	sourceURL;
	if ((sourceURL = JS4DGetExecutionURL(context)) != NULL) {

		if (JS4D::StringToVString(sourceURL, urlString)) {
#endif
			XBOX::VURL	url(urlString, true);
	
			if (!url.GetFilePath(path))

				path.Clear();	// Failed to parse URL, make path empty.
#if USE_V8_ENGINE
#else
		}
		JSStringRelease(sourceURL);
#endif
	} 

	// Push an empty path if no URL or if the URL is incorrect.
	// If a relative path is used and the URL is an empty path, this is correctly detected as an error later.

	moduleState->fCurrentPaths.push_back(path);	

	// If not already there, add the current project module folder to paths array.

	XBOX::VJSGlobalObject	*globalObject;

	if ((globalObject = ioParms.GetContext().GetGlobalObjectPrivateInstance()) != NULL) {

		XBOX::VFolder	*folder;

		if ((folder = globalObject->GetRuntimeDelegate()->RetainScriptsFolder()) != NULL) {

			XBOX::VString	modulePath;

			folder->GetPath(modulePath, FPS_POSIX);
			folder->Release();

			modulePath.AppendString("Modules/");

			XBOX::VJSObject	arrayObject	= ioParms.GetFunction().GetPropertyAsObject("paths");

			if (arrayObject.IsArray()) {

				XBOX::VJSArray	pathsArray(arrayObject, false);
				sLONG			i;
			
				for (i = 0; i < pathsArray.GetLength(); i++) {

					XBOX::VJSValue	value		= pathsArray.GetValueAt(i);
					XBOX::VString	pathString;

					if (value.IsString() && value.GetString(pathString) && pathString.EqualToString(modulePath))

						break;

				}
				if (i == pathsArray.GetLength())

					pathsArray.PushString(modulePath);

			}
		
		}

	}

	XBOX::VJSObject				requireFunction = object;
	std::vector<XBOX::VJSValue>	arguments;
	XBOX::VJSValue				result(context);
	XBOX::VJSException			exception;

	for (sLONG i = 0; i < ioParms.CountParams(); i++)
		
		arguments.push_back(ioParms.GetParamValue(i + 1));

	if (!context.GetGlobalObject().CallFunction(requireFunction, &arguments, &result, exception)) 

		ioParms.SetException(exception.GetValue());

	else 

		ioParms.ReturnValue(result);	

	moduleState->fCurrentPaths.pop_back();
}

void VJSRequireClass::_evaluate (VJSParms_callStaticFunction &ioParms, VJSModuleState *inModuleState)
{
	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VString		fullPath;
	XBOX::VJSObject		exportsObject(context), moduleObject(context);

	if (!ioParms.GetStringParam(1, fullPath)) {
		
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	if (!ioParms.GetParamObject(2, exportsObject)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "2");
		return;

	}

	if (!ioParms.GetParamObject(3, moduleObject)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "3");
		return;

	}

	XBOX::VError	error;
	XBOX::VURL		url;
	XBOX::VString	script;
		
	if ((error = VJSModuleState::LoadScript(fullPath, &url, &script)) != XBOX::VE_OK) {

		if (error == XBOX::VE_JVSC_SCRIPT_NOT_FOUND) {

			// Should not happen as require() does already check for existence.

			XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, fullPath);

		} else

			XBOX::vThrowError(error);
			
	} else {

		XBOX::VFilePath	path(fullPath, FPS_POSIX);
		XBOX::VFile		*file;
			
		if ((file = new XBOX::VFile(path)) == NULL) {

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			
		} else {

			context.GetGlobalObjectPrivateInstance()->RegisterIncludedFile(file);

			XBOX::VString				functionScript;
			XBOX::VJSValue				functionResult(context);
			XBOX::VJSException			exception;			

			// Make a function of the module script, then call it.
			
			functionScript.AppendString("(function (exports, module) {");
			functionScript.AppendString(script);
			functionScript.AppendString("\n})");	// Add a newline so it is possible for the last line to be a "//" comment.

			if (!context.EvaluateScript(functionScript, &url, &functionResult, exception)) {
						
				ioParms.SetException(exception.GetValue());	// Syntax error.

			} else {

				xbox_assert(functionResult.IsFunction());

				XBOX::VJSObject				functionObject = functionResult.GetObject((VJSException*)NULL);
				std::vector<XBOX::VJSValue>	arguments;
				XBOX::VJSValue				result(context);

				arguments.push_back(exportsObject);
				arguments.push_back(moduleObject);

				if (!context.GetGlobalObject().CallFunction(functionObject, &arguments, &result, exception)) 

					ioParms.SetException(exception.GetValue());

				else 

					ioParms.ReturnValue(result);

			}

			XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

		}

	}	
}

// Actually returns the directory of the currently executing script.

void VJSRequireClass::_getCurrentPath (VJSParms_callStaticFunction &ioParms, VJSModuleState *inModuleState)
{
	xbox_assert(inModuleState != NULL);
	
	xbox_assert(!inModuleState->fCurrentPaths.empty());
	if (inModuleState->fCurrentPaths.empty()) {
		
		// This should never happen.

		XBOX::vThrowError(XBOX::VE_JVSC_MODULE_INTERNAL_ERROR);	
		return;

	}

	XBOX::VFilePath	path, parent;

	path = inModuleState->fCurrentPaths.back();
	if (path.IsEmpty()) {

		// Current JavaScript execution doesn't have an URL. 
		// (Because it is probably a function called directly from C++.)
		// Relative paths are unavailable then.

		XBOX::vThrowError(XBOX::VE_JVSC_UNABLE_TO_RETRIEVE_URL);
		return;

	}

	if (!path.GetParent(parent)) {

		ioParms.ReturnString("/");

	} else {

		XBOX::VString	posixPath;

		parent.GetPosixPath(posixPath);

		ioParms.ReturnString(posixPath);

	}
}
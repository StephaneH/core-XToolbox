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
#include "V4DContext.h"
using namespace v8;
#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#else
#include <JavaScriptCore/JavaScript.h>
#endif
#endif
#include "VJSConstructor.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE

#if USE_V8_ENGINE



class VJSConstructor : public VObject 
{
public:
	VJSConstructor( JS4D::ClassDefinition* inClassRef, VJSParms_construct::fn_construct inCallback, bool inAcceptCallAsFunction,JS4D::StaticFunction inContructorFunctions[])
	: fClassRef( inClassRef)
	, fCallback( inCallback)
	, fAcceptCallAsFunction( inAcceptCallAsFunction)
	, fContructorFunctions(inContructorFunctions) {}

	static void Callback(const v8::FunctionCallbackInfo<v8::Value>& args);
	static XBOX::VJSObject MakeConstructor(
					const VJSContext&					inContext, 
					JS4D::ClassRef						inClassRef, 
					VJSParms_construct::fn_construct	inCallback, 
					bool								inAcceptCallAsFunction,
					JS4D::StaticFunction				inContructorFunctions[]);

private:

			JS4D::ClassDefinition*				fClassRef;
			VJSParms_construct::fn_construct	fCallback;
			bool								fAcceptCallAsFunction;	
			JS4D::StaticFunction*				fContructorFunctions;
};


VJSObject VJSConstructor::MakeConstructor( const VJSContext& inContext, JS4D::ClassRef inClassRef, VJSParms_construct::fn_construct inCallback, bool inAcceptCallAsFunction,JS4D::StaticFunction inContructorFunctions[] )
{
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;

	VJSConstructor*	newConstructor = new VJSConstructor(classDef,inCallback,inAcceptCallAsFunction,inContructorFunctions);
	V4DContext* v8Ctx = (V4DContext*)(inContext.fContext->GetData());
	v8::Handle<v8::Function>	fn = v8Ctx->CreateFunctionTemplateForClassConstructor(
										classDef,
										VJSConstructor::Callback,
										(void *)newConstructor,
										inAcceptCallAsFunction,
										inContructorFunctions);

	VJSValue	vjsVal(inContext,fn.val_);
	VJSObject	result(inContext);
	result = vjsVal.GetObject((JS4D::ExceptionRef*)NULL);
	return result;
}

void VJSConstructor::Callback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	VJSConstructor*			vjsConstructor = NULL;
	v8::Local<v8::Value>	valData = args.Data();
	if (testAssert(valData->IsExternal()))
	{
		v8::External*	extData = v8::External::Cast(valData.val_);
		vjsConstructor = (VJSConstructor*)extData->Value();
	}
	if (!strcmp(vjsConstructor->fClassRef->className, "require"))
	{
		int a = 1;
	}
	//DebugMsg("VJSConstructor::Callback called for clName=%s\n",vjsConstructor->fClassRef->className);
	if (args.IsConstructCall())
	{
		if (args.Length() == 1)
		{
			if (args[0]->IsString())
			{
				v8::String::Utf8Value	paramVal(args[0]);
				if (!strcmp(*paramVal,"NativeParam"))
				{
					return;
				}
			}
		}
		JS4D::ObjectRef result;
		{
			XBOX::VJSParms_construct	parms(args);
			if (vjsConstructor->fCallback)
			{
				vjsConstructor->fCallback(parms);
			}
		}
		// don't return NULL without exception or JSC will crash
		/*if ( (result == NULL) && (*outException == NULL) )
		{
			*outException = JS4D::MakeException( inContext, CVSTR( "Failed to create object."));
		}*/
	}
	else
	{
		if (testAssert(vjsConstructor->fAcceptCallAsFunction))
		{
			xbox_assert(args.GetIsolate() == v8::Isolate::GetCurrent());
			V4DContext*	v8Ctx = (V4DContext*)(args.GetIsolate()->GetData());

			v8::Persistent<v8::FunctionTemplate>*	pFT = v8Ctx->GetImplicitFunctionConstructor(vjsConstructor->fClassRef->className);
			v8::Local<v8::Function> locFunc = (pFT->val_)->GetFunction();
			Handle<Value>	*argv = new Handle<Value>[args.Length()];
			for (int idx = 0; idx < args.Length(); idx++)
			{
				argv[idx] = args[idx];
			}
			Local<Value> locVal = locFunc->CallAsConstructor(args.Length(), argv);
			args.GetReturnValue().Set(locVal);
		}
		else
		{
			VString					tmpStr = "VJSConstructor::Callback Constructor should not be called as a function for class ";
			tmpStr += vjsConstructor->fClassRef->className;
			DebugMsg(tmpStr + "   !!!!\n");
			VStringConvertBuffer	tmpBuffer(tmpStr, VTC_UTF_8);
			v8::ThrowException(v8::String::New(tmpBuffer.GetCPointer()));
			return;
		}
	}
}

#else
class VJSConstructor : public VObject, public IRefCountable
{
public:
	VJSConstructor( JSClassRef inClassRef, VJSParms_construct::fn_construct inCallback, bool inAcceptCallAsFunction)
		: fClassRef( inClassRef), fCallback( inCallback), fAcceptCallAsFunction( inAcceptCallAsFunction) 	{}


	static JSClassRef Class()
	{
		static JSClassRef sClassRef = NULL;
		if (sClassRef == NULL)
		{
			JSClassDefinition def = kJSClassDefinitionEmpty;
			def.attributes = kJSClassAttributeNoAutomaticPrototype;
			def.className = "GenericConstructor";
			def.callAsConstructor = _callAsConstructor;
			def.callAsFunction = _callAsFunction;
			def.hasInstance = _hasInstance;
			def.finalize = _finalize;
			def.initialize = _initialize;
			sClassRef = JSClassCreate( &def);
		}
		return sClassRef;
	}

private:

	static void __cdecl _initialize( JSContextRef inContext, JSObjectRef inObject)
	{
		VJSConstructor *constructor = reinterpret_cast<VJSConstructor*>( JSObjectGetPrivate( inObject));
		constructor->Retain();
	}
		
	static void __cdecl _finalize( JSObjectRef inObject)
	{
		VJSConstructor *constructor = reinterpret_cast<VJSConstructor*>( JSObjectGetPrivate( inObject));
		constructor->Release();
	}

	static	JSObjectRef __cdecl _callAsConstructor( JSContextRef inContext, JSObjectRef inConstructor, size_t inArgumentCount, const JSValueRef inArguments[], JSValueRef* outException)
	{
		VJSConstructor *constructor = reinterpret_cast<VJSConstructor*>( JSObjectGetPrivate( inConstructor));
		JS4D::ObjectRef result;
		{
			VJSParms_construct	parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
			constructor->fCallback( parms);
			result = parms.GetConstructedObject().GetObjectRef();
		}
		// don't return NULL without exception or JSC will crash
		if ( (result == NULL) && (*outException == NULL) )
		{
			*outException = JS4D::MakeException( inContext, CVSTR( "Failed to create object."));
		}
		return result;
	}


	static	JSValueRef __cdecl _callAsFunction( JSContextRef inContext, JSObjectRef inConstructor, JSObjectRef inThisObject, size_t inArgumentCount, const JSValueRef inArguments[], JSValueRef* outException)
	{
		VJSConstructor *constructor = reinterpret_cast<VJSConstructor*>( JSObjectGetPrivate( inConstructor));

		// test if class allows constructor called as function
		if (testAssert(constructor->fAcceptCallAsFunction))
		{
			VJSParms_construct	parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
			constructor->fCallback( parms);
			JSValueRef value = parms.GetConstructedObject().GetObjectRef();
			return (value != NULL) ? value : JSValueMakeNull(inContext);	// JSC crashes on NULL even if there's an exception
		}

		// this constructor has been called as a function, but that has been forbidden.
		// throw an error the same way as webkit
		JSStringRef jsMessage = JSStringCreateWithUTF8CString( "Failed to construct object : Please use the 'new' operator, this object constructor cannot be called as a function.");
		JSValueRef	args[1];
		args[0] = JSValueMakeString( inContext, jsMessage);
		JSStringRelease( jsMessage);

		JSStringRef jsClassName = JSStringCreateWithUTF8CString( "TypeError");
		JSObjectRef errorConstructor = JSValueToObject( inContext, JSObjectGetProperty( inContext, JSContextGetGlobalObject( inContext), jsClassName, NULL), NULL);
		JSStringRelease( jsClassName);
		*outException = JSObjectCallAsConstructor( inContext, errorConstructor, 1, args, NULL);

		return JSValueMakeUndefined( inContext);
	}

	static	bool		__cdecl _hasInstance( JSContextRef inContext, JSObjectRef inConstructor, JSValueRef inPossibleInstance, JSValueRef* outException)
	{
		VJSConstructor *constructor = reinterpret_cast<VJSConstructor*>( JSObjectGetPrivate( inConstructor));
		return JSValueIsObjectOfClass( inContext, inPossibleInstance, constructor->fClassRef);
	}

			JS4D::ClassRef						fClassRef;
			VJSParms_construct::fn_construct	fCallback;
			bool								fAcceptCallAsFunction;
};
#endif

VJSObject XTOOLBOX_API JS4DMakeConstructor( const VJSContext& inContext, JS4D::ClassRef inClass, VJSParms_construct::fn_construct inCallback, bool inAcceptCallAsFunction, JS4D::StaticFunction inContructorFunctions[])
{

#if USE_V8_ENGINE
	return VJSConstructor::MakeConstructor(inContext,inClass,inCallback,inAcceptCallAsFunction,inContructorFunctions);

#else
	if (inCallback == NULL)
	{
		return VJSObject(inContext);
	}
	VJSConstructor *constructor = new VJSConstructor( inClass, inCallback, inAcceptCallAsFunction);
	
	JSObjectRef constructorObjectRef = JSObjectMake( inContext, VJSConstructor::Class(), constructor);

	ReleaseRefCountable( &constructor);

	VJSObject	newObj = VJSObject::CreateFromPrivateRef( inContext, constructorObjectRef);

	if (inContructorFunctions)
	{
		for( JS4D::StaticFunction*	tmpFn = inContructorFunctions; tmpFn->name != NULL ; ++tmpFn)
		{
			VJSObject	newProperty(inContext);
			newProperty.MakeCallback(tmpFn->callAsFunction);
			newObj.SetProperty(tmpFn->name,newProperty,tmpFn->attributes);
		}
	}

	return newObj;
#endif

}


END_TOOLBOX_NAMESPACE
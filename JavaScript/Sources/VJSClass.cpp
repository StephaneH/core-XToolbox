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

#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE

#if 0
static VURL * CreateURLFromPath(const VString& inPath);
static VURL * CreateURLFromPath(const VString& inPath)
{
	VURL* result = NULL;

	if(inPath.GetLength() > 0)
	{
		VIndex index = inPath.Find("://");
		if(index > 0)//URL
		{
			result = new VURL(inPath, false);
		}
		else//system path
		{
			result = new VURL(inPath, eURL_POSIX_STYLE, L"file://");
		}
	}

	return result;
}
#endif



//======================================================
#if USE_V8_ENGINE

/*
void VJSParms_construct::CallbackProlog()
{
    xbox::DebugMsg("VJSParms_construct::CallbackProlog called  nbArgs=%d isConstruct=%d InternalFieldCount()=%d\n",
		fV8Arguments->Length(),
		(fV8Arguments->IsConstructCall() ? 1 : 0),
		fV8Arguments->Holder()->InternalFieldCount());
}
void VJSParms_construct::CallbackEpilog()
{
}
void VJSParms_construct::ReturnConstructedObjectProlog()
{
	xbox::DebugMsg("VJSParms_construct::ReturnConstructedObjectProlog called  IsConstructCall()=%d\n",
		(fV8Arguments->IsConstructCall() ? 1 : 0));
}
*/

#if 0
class XBOX::JS4D::V8ObjectGetPropertyCallback
{
public:
	V8ObjectGetPropertyCallback(XBOX::GetPropertyFn inFunction) { fGetPropertyFn = inFunction; }

	static	void					AccessorGetterCallback(Local<String> property, const PropertyCallbackInfo<Value>& info);

private:
	XBOX::GetPropertyFn				fGetPropertyFn;
};


class XBOX::JS4D::V8ObjectSetPropertyCallback
{
public:
	V8ObjectSetPropertyCallback(XBOX::SetPropertyFn inFunction) { fSetPropertyFn = inFunction; }

	static	void					AccessorSetterCallback(Local<String> property, Local<Value> value, const PropertyCallbackInfo<Value>& info);

private:
	XBOX::SetPropertyFn				fSetPropertyFn;
};

struct V8PropertyHandlerCallback_st
{
	V8PropertyHandlerCallback_st(JS4D::ObjectGetPropertyCallback inGetCbk, JS4D::ObjectSetPropertyCallback inSetCbk) { getCbk = inGetCbk; setCbk = inSetCbk; }
	JS4D::ObjectGetPropertyCallback		getCbk;
	JS4D::ObjectSetPropertyCallback		setCbk;
};

void XBOX::JS4D::V8ObjectGetPropertyCallback::AccessorGetterCallback(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::String::Utf8Value	utfVal(property);
	VString	propName(*utfVal);

	XBOX::GetPropertyFn		getPropertyFn = NULL;
	Local<Value> dataVal = info.Data();
	if (dataVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(dataVal);
		V8PropertyHandlerCallback_st*	propertyHandlerCallback = (V8PropertyHandlerCallback_st*)extHdl->Value();
		//JS4D::ObjectGetPropertyCallback getPropertyCallback = (JS4D::ObjectGetPropertyCallback)extHdl->Value();
		if (testAssert(propertyHandlerCallback->getCbk))
		{
			getPropertyFn = propertyHandlerCallback->getCbk()->fGetPropertyFn;//getPropertyCallback()->fGetPropertyFn;
		}
	}
	VJSParms_getProperty	parms(propName, info);
	Local<Object> locObj = info.Holder();
	void* data = JS4D::GetObjectPrivate(info.GetIsolate(), locObj.val_);

	getPropertyFn(parms, data);
}

void XBOX::JS4D::V8ObjectSetPropertyCallback::AccessorSetterCallback(Local<String> property, Local<Value> value, const PropertyCallbackInfo<v8::Value>& info)
{
	v8::String::Utf8Value	utfVal(property);
	VString	propName(*utfVal);

	XBOX::SetPropertyFn		setPropertyFn = NULL;
	Local<Value> dataVal = info.Data();
	if (dataVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(dataVal);
		V8PropertyHandlerCallback_st*	propertyHandlerCallback = (V8PropertyHandlerCallback_st*)extHdl->Value();
		if (testAssert(propertyHandlerCallback->setCbk))
		{
			setPropertyFn = propertyHandlerCallback->setCbk()->fSetPropertyFn;
		}
	}
	VJSParms_setProperty	parms(propName, value.val_, info);
	Local<Object> locObj = info.Holder();
	void* data = JS4D::GetObjectPrivate(info.GetIsolate(), locObj.val_);

	setPropertyFn(parms, data);
}

void* XBOX::VJSClass__GetV8PropertyGetterCallback(JS4D::V8ObjectGetPropertyCallback* inCbk)
{
	return (void*)inCbk->AccessorGetterCallback;
}

void* XBOX::VJSClass__GetV8PropertySetterCallback(JS4D::V8ObjectSetPropertyCallback* inCbk)
{
	return (void*)inCbk->AccessorSetterCallback;
}

XBOX::JS4D::V8ObjectGetPropertyCallback* XBOX::VJSClass__GetPropertyGetterCallback(XBOX::GetPropertyFn inFunction)
{
	XBOX::JS4D::V8ObjectGetPropertyCallback*	result = NULL;
	result = new XBOX::JS4D::V8ObjectGetPropertyCallback(inFunction);
	return result;
}

XBOX::JS4D::V8ObjectSetPropertyCallback* XBOX::VJSClass__GetPropertySetterCallback(XBOX::SetPropertyFn inFunction)
{
	XBOX::JS4D::V8ObjectSetPropertyCallback*	result = NULL;
	result = new XBOX::JS4D::V8ObjectSetPropertyCallback(inFunction);
	return result;
}
void* XBOX::VJSClass__GetV8PropertyHandlerCallback(JS4D::ObjectGetPropertyCallback cbkG, JS4D::ObjectSetPropertyCallback cbkS)
{
	return new V8PropertyHandlerCallback_st(cbkG, cbkS);
}

#endif


void XBOX::VJSClassImpl::CallStaticFunctionEpilog(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	/*Local<Object>		locObj = args.Holder();
	Local<String>		constName = locObj->GetConstructorName();
	String::Utf8Value	utf8Name(constName);
	if (!strcmp(*utf8Name, "TextStream"))
	{
		void*	data = JS4D::GetObjectPrivate(args.GetIsolate(), args.Holder().val_);
		if (!data)
		{
			int a = 1;
		}
	
	}*/
	//DebugMsg("VJSClassImpl::CallStaticFunctionEpilog called IsConstructCall=%d\n",
	//    args.IsConstructCall() ? 1 : 0);
}

bool XBOX::VJSClassImpl::CallAsConstructor(JS4D::ClassRef inClassRef, const v8::FunctionCallbackInfo<v8::Value>& args, fn_callAsConstructor fn)
{
	bool isOK = false;
	/*VJSParms_callAsConstructor parms(inContext, inConstructor, inArgumentCount, inArguments, outException);
	fn(parms);
	return parms.GetConstructedObject().GetObjectRef();*/
	if (testAssert(args.IsConstructCall()))
	{
		VJSParms_callAsConstructor	callParams(args);
		fn(callParams);

		const VJSObject&	constructedObject = callParams.GetConstructedObject();

		if (constructedObject.HasRef())
		{
#if V8_USE_MALLOC_IN_PLACE
			Handle<Object>	tmpVal = (*ToV8Persistent(constructedObject.fObject))->ToObject();
#else
			Handle<Value>	tmpVal = Handle<Value>::New(args.GetIsolate(), constructedObject.fObject); 
#endif
			args.GetReturnValue().Set(tmpVal);

			/*JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;

			VJSContext	vjsCtx = constructedObject.GetContext();

			JS4D::ContextRef	ctxRef = vjsCtx;
			if (classDef->initialize)
			{
				classDef->initialize(ctxRef, constructedObject.fObject);
			}*/
			isOK = true;
		}
		else
		{
			args.GetReturnValue().SetUndefined();
		}
	}
	return isOK;
}



void VJSClassImpl::UpdateInstanceFromPrivateData(JS4D::ContextRef inContextRef, const VJSObject& inObjRef, void* inPrivateData, JS4D::ClassDefinition* inClassDef)
{
	if (!testAssert(inContextRef == v8::Isolate::GetCurrent()))
	{
		int a = 1;
	}
	V4DContext::SetObjectPrivateData(inContextRef, inObjRef, inPrivateData, inClassDef);
}

void* VJSClassImpl::GetHolderPrivateData(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	return V4DContext::GetHolderPrivateData(info);
}

VJSObject VJSClassImpl::CreateInstanceFromPrivateData(const XBOX::VJSContext& inContext, JS4D::ClassRef inClassRef, void *inPrivateData)
{
	if (!testAssert(inContext.fContext == v8::Isolate::GetCurrent()))
	{
		int a = 1;
	}
	HandleScope handle_scope(inContext);
	v8::Persistent<v8::Context>*	persistentContext = V4DContext::GetPersistentContext(inContext);
	Handle<Context> context = Handle<Context>::New(inContext,*persistentContext);
	Context::Scope context_scope(context);
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	V4DContext*	v8Ctx = (V4DContext*)(v8::Isolate::GetCurrent()->GetData(0));

	if (classDef->callAsConstructor && classDef->callAsFunction )
	{
		int a = 1;
		/*Handle<External>	extPtr = External::New((void*)inClassRef);
		Handle<Function>	locFunc = v8::Function::New(inContext.fContext, V4DContext::DualCallFunctionHandler, extPtr);
		VJSObject	result(inContext, locFunc.val_, true);
		V4DContext::SetObjectPrivateData(v8::Isolate::GetCurrent(), result.fObject, inPrivateData);
		return result;*/
	}

	if (!strcmp(classDef->className, "EntityAttribute"))
	{
		int a = 1;
	}
	Persistent<FunctionTemplate>*	pFT = v8Ctx->GetImplicitFunctionConstructor(classDef->className);
	v8::Local<v8::FunctionTemplate> locFT = Handle<FunctionTemplate>::New(inContext.fContext, *pFT);
	v8::Local<v8::Function> locFunc = locFT->GetFunction();

	Handle<String>	paramStr = v8::String::NewFromUtf8(inContext.fContext,"NativeParam");
	Handle<Value>	argv[1];
	argv[0] = paramStr;
	Handle<Value> locVal = locFunc->CallAsConstructor(1, argv);
	if (!*locVal)
	{
#if V8_CHECK_CONTEXT_REUSE
		v8Ctx->DisplayStack();
#endif
	}
	if (!testAssert(*locVal != NULL))
	{
		int a = 1;
	}
	if (locVal->IsUndefined())
	{
		DebugMsg("VJSClassImpl::CreateInstanceFromPrivateData created UNDEFINED for %s\n", classDef->className);
	}
	if (locVal->IsNull())
	{
		DebugMsg("VJSClassImpl::CreateInstanceFromPrivateData created NULL for %s\n", classDef->className);
	}

	VJSObject	result(inContext, locVal->ToObject());

	V4DContext::SetObjectPrivateData(v8::Isolate::GetCurrent(), result, inPrivateData, classDef );

	return result;

}

#else


#endif


//======================================================

/*
void VJSParms_finalize::UnprotectValue( JS4D::ValueRef inValue) const
{
	JSValueUnprotect( NULL, inValue);
}
*/


//======================================================




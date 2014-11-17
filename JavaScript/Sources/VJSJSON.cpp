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
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "VJSValue.h"
#include "VJSJSON.h"

USING_TOOLBOX_NAMESPACE


VJSJSON::VJSJSON( const XBOX::VJSContext& inContext, XBOX::VJSException *outException)
: fContext( inContext)
#if USE_V8_ENGINE
#else
, fJSONObject(inContext)
, fParseFunction(inContext)
, fStringifyFunction(inContext)
#endif
{
#if USE_V8_ENGINE
#else
	_GetJSON(*this,VJSException::GetExceptionRefIfNotNull(outException));
#endif
}


void VJSJSON::Parse(VJSValue& ioValue, const VString& inJSON, XBOX::VJSException *outException)
{
#if USE_V8_ENGINE
	xbox_assert(v8::Isolate::GetCurrent() == fContext.fContext);
	V4DContext*				v8Ctx = (V4DContext*)fContext.fContext->GetData(0);
//	DebugMsg("VJSJSON::Parse parsing-> '%S' ....\n", &inJSON);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext.fContext);
	HandleScope				handleScope(fContext);
	Local<Context>			locCtx = Handle<Context>::New(fContext.fContext,*v8PersContext);
	Context::Scope			context_scope(locCtx);
#if V8_USE_MALLOC_IN_PLACE
	ToV8Persistent(ioValue.fValue).Reset();
	if (outException)
	{
		TryCatch		tryCatch;
		Local<Value>	resultVal = v8::JSON::Parse(v8::String::NewFromTwoByte(fContext.fContext, inJSON.GetCPointer(), v8::String::kNormalString, inJSON.GetLength()));
		if (tryCatch.HasCaught())
		{
			outException->SetValue(fContext.fContext, tryCatch.Exception());
		}
		if (!resultVal.IsEmpty())
		{
			//String::Utf8Value		utf8Val(resultVal);
			//	DebugMsg("VJSJSON::Parse result-> '%s'\n", *utf8Val);
			ToV8Persistent(ioValue.fValue).Reset(fContext.fContext, resultVal);
		}
	}
	else
	{
		Local<Value>	resultVal = v8::JSON::Parse(v8::String::NewFromTwoByte(fContext.fContext, inJSON.GetCPointer(), v8::String::kNormalString, inJSON.GetLength()));
		if (!resultVal.IsEmpty())
		{
			ToV8Persistent(ioValue.fValue).Reset(fContext.fContext, resultVal);
		}
	}
#else
	Handle<Value>			resultVal = v8::JSON::Parse(v8::String::NewFromTwoByte(fContext.fContext, inJSON.GetCPointer(), v8::String::kNormalString, inJSON.GetLength()));
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	if (*resultVal)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(*resultVal);
		ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
#endif
#else
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	_Parse(ioValue, jsString, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsString);
#endif
}


VJSValue VJSJSON::Parse( const VString& inJSON, JS4D::ObjectCallAsFunctionCallback inReviverFunction, XBOX::VJSException *outException)
{
#if USE_V8_ENGINE
	xbox_assert(false);
	return VJSValue( fContext );
#else
	JSObjectRef reviverFunction = JSObjectMakeFunctionWithCallback( fContext, NULL, inReviverFunction);
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	VJSValue value = _Parse( jsString, reviverFunction, VJSException::GetExceptionRefIfNotNull(outException) );
	JSStringRelease( jsString);
	return value;
#endif
}

#if USE_V8_ENGINE
#else
void VJSJSON::_Parse(VJSValue& ioValue, JS4D::StringRef inJSON, JS4D::ExceptionRef *outException)
{
	_GetParseFunction( outException);

	if ((fParseFunction.fObject == NULL) || (inJSON == NULL))
		ioValue.fValue = NULL;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON)};
	ioValue.fValue = JS4DObjectCallAsFunction(fContext, fParseFunction.fObject, fJSONObject.fObject, 1, arguments, outException);

}

XBOX::VJSValue VJSJSON::_Parse(JS4D::StringRef inJSON, JS4D::ObjectRef inReviverFunction, JS4D::ExceptionRef *outException)
{
	VJSValue	result(fContext);

	_GetParseFunction(outException);
	if ((fParseFunction.fObject == NULL) || (inJSON == NULL) || (inReviverFunction == NULL))
		return result;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON), inReviverFunction };
	result.fValue = JS4DObjectCallAsFunction(fContext, fParseFunction.fObject, fJSONObject.fObject, 2, arguments, outException);

	return result;
}
#endif

void VJSJSON::Stringify( const VJSValue& inValue, VString& outJSON, XBOX::VJSException *outException)
{

	VJSValue xnull(inValue.fContext);
	xnull.SetNull();
#if USE_V8_ENGINE
	_Stringify(inValue, xnull, xnull, outJSON, outException);
#else
	_Stringify( inValue, xnull, xnull, outJSON, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}


void VJSJSON::StringifyWithSpaces( const VJSValue& inValue, sLONG inSpaces, VString& outJSON, VJSException *outException)
{
	VJSValue xnull(inValue);
	xnull.SetNull();
	VJSValue spacesVal(inValue.fContext);
	spacesVal.SetNumber(inSpaces);
#if USE_V8_ENGINE
	_Stringify(inValue, xnull, spacesVal, outJSON, outException);
#else
	_Stringify(inValue, xnull, spacesVal, outJSON, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}


#if USE_V8_ENGINE
void VJSJSON::_Stringify(const XBOX::VJSValue& inValue, const XBOX::VJSValue inReplacer, const XBOX::VJSValue inSpace, XBOX::VString& outJSON, VJSException *outException)
{
	outJSON.Clear();
	xbox_assert(fContext.fContext == v8::Isolate::GetCurrent());
	V4DContext* 				v8Ctx = (V4DContext*)((fContext.fContext)->GetData(0));
	xbox_assert(v8::Isolate::GetCurrent() == v8Ctx->GetIsolate());
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext.fContext);
	HandleScope					handle_scope(v8Ctx->GetIsolate());
	Handle<Context>				context = Handle<Context>::New(v8Ctx->GetIsolate(), *v8PersContext);
	/*Handle<Object>				global = context->Global();
	xbox_assert(!global->IsUndefined() && !global->IsNull() && global->IsObject());
	xbox_assert(global->Has(v8::String::New("JSON")));
	Local<Value>				locJSONFunc = global.val_->Get(v8::String::New("JSON"));*/
	//xbox_assert(!locJSONFunc->IsUndefined() && !locJSONFunc->IsNull());
	//if (locJSONFunc->IsObject())
	Persistent<Object>*		pStringifyFunc = v8Ctx->GetStringifyFunction();
	{
		//Local<Object>	locJSONObj = locJSONFunc->ToObject();
		//Local<Value>	locVal = locJSONObj->Get(v8::String::New("stringify"));
		//if (locVal->ToObject()->IsCallable())
		{
			Handle<Value>	args[3];
			VJSValue	tmpVal(inValue);
#if V8_USE_MALLOC_IN_PLACE
			args[0] = v8::Handle<Value>::New(fContext, ToV8Persistent(tmpVal.fValue));
			args[1] = v8::Handle<Value>::New(fContext, ToV8Persistent(inReplacer.fValue));
			args[2] = v8::Handle<Value>::New(fContext, ToV8Persistent(inSpace.fValue));
#else
			args[0] = v8::Handle<Value>::New(fContext, tmpVal.fValue);
			if (!inValue.fValue || inValue.fValue->IsUndefined() || !inValue.fValue->IsNull())
			{
				int a = 1;
			}
			if (!inReplacer.fValue || !inSpace.fValue)
			{
				int a = 1;
			}
			args[1] = v8::Handle<Value>::New(fContext, inReplacer.fValue);
			args[2] = v8::Handle<Value>::New(fContext, inSpace.fValue);
#endif
			Local<Object>	locFuncObj = (*(*pStringifyFunc))->ToObject();
			//v8::Handle<Value>	ghVal = v8::Handle<Value>::New(fContext, tmpVal.fValue);
			//v8::String::Utf8Value	utf8Str(ghVal);
			//DebugMsg("VJSJSON::_Stringify -> '%s'\n", *utf8Str);
			Local<Value>	resVal = locFuncObj->CallAsFunction(locFuncObj, 3, args);
			if (resVal->IsNull())
			{
				int a = 1;
			}
			if (resVal->IsUndefined())
			{
				outJSON = "undefined";
				return;
			}
			if (resVal->IsObject())
			{
				int a = 1;
			}
			v8::String::Value	resUtf(resVal);
			outJSON = *resUtf;
		}
	}
}
#else
void VJSJSON::_Stringify(const XBOX::VJSValue& inValue, const XBOX::VJSValue inReplacer, const XBOX::VJSValue inSpace, XBOX::VString& outJSON, JS4D::ExceptionRef *outException)
{
	_GetStringifyFunction( outException);
	if (fStringifyFunction.fObject == NULL)
		outJSON.Clear();
	else
	{
		JSValueRef arguments[] = { inValue.GetValueRef(), inReplacer.GetValueRef(), inSpace.GetValueRef() };

		JS4D::ExceptionRef except = NULL;
		JS4D::ExceptionRef* outExcept = outException;
		if (outExcept == NULL)
			outExcept = &except;
		JSValueRef result = JS4DObjectCallAsFunction(fContext, fStringifyFunction.fObject, fJSONObject.fObject, 3, arguments, outExcept);

		if (result == NULL)
			result = *outExcept;

		JS4D::ValueToString( fContext, result, outJSON, NULL);
	}
}
#endif



#if USE_V8_ENGINE

#else

void VJSJSON::_GetJSON(VJSJSON& ioJSON, JS4D::ExceptionRef *outException)
{

JSStringRef jsString = JSStringCreateWithUTF8CString( "JSON");
JSValueRef jsonValue = JSObjectGetProperty(ioJSON.fJSONObject.fContext, JSContextGetGlobalObject(ioJSON.fContext), jsString, outException);
ioJSON.fJSONObject.fObject = ((jsonValue != NULL) && JSValueIsObject(ioJSON.fContext, jsonValue)) ? JSValueToObject(ioJSON.fContext, jsonValue, outException) : NULL;
JSStringRelease( jsString);
}

void VJSJSON::_GetParseFunction( JS4D::ExceptionRef *outException)
{
	if (fParseFunction.fObject == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "parse");
		JSValueRef functionValue = JSObjectGetProperty( fContext, fJSONObject.fObject, jsString, outException);
		fParseFunction.fObject = ((functionValue != NULL) && JSValueIsObject( fContext, functionValue)) ? JSValueToObject( fContext, functionValue, outException) : NULL;
		JSStringRelease( jsString);
	}
}

void VJSJSON::_GetStringifyFunction(JS4D::ExceptionRef *outException)
{
	if (fStringifyFunction.fObject == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString("stringify");
		JSValueRef functionValue = JSObjectGetProperty(fContext, fJSONObject.fObject, jsString, outException);
		fStringifyFunction.fObject = ((functionValue != NULL) && JSValueIsObject(fContext, functionValue)) ? JSValueToObject(fContext, functionValue, outException) : NULL;
		JSStringRelease(jsString);
	}
}
#endif





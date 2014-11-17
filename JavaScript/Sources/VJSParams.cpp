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
#include "VJSParams.h"
#include "VJSContext.h"
#include "VJSValue.h"

#include "VJSRuntime_file.h"

BEGIN_TOOLBOX_NAMESPACE

	
VJSParms_withContext::~VJSParms_withContext()
{
	GetContext().EndOfNativeCodeExecution();
}


void VJSParms_withContext::ProtectValue( JS4D::ValueRef inValue) const
{
#if USE_V8_ENGINE

#else
	JSValueProtect( GetContext(), inValue);
#endif
}


void VJSParms_withContext::UnprotectValue( JS4D::ValueRef inValue) const
{
#if USE_V8_ENGINE

#else
	JSValueUnprotect( GetContext(), inValue);
#endif
}

#if USE_V8_ENGINE
VJSParms_withContext::VJSParms_withContext(const v8::FunctionCallbackInfo<v8::Value>& args)
: fObject(args.GetIsolate(), args.Holder())
{
}
VJSParms_withContext::VJSParms_withContext(const v8::PropertyCallbackInfo<void>* info)
: fObject(info->GetIsolate(), info->Holder())
{
}
VJSParms_withContext::VJSParms_withContext(const v8::PropertyCallbackInfo<v8::Value>* info)
: fObject(info->GetIsolate(), info->Holder())
{
}
VJSParms_withContext::VJSParms_withContext(const v8::PropertyCallbackInfo<v8::Array>& info)
: fObject(info.GetIsolate(), info.Holder())
{
}
#endif

//======================================================
#if  USE_V8_ENGINE
VJSParms_withException::VJSParms_withException(const v8::FunctionCallbackInfo<v8::Value>& args)
: VJSParms_withContext(args)
{
	fTaskContext = VTask::GetCurrent()->GetErrorContext(true);
	fErrorContext = fTaskContext->PushNewContext(false /* do not keep */, true /* silent */);
}
VJSParms_withException::VJSParms_withException(const v8::PropertyCallbackInfo<v8::Value>* info)
: VJSParms_withContext(info)
{
	fTaskContext = VTask::GetCurrent()->GetErrorContext(true);
	fErrorContext = fTaskContext->PushNewContext(false /* do not keep */, true /* silent */);
}
VJSParms_withException::VJSParms_withException(const v8::PropertyCallbackInfo<void>* info)
: VJSParms_withContext(info)
{
	fTaskContext = VTask::GetCurrent()->GetErrorContext(true);
	fErrorContext = fTaskContext->PushNewContext(false /* do not keep */, true /* silent */);
}
VJSParms_withException::VJSParms_withException(const v8::PropertyCallbackInfo<void>& info)
: VJSParms_withContext(&info)
{
	fTaskContext = VTask::GetCurrent()->GetErrorContext(true);
	fErrorContext = fTaskContext->PushNewContext(false /* do not keep */, true /* silent */);
}
#else
VJSParms_withException::VJSParms_withException(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException)
: VJSParms_withContext( inContext, inObject)
, fException( outException)
{
	if (fException)
	{
		int a = 1;
	}
	fTaskContext = VTask::GetCurrent()->GetErrorContext( true);
	fErrorContext = fTaskContext->PushNewContext( false /* do not keep */, true /* silent */);
}
#endif

VJSParms_withException::~VJSParms_withException()
{
#if  USE_V8_ENGINE
	fException.ConvertErrorContextToException(GetContext(), fErrorContext);
#else
	JS4D::ConvertErrorContextToException( GetContext(), fErrorContext, fException);
#endif
	xbox_assert(fErrorContext == fTaskContext->GetLastContext());
	fTaskContext->PopContext();
	fTaskContext->RecycleContext( fErrorContext);

}


void VJSParms_withException::SetException(const VJSException& inException) 
{
#if USE_V8_ENGINE
	fException = inException;
#else
	if (testAssert(fException))
	{
		*fException = inException.GetValue();
	}
#endif
}

void VJSParms_withException::EvaluateScript( const VString& inScript, VValueSingle** outResult)
{
	VJSValue value( GetContext());
#if USE_V8_ENGINE
	JS4D::EvaluateScript(GetContext(), inScript, value, &fException);
#else
	JS4D::EvaluateScript(GetContext(), inScript, value, fException);
#endif
	if (outResult != NULL)
	{
#if USE_V8_ENGINE
		*outResult = value.CreateVValue(&fException, false);
#else
		VJSException	except;
		*outResult = value.CreateVValue(&except, false);
		if (fException)
		{
			*fException = except.GetValue();
		}
#endif
	}
}


void VJSParms_withReturnValue::ReturnBool( bool inValue)							
{
#if USE_V8_ENGINE
	fV8Arguments->GetReturnValue().Set(inValue);
#else
	VJSException	except;
	fReturnValue.SetBool( inValue, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnString( const XBOX::VString& inString)		
{
#if USE_V8_ENGINE
	Local<String> newStr =
		String::NewFromTwoByte(	fV8Arguments->GetIsolate(),
								inString.GetCPointer(),
								v8::String::kNormalString,
								inString.GetLength());
	fV8Arguments->GetReturnValue().Set(newStr);
#else	
	VJSException	except;
	fReturnValue.SetString( inString, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnTime( const XBOX::VTime& inTime)				
{
#if USE_V8_ENGINE
	VTime	unixStartTime;
	unixStartTime.FromUTCTime(1970,1,1,0,0,0,0);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fV8Arguments->GetIsolate());
	HandleScope		scope(fV8Arguments->GetIsolate());
	Handle<Context>	ctx = Handle<Context>::New(fV8Arguments->GetIsolate(), *v8PersContext);
	Context::Scope	context_scope(ctx);
	double			dateMS = inTime.GetMilliseconds() - unixStartTime.GetMilliseconds();
	fV8Arguments->GetReturnValue().Set(v8::Date::New(fV8Arguments->GetIsolate(),dateMS));
#else
	VJSException	except;
	fReturnValue.SetTime( inTime, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnDuration( const XBOX::VDuration& inDuration)	
{
#if USE_V8_ENGINE
	HandleScope				scope(fV8Arguments->GetIsolate());
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fV8Arguments->GetIsolate());
	Handle<Context>	ctx = Handle<Context>::New(fV8Arguments->GetIsolate(), *v8PersContext);
	Context::Scope	context_scope(ctx);
	if (inDuration.IsNull())
	{
		fV8Arguments->GetReturnValue().SetNull();
	}
	else
	{
		fV8Arguments->GetReturnValue().Set(v8::Number::New(fV8Arguments->GetIsolate(),inDuration.ConvertToMilliseconds()));
	}
#else
	VJSException	except;
	fReturnValue.SetDuration( inDuration, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}

void VJSParms_withReturnValue::ReturnVValue( const XBOX::VValueSingle& inValue, bool simpleDate)	
{ 
#if USE_V8_ENGINE
	VJSValue	tmpVal(fV8Arguments->GetIsolate());
	tmpVal.SetVValue(inValue,NULL,simpleDate);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpVal.fValue);
#endif
	fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));
#else
	VJSException	except;
	fReturnValue.SetVValue( inValue, &except, simpleDate);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnFile( XBOX::VFile *inFile)					
{
#if USE_V8_ENGINE
	VJSValue	tmpVal(fV8Arguments->GetIsolate());
	tmpVal.SetFile(inFile);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpVal.fValue);
#endif
	fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));
#else
	VJSException	except;
	fReturnValue.SetFile( inFile, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnFolder( XBOX::VFolder *inFolder)				
{ 
#if USE_V8_ENGINE
	VJSValue	tmpVal(fV8Arguments->GetIsolate());
	tmpVal.SetFolder(inFolder);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpVal.fValue);
#endif
	fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));
#else
	VJSException	except;
	fReturnValue.SetFolder( inFolder, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnFilePathAsFileOrFolder( const XBOX::VFilePath& inPath)	
{ 
#if USE_V8_ENGINE
	if (inPath.IsFolder())
	{
		VJSContext	vjsContext(fV8Arguments->GetIsolate());
		VFolder *folder = new VFolder(inPath);
		JS4DFolderIterator*	folderIt = new JS4DFolderIterator(folder);
		VJSObject	tmpObj = VJSFolderIterator::CreateInstance(vjsContext, folderIt);
		ReleaseRefCountable( &folder);
#if V8_USE_MALLOC_IN_PLACE
		//Handle<Object>	hdl = (*ToV8Persistent(tmpObj.fObject))->ToObject();
#else		
		VJSValue tmpVal(fV8Arguments->GetIsolate());
		JS4D::ObjectToValue(tmpVal, tmpObj);
		Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpVal.fValue);
#endif
		fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpObj.fObject));
	}
	else if (inPath.IsFile())
	{
		VJSContext	vjsContext(fV8Arguments->GetIsolate());
		VFile *file = new VFile(inPath);
		JS4DFileIterator*	fileIt = new JS4DFileIterator(file);
		VJSObject	tmpObj = VJSFileIterator::CreateInstance(vjsContext, fileIt);
		ReleaseRefCountable(&file);
#if V8_USE_MALLOC_IN_PLACE
		//Handle<Object>	hdl = (*ToV8Persistent(tmpObj.fObject))->ToObject();
#else
		Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpVal.fValue);
#endif
		fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpObj.fObject));
	} else
	{
		//xbox_assert(false);
	}

#else
	VJSException	except;
	fReturnValue.SetFilePathAsFileOrFolder( inPath, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnJSONValue( const VJSONValue& inValue)			
{ 
#if USE_V8_ENGINE
	VJSValue	tmpValue(fV8Arguments->GetIsolate());
	tmpValue.SetJSONValue(inValue,NULL);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), ToV8Persistent(tmpValue.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(fV8Arguments->GetIsolate(), tmpValue.fValue);
#endif
	fV8Arguments->GetReturnValue().Set(ToV8Persistent(tmpValue.fValue));
#else
	VJSException	except;
	fReturnValue.SetJSONValue( inValue, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}
void VJSParms_withReturnValue::ReturnNullValue()
{
#if USE_V8_ENGINE
	fV8Arguments->GetReturnValue().SetNull();
#else
	fReturnValue.SetNull();
#endif
}

void VJSParms_withReturnValue::ReturnUndefinedValue()
{
#if USE_V8_ENGINE
	fV8Arguments->GetReturnValue().SetUndefined();
#else
	fReturnValue.SetUndefined();
#endif
}

void VJSParms_withReturnValue::ReturnValue( const VJSValue& inValue)
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value> handle = Handle<Value>::New(fV8Arguments->GetIsolate(), ToV8Persistent(inValue.fValue));
#else
	Handle<Value> handle = Handle<Value>::New( fV8Arguments->GetIsolate(),inValue.fValue);
#endif
	fV8Arguments->GetReturnValue().Set(ToV8Persistent(inValue.fValue));

#else
	fReturnValue = inValue;
#endif
}


#if USE_V8_ENGINE
VJSParms_hasProperty::VJSParms_hasProperty(const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Integer>& inInfo)
: VJSParms_withQueryPropertyCallbackReturnValue(&inInfo)
, fPropertyName(inPropertyName)
{
}
#else
VJSParms_hasProperty::VJSParms_hasProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName)
: VJSParms_withContext( inContext, inObject)
, fPropertyName( inPropertyName)
, fReturnedBoolean(false)			// Default is to forward to parent.
{

#if VERSIONDEBUG
	JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
#endif
}
#endif

#if USE_V8_ENGINE
VJSParms_deleteProperty::VJSParms_deleteProperty(const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Boolean>& inInfo)
: fPropertyName(inPropertyName)
, fReturnedBoolean(true)
{

}
#else
VJSParms_deleteProperty::VJSParms_deleteProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
: VJSParms_withException( inContext, inObject, outException)
, fPropertyName( inPropertyName)
, fReturnedBoolean(true)			// Default is successful deletion.
{

#if VERSIONDEBUG
	JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
#endif

}
#endif

#if USE_V8_ENGINE
#else
VJSParms_hasInstance::VJSParms_hasInstance(JS4D::ContextRef inContext, JS4D::ObjectRef inPossibleConstructor, JS4D::ValueRef inObjectToTest, JS4D::ExceptionRef *outException)
: VJSParms_withException(inContext, (JS4D::ObjectRef) inObjectToTest, outException)
, fPossibleConstructor((JS4D::ObjectRef) inPossibleConstructor)
,fObjectToTest((JS4D::ObjectRef) inObjectToTest)
{
}
#endif

#if USE_V8_ENGINE
VJSParms_getPropertyNames::VJSParms_getPropertyNames(const v8::PropertyCallbackInfo<v8::Array>& info, std::vector<XBOX::VString>& propertyNames)
: fPropertyNames(propertyNames)
, VJSParms_withContext(info)
{
}
#else
VJSParms_getPropertyNames::VJSParms_getPropertyNames( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::PropertyNameAccumulatorRef inPropertyNames)
: VJSParms_withContext( inContext, inObject)
, fPropertyNames( inPropertyNames)
{
}
#endif


size_t VJSParms_withArguments::CountParams() const					
{
#if USE_V8_ENGINE
	return fV8Arguments->Length();
#else
	return fArgumentCount;
#endif
}

VJSValue VJSParms_withArguments::GetParamValue( size_t inIndex) const
{
#if USE_V8_ENGINE
    VJSValue	result(GetContext());
	if (inIndex > 0 && inIndex <= (*fV8Arguments).Length())
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fV8Arguments->GetIsolate());
		v8::HandleScope handle_scope((*fV8Arguments).GetIsolate());
		Handle<Context>	ctx = Handle<Context>::New(fV8Arguments->GetIsolate(), *v8PersContext);
		Context::Scope	context_scope(ctx);

		//DebugMsg("VJSParms_withArguments::GetParamValue i=%d isol=%x\n",inIndex,(*fV8Arguments).GetIsolate());
		//Handle<Value> val = (*fV8Arguments)[inIndex-1];
		//String::Utf8Value valStr((*fV8Arguments)[inIndex-1]l);
		//DebugMsg("VJSParms_withArguments::GetParamValue '%s'\n",*valStr);

		JS4D::ContextRef	ctxRef = GetContext();
		xbox_assert(ctxRef == (*fV8Arguments).GetIsolate());
		Handle<Value>	locVal = (*fV8Arguments)[inIndex - 1];
		// GH trouver pourquoi necessaire alor que transporter le handle suffit

		/*TryCatch try_catch;
		{
			Local<String> str = locVal->ToString();
			String::Utf8Value  value2(locVal->ToString());
			DebugMsg("VJSParms_withArguments::GetParamValue #%d -> '%s' (isExt=%d)\n", inIndex, *value2, (locVal->IsExternal() ? 1 : 0));
		}*/
		//xbox_assert(!locVal->IsUndefined());
		xbox_assert(!locVal->IsNull());
		result = VJSValue(GetContext(), locVal);

	}
	// somtimes this fuynction is called with no param -> a NULL value is returned as in JSC
	/*else
	{
		DebugMsg("VJSParms_withArguments::GetParamValue #%d / %d NOT FOUND!!!!\n",inIndex,(*fV8Arguments).Length());
	}*/
	return result;

#else
	if (inIndex > 0 && inIndex <= fArgumentCount)	// sc 05/11/2009 inIndex is 1 based
		return VJSValue( GetContext(), fArguments[inIndex-1]);
	else
		return VJSValue( GetContext(), NULL);
#endif
}


VValueSingle *VJSParms_withArguments::CreateParamVValue( size_t inIndex, bool simpleDate) const
{
#if USE_V8_ENGINE
	if (inIndex > 0 && inIndex <= fV8Arguments->Length())
		return JS4D::ValueToVValue(GetContext(), ((*fV8Arguments)[inIndex - 1]), &fException, simpleDate);
#else
	if (inIndex > 0 && inIndex <= fArgumentCount)
		return JS4D::ValueToVValue( GetContext(), fArguments[inIndex-1], fException, simpleDate);
#endif
	return NULL;
}


bool VJSParms_withArguments::GetParamObject( size_t inIndex, VJSObject& outObject) const
{
	bool okobj = false;
	
#if USE_V8_ENGINE
	if (inIndex > 0 && inIndex <= (*fV8Arguments).Length())
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fV8Arguments->GetIsolate());
		v8::HandleScope handle_scope((*fV8Arguments).GetIsolate());
		Handle<Context>	ctx = Handle<Context>::New(fV8Arguments->GetIsolate(), *v8PersContext);
		Context::Scope	context_scope(ctx);

		JS4D::ContextRef	ctxRef = GetContext();
		xbox_assert(ctxRef == (*fV8Arguments).GetIsolate());
		Handle<Value>	locVal = (*fV8Arguments)[inIndex - 1];

		/*TryCatch try_catch;
		{
			Local<String> str = locVal->ToString();
			String::Utf8Value  value2(locVal->ToString());
			DebugMsg("VJSParms_withArguments::GetParamObject #%d -> '%s' (isExt=%d)\n", inIndex, *value2, (locVal->IsExternal() ? 1 : 0));
		}
		if (try_catch.HasCaught())
		{
			int a = 1;
		}*/
		if (locVal->IsObject())
		{
			outObject = VJSObject((*fV8Arguments).GetIsolate(), locVal->ToObject());
		}
		else
		{
			outObject = VJSObject((*fV8Arguments).GetIsolate());
		}
		okobj = true;
	}
	else
	{
		outObject = VJSObject((*fV8Arguments).GetIsolate());
	}

	//outObject = VJSObject( (*fV8Arguments).GetIsolate(), (*fV8Arguments)[inIndex-1].val_ );
#else
	if (inIndex > 0 && inIndex <= fArgumentCount && JSValueIsObject( GetContext(), fArguments[inIndex-1]) )
	{
		outObject.SetObjectRef( JSValueToObject( GetContext(), fArguments[inIndex-1], fException));
		okobj = true;
	}
	else
		outObject.SetObjectRef( NULL);
#endif	
	return okobj;
}


bool VJSParms_withArguments::GetParamJSONValue( size_t inIndex, VJSONValue& outValue) const
{
	bool ok;
#if USE_V8_ENGINE	
	if (inIndex > 0 && inIndex <= fV8Arguments->Length())
#else
	if (inIndex > 0 && inIndex <= fArgumentCount)
#endif
	{
#if USE_V8_ENGINE
		ok = JS4D::ValueToVJSONValue(GetContext(), ((*fV8Arguments)[inIndex - 1]), outValue, &fException);
#else
		ok = JS4D::ValueToVJSONValue( GetContext(), fArguments[inIndex-1], outValue, fException);
#endif
	}
	else
	{
		outValue.SetUndefined();
		ok = false;
	}
	
	return ok;
}


bool VJSParms_withArguments::GetParamFunc( size_t inIndex, VJSObject& outObject, bool allowString) const
{
	bool okfunc = false;

	VJSValue valfunc(GetParamValue(inIndex));
	if (!valfunc.IsUndefined() && valfunc.IsObject())
	{
		VJSObject objfunc(GetContext());
		valfunc.GetObject(objfunc);
		if (objfunc.IsFunction())
		{
			outObject = objfunc;
			okfunc = true;
		}
	}
	/*
	if (!okfunc && allowString)
	{
		if (IsStringParam(inIndex))
		{
		}
	}
	*/

	return okfunc;
}



bool VJSParms_withArguments::GetParamArray( size_t inIndex, VJSArray& outArray) const
{
	bool okarray = false;

	if (IsArrayParam( inIndex))
    {
#if USE_V8_ENGINE
		VJSValue	tmpVal(GetContext(), ((*fV8Arguments)[inIndex - 1]));
		VJSObject	tmpObj = tmpVal.GetObject();
		outArray = VJSArray(tmpObj);
#else
		VJSObject	tmpObj(GetContext(), JSValueToObject(GetContext(), fArguments[inIndex - 1], fException));
		outArray = VJSArray(tmpObj);
#endif
		okarray=true;
    }
	else
		outArray.SetNull();

    return okarray;
}


bool VJSParms_withArguments::IsNullOrUndefinedParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	Handle<Value>	locVal = (*fV8Arguments)[inIndex-1];
	return (*locVal)->IsUndefined() || (*locVal)->IsNull();
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? (JSValueIsNull( GetContext(), fArguments[inIndex-1]) || JSValueIsUndefined( GetContext(), fArguments[inIndex-1])) : true;
#endif
}


bool VJSParms_withArguments::IsNullParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsNull();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsNull( GetContext(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsUndefinedParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsUndefined();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsUndefined( GetContext(), fArguments[inIndex-1]) : true;
#endif
}


bool VJSParms_withArguments::IsBooleanParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsBoolean();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsBoolean( GetContext(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsNumberParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsNumber();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsNumber( GetContext(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsStringParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsString();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsString( GetContext(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsObjectParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsObject();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JSValueIsObject( GetContext(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsFunctionParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex-1]->IsFunction();
	return result;
#else
	return IsObjectParam( inIndex) && JSObjectIsFunction( GetContext(), JSValueToObject( GetContext(), fArguments[inIndex-1], NULL));
#endif
}


bool VJSParms_withArguments::IsArrayParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	bool result = (*fV8Arguments)[inIndex - 1]->IsArray();
	return result;
#else
	return (inIndex > 0 && inIndex <= fArgumentCount) ? JS4D::ValueIsInstanceOf( GetContext(), fArguments[inIndex-1], CVSTR( "Array"), NULL) : false;
#endif
}


bool VJSParms_withArguments::GetStringParam( size_t inIndex, VString& outString) const
{
#if USE_V8_ENGINE
    //DebugMsg("VJSParms_withArguments::GetStringParam for idx=%d\n",inIndex);
#endif
	VJSValue value(GetParamValue(inIndex));
#if USE_V8_ENGINE
	bool res = value.GetString(outString, &fException);
#else
	VJSException tmpExc;
	bool res = value.GetString(outString, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetLongParam( size_t inIndex, sLONG *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
#if USE_V8_ENGINE
	bool res = value.GetLong(outValue, &fException);
#else
	VJSException	tmpExc;
	bool res = value.GetLong(outValue, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetLong8Param( size_t inIndex, sLONG8 *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
#if  USE_V8_ENGINE
	bool res = value.GetLong8(outValue, &fException);
#else
	VJSException	tmpExc;
	bool res = value.GetLong8(outValue, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetULongParam( size_t inIndex, uLONG *outValue) const
{
	VJSValue value(GetParamValue(inIndex));		
#if  USE_V8_ENGINE
	bool res = value.GetULong(outValue, &fException);
#else
	VJSException	tmpExc;
	bool res = value.GetULong( outValue, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetRealParam( size_t inIndex, Real *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
#if  USE_V8_ENGINE
	bool res = value.GetReal(outValue, &fException);
#else
	VJSException	tmpExc;
	bool res = value.GetReal(outValue, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetBoolParam( size_t inIndex, bool *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
#if  USE_V8_ENGINE
	bool res = value.GetBool(outValue, &fException);
#else
	VJSException	tmpExc;
	bool res = value.GetBool(outValue, &tmpExc);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}


bool VJSParms_withArguments::GetBoolParam( size_t inIndex, const VString& inNameForTrue, const VString& inNameForFalse, size_t* outIndex) const
{
	bool result = false;
	
	if (CountParams() >= inIndex)
	{
		if (IsStringParam( inIndex))
		{
			if (outIndex != NULL)
				*outIndex = inIndex + 1;
			VString s;
			GetStringParam( inIndex, s);
			if (s == inNameForTrue)
				result = true;
		}
		else
		{
			if (IsBooleanParam( inIndex))
			{
				if (outIndex != NULL)
					*outIndex = inIndex + 1;
				GetBoolParam( inIndex, &result);
			}
		}
	}
	
	return result;
}


VFile* VJSParms_withArguments::RetainFileParam( size_t inIndex, bool allowPath) const
{
#if USE_V8_ENGINE
    //DebugMsg("VJSParms_withArguments::RetainFileParam for idx=%d\n",inIndex);
#endif
	VFile* result = NULL;
	
	if ( CountParams() >= inIndex )
	{
#if USE_V8_ENGINE
	 	VJSValue paramValue(GetContext(),(*fV8Arguments)[inIndex-1]);
		result = paramValue.GetFile( &fException);
#else
		VJSException	tmpExc;
		VJSValue paramValue( GetContext(), fArguments[inIndex-1]);
		result = paramValue.GetFile(&tmpExc);
		if (fException)
		{
			*fException = tmpExc.GetValue();
		}
#endif
		if (result != NULL)
		{
			result->Retain();
		}
		else
		{
			if (allowPath)
			{
				VURL url;
				VJSException	tmpExc;
				if (paramValue.GetURL(url, &tmpExc))
				{
					VFilePath path;
					if (url.GetFilePath(path) && path.IsFile())
					{
						VString strpath;
						//paramValue.GetString(strpath);
						path.GetPosixPath(strpath);
						result = VJSPath::ResolveFileParam(GetContext(), strpath);
						//result = new VFile( path);
					}
					else
					{
						vThrowError( VE_FILE_BAD_NAME);
					}
				}
#if USE_V8_ENGINE
#else
				if (fException)
				{
					*fException = tmpExc.GetValue();
				}
#endif
			}
				
		}
	}
	return result;
}



VFolder* VJSParms_withArguments::RetainFolderParam( size_t inIndex, bool allowPath) const
{
	VFolder* result = NULL;
	
	if (CountParams() >= inIndex)
	{
#if USE_V8_ENGINE
		VJSValue paramValue(fV8Arguments->GetIsolate(), (*fV8Arguments)[inIndex - 1]);
		result = paramValue.GetFolder(&fException);
#else
		VJSException	tmpExc;
		VJSValue paramValue( GetContext(), fArguments[inIndex-1]);
		result = paramValue.GetFolder(&tmpExc);
		if (fException)
		{
			*fException = tmpExc.GetValue();
		}
#endif
		if (result != NULL)
		{
			result->Retain();
		}
		else
		{
			if (allowPath)
			{
				VURL url;
				VJSException	tmpExc;
				if (paramValue.GetURL( url, &tmpExc))
				{
					VFilePath path;
					if (url.GetFilePath(path) && path.IsFolder())
					{
						VString strpath;
						path.GetPosixPath(strpath);
						result = VJSPath::ResolveFolderParam(GetContext(), strpath);
						//result = new VFolder(path);
					}
					else
					{
						vThrowError( VE_FILE_BAD_NAME);
					}
				}
#if  USE_V8_ENGINE
#else
				if (fException)
				{
					*fException = tmpExc.GetValue();
				}
#endif
			}
		}
	}
	return result;
}



const UniChar *VJSParms_getProperty::GetPropertyNamePtr( VIndex *outLength) const
{
#if USE_V8_ENGINE
	xbox_assert(false);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	return NULL;
#else
	if (outLength != NULL)
		*outLength = JSStringGetLength( fPropertyName);
	return JSStringGetCharactersPtr( fPropertyName);
#endif
}

//======================================================


void VJSParms_getPropertyNames::AddPropertyName(const VString& inPropertyName)
{
#if USE_V8_ENGINE
	DebugMsg("VJSParms_getPropertyNames::AddPropertyName called for '%S'\n",&inPropertyName);
	fPropertyNames.push_back(inPropertyName);
#else
	JSStringRef jsString = JS4D::VStringToString(inPropertyName);
	JSPropertyNameAccumulatorAddName( fPropertyNames, jsString);
	JSStringRelease( jsString);
#endif
}


void VJSParms_getPropertyNames::ReturnNumericalRange( size_t inBegin, size_t inEnd)
{
#if USE_V8_ENGINE
	xbox_assert(false);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );

#else
	for( size_t i = inBegin ; i != inEnd ; ++i)
	{
		size_t index = i;
		size_t length = 0;
		JSChar	temp[20];
		JSChar* current = &temp[20];
		*--current = 0;
		while(index > 0)
		{
			*--current = (JSChar) (CHAR_DIGIT_ZERO + index%10L);
			index /= 10L;
			length++;
		}

		JSStringRef jsString = JSStringCreateWithCharacters( current, length);

		JSPropertyNameAccumulatorAddName( fPropertyNames, jsString);

		JSStringRelease( jsString);
	}
#endif
}


#if USE_V8_ENGINE

VJSParms_construct::VJSParms_construct(const v8::FunctionCallbackInfo<v8::Value>& args)
:VJSParms_withArguments(args)
,VJSParms_withException(args)
{
	;
}

void VJSParms_withGetPropertyCallbackReturnValue::ReturnDouble(double inValue)
{
	DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnDouble val=%f\n", inValue);
	info->GetReturnValue().Set(inValue);
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnBool(bool inValue)
{
	info->GetReturnValue().Set(inValue);
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnString(const XBOX::VString& inString)
{
	//DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnString str=<%S>\n",&inString);
	Local<String> newStr =
		String::NewFromTwoByte(	info->GetIsolate(),
								inString.GetCPointer(),
								v8::String::kNormalString,
								inString.GetLength());
	info->GetReturnValue().Set(newStr);
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnTime(const XBOX::VTime& inTime)
{
	VTime	unixStartTime;
	unixStartTime.FromUTCTime(1970, 1, 1, 0, 0, 0, 0);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(info->GetIsolate());
	v8::HandleScope handle_scope(info->GetIsolate());
	Handle<Context> ctx = Handle<Context>::New(info->GetIsolate(), *v8PersContext);
	Context::Scope context_scope(ctx);

	double		dateMS = inTime.GetMilliseconds() - unixStartTime.GetMilliseconds();
	info->GetReturnValue().Set(v8::Date::New(info->GetIsolate(),dateMS));
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnDuration(const XBOX::VDuration& inDuration)
{
	Handle<Number>	tmpDur = v8::Number::New(info->GetIsolate(),inDuration.ConvertToMilliseconds());
	info->GetReturnValue().Set(tmpDur);

}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnVValue(const XBOX::VValueSingle& inValue, bool simpleDate)
{
	VJSValue	tmpVal(info->GetIsolate());
	tmpVal.SetVValue(inValue,NULL,simpleDate);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), tmpVal.fValue);
#endif
	info->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnFile(XBOX::VFile *inFile)
{
	VJSValue	tmpVal(info->GetIsolate());
	tmpVal.SetFile(inFile);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), tmpVal.fValue);
#endif
	info->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));
}


void VJSParms_withGetPropertyCallbackReturnValue::ReturnFolder(XBOX::VFolder *inFolder)
{
	//DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnFolder called\n");
	VJSValue	tmpVal(info->GetIsolate());
	tmpVal.SetFolder(inFolder);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), tmpVal.fValue);
#endif
	info->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));

#if 0
	HandleScope handle_scope(info->GetIsolate());
	Context::Scope context_scope(info->GetIsolate(), V4DContext::GetPersistentContext(info->GetIsolate()));
	V4DContext*	v8Ctx = (V4DContext*)(info->GetIsolate()->GetData());
	JS4D::ClassDefinition*	clDef = (JS4D::ClassDefinition*)VJSFolderIterator::Class();
	Handle<Object> locFunc = v8Ctx->GetFunction(clDef->className);

	JS4DFolderIterator* folder = (inFolder == NULL) ? NULL : new JS4DFolderIterator(inFolder);
	
	//DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnFolder setting prdData=%x\n", folder);
	//Handle<External> extVal = External::New((void*)folder);
	Handle<Value> valuesArray[32];
	//valuesArray[0] = extVal;
	for (int idx = 0; idx<32; idx++)
	{
		valuesArray[idx] = v8::Integer::New(0);
	}
	Local<Value> locVal = locFunc->CallAsConstructor(32, valuesArray);
	if (locVal.val_)
	{
		if (locVal.val_->IsObject())
		{
			V4DContext::SetObjectPrivateData(info->GetIsolate(), locVal.val_, folder);

			Local<Object>	obj = locVal.val_->ToObject();
			Local<String>		constrName = obj->GetConstructorName();
			String::Utf8Value	utf8Name(constrName);
			//DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnFolder constrName=%s isFunc=%d\n", *utf8Name, locVal.val_->IsFunction());
		}
	}
	else
	{
		//DebugMsg("VJSParms_withGetPropertyCallbackReturnValue::ReturnFolder NULL obj\n");
	}
	info->GetReturnValue().Set(locVal);
	//info->GetReturnValue().Set(String::New("ABCDEFGHI"));
	//	V4DContext::SetObjectPrivateData(inContext,ioValue,folder);
	/*	VJSException	except;
	fReturnValue.SetFolder( inFolder, &except);
	if (fException)
	{
	*fException = except.GetValue();
	}*/
#endif
}


void VJSParms_withGetPropertyCallbackReturnValue::ReturnFilePathAsFileOrFolder(const XBOX::VFilePath& inPath)
{
	xbox_assert(false);
	for (;;)
		DebugMsg("%s  NOT IMPL\n", __PRETTY_FUNCTION__);
	xbox_assert(false);
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnJSONValue(const VJSONValue& inValue)
{
	xbox_assert(false);
	for (;;)
		DebugMsg("%s  NOT IMPL\n", __PRETTY_FUNCTION__);
	xbox_assert(false);
	/*	VJSException	except;
	fReturnValue.SetJSONValue( inValue, &except);
	if (fException)
	{
	*fException = except.GetValue();
	}*/
}

void VJSParms_withGetPropertyCallbackReturnValue::ReturnValue(const VJSValue& inValue)
{
	fReturnValue = inValue;
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value> handle = Handle<Value>::New(info->GetIsolate(), ToV8Persistent(inValue.fValue));
#else
	Handle<Value> handle = Handle<Value>::New(info->GetIsolate(), inValue.fValue);
#endif
	info->GetReturnValue().Set(ToV8Persistent(inValue.fValue));
}

void VJSParms_withGetPropertyCallbackReturnValue::ReturnNullValue()
{
	info->GetReturnValue().SetNull();
}
void VJSParms_withGetPropertyCallbackReturnValue::ReturnUndefinedValue()
{
	info->GetReturnValue().SetUndefined();
}

const VJSValue&	VJSParms_withGetPropertyCallbackReturnValue::GetReturnValue() const
{
	return fReturnValue;
}

void VJSParms_withSetPropertyCallbackReturnValue::ReturnDouble(double inValue)
{
	DebugMsg("VJSParms_withSetPropertyCallbackReturnValue::ReturnDouble val=%f\n", inValue);
	if (info)
	{
		info->GetReturnValue().Set(inValue);
	}
	else
	{
		infoVal->GetReturnValue().Set(inValue);
	}
}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnBool(bool inValue)
{
	if (info)
	{
		info->GetReturnValue().Set(inValue);
	}
	else
	{
		infoVal->GetReturnValue().Set(inValue);
	}
}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnString(const XBOX::VString& inString)
{
	Local<String> newStr =
		String::NewFromTwoByte(	info->GetIsolate(),
								inString.GetCPointer(),
								v8::String::kNormalString,
								inString.GetLength());
	if (info)
	{
		info->GetReturnValue().Set(newStr);
	}
	else
	{
		infoVal->GetReturnValue().Set(newStr);
	}
}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnTime(const XBOX::VTime& inTime)
{
	VTime	unixStartTime;
	unixStartTime.FromUTCTime(1970,1,1,0,0,0,0);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(info->GetIsolate());
	v8::HandleScope handle_scope(info->GetIsolate());
	Handle<Context>	ctx = Handle<Context>::New(info->GetIsolate(), *v8PersContext);
	Context::Scope context_scope(ctx);
	double		dateMS = inTime.GetMilliseconds() - unixStartTime.GetMilliseconds();

	if (info)
	{
		info->GetReturnValue().Set(v8::Date::New(info->GetIsolate(),dateMS));
	}
	else
	{
		infoVal->GetReturnValue().Set(v8::Date::New(info->GetIsolate(),dateMS));
	}
}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnDuration(const XBOX::VDuration& inDuration)
{
	xbox_assert(false);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );

}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnVValue(const XBOX::VValueSingle& inValue, bool simpleDate)
{
	xbox_assert(false);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );

}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnFile(XBOX::VFile *inFile)
{
	xbox_assert(false);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );

}


void VJSParms_withSetPropertyCallbackReturnValue::ReturnFolder(XBOX::VFolder *inFolder)
{ 
	//DebugMsg("VJSParms_withSetPropertyCallbackReturnValue::ReturnFolder called\n");
	VJSValue	tmpVal(info->GetIsolate());
	tmpVal.SetFolder(inFolder);
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), ToV8Persistent(tmpVal.fValue));
#else
	Handle<Value>	hdl = Handle<Value>::New(info->GetIsolate(), tmpVal.fValue);
#endif
	info->GetReturnValue().Set(ToV8Persistent(tmpVal.fValue));

#if 0
	HandleScope handle_scope(info->GetIsolate());
    Context::Scope context_scope(info->GetIsolate(),V4DContext::GetPersistentContext(info->GetIsolate()));
	V4DContext*	v8Ctx = (V4DContext*)(info->GetIsolate()->GetData());
	JS4D::ClassDefinition*	clDef = (JS4D::ClassDefinition*)VJSFolderIterator::Class();
	Handle<Object> locFunc = v8Ctx->GetFunction(clDef->className);

	//Persistent<FunctionTemplate>*	pFT = v8Ctx->GetImplicitFunctionConstructor(clDef->className);
	//Local<Function> locFunc = (pFT->val_)->GetFunction();
	JS4DFolderIterator* folder = (inFolder == NULL) ? NULL : new JS4DFolderIterator( inFolder);
    //DebugMsg("VJSParms_withSetPropertyCallbackReturnValue::ReturnFolder setting prdData=%x\n",folder);
	//Handle<External> extVal = External::New((void*)folder);
    Handle<Value> valuesArray[32];
	//valuesArray[0] = extVal;
    for( int idx=0; idx<32; idx++ )
    {
        valuesArray[idx] = v8::Integer::New(0);
    }
    Local<Value> locVal = locFunc->CallAsConstructor(32,valuesArray);
    if (locVal.val_)
    {
        if (locVal.val_->IsObject())
        {
			V4DContext::SetObjectPrivateData(info->GetIsolate(),locVal.val_,folder);

			Local<Object>	obj = locVal.val_->ToObject();
			Local<String>		constrName = obj->GetConstructorName();
			String::Utf8Value	utf8Name(constrName);
			//DebugMsg("VJSParms_withSetPropertyCallbackReturnValue::ReturnFolder constrName=%s isFunc=%d\n",*utf8Name,locVal.val_->IsFunction());
        }
    }
    else
    {
        //DebugMsg("VJSParms_withSetPropertyCallbackReturnValue::ReturnFolder NULL obj\n");
    }
	if (info)
	{
		info->GetReturnValue().Set(locVal);
	}
	else
	{
		infoVal->GetReturnValue().Set(locVal);
	}
#endif
}


void VJSParms_withSetPropertyCallbackReturnValue::ReturnFilePathAsFileOrFolder(const XBOX::VFilePath& inPath)
{ 
	xbox_assert(false);
	for (;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	xbox_assert(false);

}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnJSONValue(const VJSONValue& inValue)
{ 
	xbox_assert(false);
	for (;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	xbox_assert(false);

}

void VJSParms_withSetPropertyCallbackReturnValue::ReturnValue(const VJSValue& inValue)
{ 
#if V8_USE_MALLOC_IN_PLACE
	//Handle<Value> handle = Handle<Value>::New( info->GetIsolate(), ToV8Persistent(inValue.fValue));
#else
	Handle<Value> handle = Handle<Value>::New( info->GetIsolate(),inValue.fValue);
#endif
	if (info)
	{
		info->GetReturnValue().Set(ToV8Persistent(inValue.fValue));
	}
	else
	{
		infoVal->GetReturnValue().Set(ToV8Persistent(inValue.fValue));
	}
}

void VJSParms_withSetPropertyCallbackReturnValue::ReturnNullValue()
{
	if (info)
	{
		info->GetReturnValue().SetNull();
	}
	else
	{
		infoVal->GetReturnValue().SetNull();
	}
}
void VJSParms_withSetPropertyCallbackReturnValue::ReturnUndefinedValue()
{ 
	if (info)
	{
		info->GetReturnValue().SetUndefined();
	}
	else
	{
		infoVal->GetReturnValue().SetUndefined();
	}
}

const VJSValue&	VJSParms_withSetPropertyCallbackReturnValue::GetReturnValue() const
{
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	xbox_assert(false);
	//return fReturnValue;
}

void VJSParms_withReturnValue::ReturnDouble( const double& inValue)
{
	fV8Arguments->GetReturnValue().Set(inValue);
}


void VJSParms_callStaticFunction::ReturnRawValue( void* inData )
{
	xbox_assert(false);
	for (;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
//	VJSObject	vjsObj(fThis);
//	vjsObj.SetPrivateData(inData);
}

void VJSParms_callStaticFunction::ReturnThis()
{
	VJSValue	val(fThis);
	ReturnValue(val);
}

void VJSParms_callAsFunction::ReturnThis()
{
	VJSValue	val(fThis);
	ReturnValue(val);
}


VJSParms_withQueryPropertyCallbackReturnValue::VJSParms_withQueryPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<v8::Integer>* info)
: info(info)
{
	;
}

VJSParms_withGetPropertyCallbackReturnValue::VJSParms_withGetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<v8::Value>* info)
:VJSParms_withException(info)
,info(info)
,fReturnValue(info->GetIsolate())
 {
	;
 }

VJSParms_withSetPropertyCallbackReturnValue::VJSParms_withSetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<void>* info)
:VJSParms_withException(info)
, info(info)
, infoVal(NULL)
{
	;
}

VJSParms_withSetPropertyCallbackReturnValue::VJSParms_withSetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<Value>* info)
:VJSParms_withException(info)
, info(NULL)
, infoVal(info)
{
	;
}
VJSParms_withReturnValue::VJSParms_withReturnValue(const v8::FunctionCallbackInfo<v8::Value>& args)
: fV8Arguments(&args)
, VJSParms_withException(args)
 {
	 ;
 }
 
VJSParms_withReturnValue::VJSParms_withReturnValue( const v8::FunctionCallbackInfo<v8::Value>& args,v8::Value* inConstructedValue  ) 
:fV8Arguments(&args)
, VJSParms_withException(args)
 {
	 ;
 }

VJSParms_withArguments::VJSParms_withArguments( const v8::FunctionCallbackInfo<v8::Value>& args )
: fV8Arguments(&args)
, VJSParms_withException(args)
,fConstructedObject(args.GetIsolate(),args.Holder())
{
	;
}


VJSParms_callStaticFunction::VJSParms_callStaticFunction( const v8::FunctionCallbackInfo<v8::Value>& args )
: VJSParms_withArguments(args)
, VJSParms_withReturnValue(args)
, VJSParms_withException(args)
, fThis(args.GetIsolate(),args.Holder())
{
	;
}

VJSParms_callAsFunction::VJSParms_callAsFunction( const v8::FunctionCallbackInfo<v8::Value>& args ) 
: VJSParms_withArguments(args)
, VJSParms_withReturnValue(args)
, VJSParms_withException(args)
, fFunction(args.GetIsolate(), args.Holder())    //TBC
, fThis(args.GetIsolate(), args.This())
{
	xbox::DebugMsg("VJSParms_callAsFunction::VJSParms_callAsFunction Holder is callable=%d\n", (*args.Holder())->IsCallable());
	if ((*args.Holder())->IsCallable())
	{
		int a = 1;
	}
}


VJSParms_callAsConstructor::VJSParms_callAsConstructor(const v8::FunctionCallbackInfo<v8::Value>& args )
: VJSParms_withArguments(args)
, VJSParms_withException(args)
{
    if ((*args.Holder()))
    {
        if (args.Holder()->IsObject())
        {
            Local<Object>	obj = args.Holder()->ToObject();
            Local<String>		constrName = obj->GetConstructorName();
            String::Utf8Value	utf8Name(constrName);
            xbox::DebugMsg(":VJSParms_callAsConstructor:VJSParms_callAsConstructor called  nbArgs=%d isConstruct=%d const=%s\n",
                args.Length(),
                (args.IsConstructCall() ? 1 : 0),
                *utf8Name)	;
        }
    }
    xbox::DebugMsg(":VJSParms_callAsConstructor:VJSParms_callAsConstructor called  nbArgs=%d isConstruct=%d \n",
        args.Length(),
        (args.IsConstructCall() ? 1 : 0),
        args.Holder()->InternalFieldCount())	;
}


void VJSParms_callAsConstructor::ReturnUndefined()
{
}

void VJSParms_callAsConstructor::ReturnConstructedObject(const VJSObject& inObject)
{
	fConstructedObject = inObject;
}


VJSParms_setProperty::VJSParms_setProperty(const XBOX::VString& inPropertyName, const v8::Handle<v8::Value>& inValue, const v8::PropertyCallbackInfo<void>& info)
:VJSParms_withSetPropertyCallbackReturnValue(&info)
,VJSParms_withException(info)
,fValue(info.GetIsolate(),inValue)
,fPropertyName( inPropertyName)
{
	//DebugMsg("VJSParms_setProperty::VJSParms_setProperty called for '%S'\n",&inPropertyName);
}

VJSParms_setProperty::VJSParms_setProperty(const XBOX::VString& inPropertyName, const v8::Handle<v8::Value>& inValue, const v8::PropertyCallbackInfo<Value>& info)
:VJSParms_withSetPropertyCallbackReturnValue(&info)
,VJSParms_withException(&info)
,fValue(info.GetIsolate(), inValue)
,fPropertyName(inPropertyName)
{
	//DebugMsg("VJSParms_setProperty::VJSParms_setProperty2 called for '%S'\n", &inPropertyName);
}

VJSParms_getProperty::VJSParms_getProperty(const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Value>& info)
:VJSParms_withGetPropertyCallbackReturnValue(&info)
,VJSParms_withException(&info)
,fPropertyName( inPropertyName)
{
	//DebugMsg("VJSParms_getProperty::VJSParms_getProperty called for '%S'\n",&inPropertyName);
}


bool VJSParms_getProperty::GetPropertyName( XBOX::VString& outPropertyName) const
{ 
	outPropertyName = fPropertyName;
	return true;
}

bool VJSParms_getProperty::GetPropertyNameAsLong( sLONG *outValue) const
{ 
	return JS4D::StringToLong(fPropertyName, outValue);
}




#else

VJSParms_withReturnValue::VJSParms_withReturnValue( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException)
: VJSParms_withException( inContext, inObject, outException)
, fReturnValue( inContext)
{
	;
}

VJSParms_withArguments::VJSParms_withArguments( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
: VJSParms_withException( inContext, inObject, outException)
, fArgumentCount( inArgumentCount)
, fArguments( inArguments)
{
	;
}

VJSParms_callStaticFunction::VJSParms_callStaticFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
: VJSParms_withArguments( inContext, inFunction, inArgumentCount, inArguments, outException)
, VJSParms_withReturnValue( inContext, inFunction, outException)
, VJSParms_withException( inContext, inFunction, outException)
, fThis( inContext, inThis)
{
	;
}

JS4D::ValueRef VJSParms_callStaticFunction::GetValueRefOrNull(JS4D::ContextRef inContext)
{
	JS4D::ValueRef tmpRef = GetReturnValue().GetValueRef();
	return (tmpRef != NULL) ? tmpRef : (JS4D::MakeNull(inContext)).fValue;
}


VJSParms_callAsFunction::VJSParms_callAsFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
: VJSParms_withArguments( inContext, inFunction, inArgumentCount, inArguments, outException)
, VJSParms_withReturnValue( inContext, inFunction, outException)
, VJSParms_withException( inContext, inFunction, outException)
, fFunction (inContext, inFunction)
, fThis( inContext, inThis)
{
	;
}
JS4D::ValueRef VJSParms_callAsFunction::GetValueRefOrNull(JS4D::ContextRef inContext) 
{
	JS4D::ValueRef tmpRef = GetReturnValue().GetValueRef();
	return (tmpRef != NULL) ? tmpRef : (JS4D::MakeNull(inContext)).fValue;
}

VJSParms_callAsConstructor::VJSParms_callAsConstructor( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
: VJSParms_withArguments( inContext, inConstructor, inArgumentCount, inArguments, outException)
, VJSParms_withException( inContext, inConstructor, outException)
, fConstructedObject( inContext)
{
	;
}

VJSParms_construct::VJSParms_construct( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
: VJSParms_withArguments( inContext, inConstructor, inArgumentCount, inArguments, outException)
, VJSParms_withException( inContext, inConstructor, outException)
, fConstructedObject( inContext)
{
	;
}

VJSParms_getProperty::VJSParms_getProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
: VJSParms_withReturnValue( inContext, inObject, outException)
, VJSParms_withException( inContext, inObject, outException)
, fPropertyName( inPropertyName)
{
#if VERSIONDEBUG
	JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
#endif
}

bool VJSParms_getProperty::GetPropertyName( XBOX::VString& outPropertyName) const
{ 
	return JS4D::StringToVString( fPropertyName, outPropertyName);
}
bool VJSParms_getProperty::GetPropertyNameAsLong( sLONG *outValue) const
{ 
	return JS4D::StringToLong( fPropertyName, outValue);
}

VJSParms_setProperty::VJSParms_setProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ValueRef inValue, JS4D::ExceptionRef* outException)
: VJSParms_withException( inContext, inObject, outException)
, fPropertyName( inPropertyName)
, fValue( inContext, inValue)
{
#if VERSIONDEBUG
	JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
#endif
}

#endif


END_TOOLBOX_NAMESPACE

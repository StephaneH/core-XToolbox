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
#include <V4DContext.h>
#include <v8.h>
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
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE

#if USE_V8_ENGINE

#if 0// USE_V8_ENGINE
void VJSPersistentValue::SetValue(const VJSValue& inValue)
{
	Persistent<Value>*	pVal = (Persistent<Value>*)fPersistentValue;
	Handle<Value> tmpVal = Handle<Value>::New(fContext, inValue.GetValueRef());
	pVal->Reset(fContext, tmpVal);
}
VJSPersistentValue::VJSPersistentValue(const VJSContext& inContext)
:fContext(inContext)
{
	fPersistentValue = (void*) new Persistent<Value>();
}
VJSValue VJSPersistentValue::GetValue()
{
	Persistent<Value>*	pVal = (Persistent<Value>*)fPersistentValue;
	return VJSValue(fContext, pVal->val_);
}
void VJSPersistentValue::SetNull()
{
	Persistent<Value>*	pVal = (Persistent<Value>*)fPersistentValue;
	pVal->Reset();
}
#endif

void VJSValue::Dispose()
{
	if (fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fValue));
		fValue = NULL;
	}
}

void VJSValue::CopyFrom(JS4D::ValueRef inOther)
{
	Dispose();
	if (inOther)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther);
		fValue = reinterpret_cast<Value*>(V8::CopyPersistent(p));
	}
}

void VJSValue::CopyFrom(const VJSValue& inOther)
{
	xbox_assert(fContext == inOther.fContext);
	Dispose();
	if (inOther.fValue)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fValue);
		fValue = reinterpret_cast<Value*>(V8::CopyPersistent(p));
	}
}

static void ghCbk(const WeakCallbackData<Value, void>& data)
{
	int a = 1;
}
static void ghRevivCbk(const WeakCallbackData<Value, void>& data)
{
	intptr_t memAmount = data.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(-1 * 2048);
	DebugMsg("ghRevivCbk memAmount=%u\n", (uintptr_t)memAmount);
	Value*	tmpVal = data.GetValue().val_;
	void* tmp = data.GetParameter();
	int a = 1;
}
//WeakReferenceCallbacks<Value, void>::Revivable

void VJSValue::Set(JS4D::ValueRef inValue)
{
	xbox_assert(fValue == NULL);
	if (inValue)
	{
		intptr_t memAmount = fContext->AdjustAmountOfExternalAllocatedMemory(1 * 2048);
		//DebugMsg("VJSValue::Set memAmount=%u\n", (uintptr_t)memAmount);
		internal::Object** p = reinterpret_cast<internal::Object**>(inValue);
		fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext),
			p));
		V8::MakeWeak(
			reinterpret_cast<internal::Object**>(fValue),
			NULL,
			NULL,
			reinterpret_cast<V8::RevivableCallback>(ghRevivCbk));
	}
}

VJSValue::~VJSValue()
{
	//DebugMsg("VJSValue::~VJSValue( CALLED!!!!\n");
	Dispose();
}

VJSValue::VJSValue(const VJSContext& inContext)
:fContext( inContext)
,fValue( NULL)
{
    //DebugMsg("VJSValue::VJSValue1( CALLED!!!!\n");
}

VJSValue::VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException )
:fContext(inContext)
,fValue( NULL)
{
	//DebugMsg("VJSValue::VJSValue2( CALLED!!!!\n");
}

VJSValue::VJSValue( const VJSValue& inOther)
:fContext( inOther.fContext)
,fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue3( CALLED!!!!\n");
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	CopyFrom(inOther);

}

VJSValue&	VJSValue::operator=( const VJSValue& inOther)
{
	//DebugMsg("VJSValue::VJSValue3bin( CALLED!!!!\n");
	fContext = inOther.fContext;
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext, *v8PersContext);

	CopyFrom(inOther);

	return *this;
}

VJSValue::VJSValue( JS4D::ContextRef inContext) 
:fContext( inContext)
,fValue( NULL)
{
	//DebugMsg("VJSValue::VJSValue4( CALLED!!!!\n");
	if (!inContext)
	{
		xbox_assert(inContext);
	}
}

VJSValue::VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue)
:fContext( inContext)
,fValue(NULL)
{
    //DebugMsg("VJSValue::VJSValue5( CALLED!!!!\n");
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext, *v8PersContext);

	Set(inValue);
}

VJSValue::VJSValue( JS4D::ContextRef inContext, JS4D::HandleValueRef inValue)
:fContext(inContext)
,fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue6( CALLED!!!!\n");
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	CopyFrom(inValue->val_);

}


VJSValue::operator JS4D::ValueRef() const
{
	return fValue;
}
JS4D::ValueRef		VJSValue::GetValueRef() const 
{
	return fValue;
}

			
bool VJSValue::IsUndefined() const
{
    return JS4D::ValueIsUndefined( fContext, fValue );
}
bool VJSValue::IsNull() const
{
    return JS4D::ValueIsNull( fContext, fValue );
}
bool VJSValue::IsNumber() const
{ 
	return JS4D::ValueIsNumber( fContext,fValue );
}
bool VJSValue::IsBoolean() const
{ 
	return JS4D::ValueIsBoolean( fContext,fValue );
}
bool VJSValue::IsString() const		
{ 
	return JS4D::ValueIsString( fContext,fValue );
}
bool VJSValue::IsObject() const		
{ 
	return JS4D::ValueIsObject( fContext,fValue );
}
bool VJSValue::IsArray() const
{
    return fValue ? false : fValue->IsArray();
}

bool VJSValue::IsInstanceOfBoolean() const
{
	return fValue ? false : fValue->IsBooleanObject();
}
bool VJSValue::IsInstanceOfNumber() const
{
	return fValue ? false : fValue->IsNumberObject();

}
bool VJSValue::IsInstanceOfString() const
{
	return fValue ? false : fValue->IsStringObject();
}
bool VJSValue::IsInstanceOfDate() const
{
	return fValue ? false : fValue->IsDate();
}
bool VJSValue::IsInstanceOfRegExp() const
{
	return fValue ? false : fValue->IsRegExp();
}


void *VJSValue::InternalGetObjectPrivateData( 	JS4D::ClassRef 	inClassRef,
                                                VJSException*	outException,
                                                JS4D::ValueRef	inValueRef ) const
{
	V4DContext*	v8Ctx = (V4DContext*)fContext->GetData();
	void*		data = v8Ctx->GetObjectPrivateData(fContext, inValueRef);
    return data;
}
#endif

bool VJSValue::IsInstanceOf( const XBOX::VString& inConstructorName, VJSException* outException ) const
{
#if USE_V8_ENGINE
	VStringConvertBuffer	bufferTmp(inConstructorName,VTC_UTF_8);
	if (fValue->IsObject())
	{
		xbox_assert(v8::Isolate::GetCurrent() == fContext);
		V4DContext*				v8Ctx = (V4DContext*)fContext->GetData();
		Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(fContext);
		HandleScope				handleScope(fContext);
		Local<Context>			locCtx = Handle<Context>::New(fContext,*v8PersContext);
		Context::Scope			context_scope(locCtx);
		Handle<Object> obj = fValue->ToObject();
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Utf8Value  valueStr( constrName );
		if (!strcmp(bufferTmp.GetCPointer(),*valueStr))
		{
			return true;
		}
		DebugMsg("VJSValue::IsInstanceOf strcmp NOK for '%s'\n",bufferTmp.GetCPointer());
	}
	else
	{
		//for(;;)
		DebugMsg("VJSValue::IsInstanceOf NOK for '%s'\n",bufferTmp.GetCPointer());
	}
	return false;
#else
	return IsInstanceOf( inConstructorName, VJSException::GetExceptionRefIfNotNull(outException) );
#endif
}

bool VJSValue::GetJSONValue( VJSONValue& outValue) const {
	return GetJSONValue( outValue, (JS4D::ExceptionRef *)NULL );
}

bool VJSValue::GetJSONValue( VJSONValue& outValue, VJSException* outException) const { 
	return GetJSONValue( outValue, VJSException::GetExceptionRefIfNotNull(outException) );
}

bool VJSValue::GetString( VString& outString, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (JS4D::ValueIsNull(fContext,fValue))
	{
		return false;
	}
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	if (fValue)
	{
		Local<String> cvStr = fValue->ToString();
		String::Utf8Value  value2(cvStr);
		outString = *value2;
		//DebugMsg("VJSValue::GetString CALLED <%s>",*value2);
	}
	return (fValue != NULL);
#else
	return (fValue != NULL) ? JS4D::ValueToString( fContext, fValue, outString, outException) : false;
#endif
}


bool VJSValue::GetLong( sLONG *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	if (fValue)
	{
		Local<Number>	nb = fValue->ToNumber();
		double r = nb->Value();
		sLONG l = (sLONG)r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG l = (sLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#endif
	return (fValue != NULL) && ((outException == NULL) || (*outException == NULL));
}


bool VJSValue::GetLong8( sLONG8 *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	if (fValue)
	{
		Local<Number>	nb = fValue->ToNumber();
		double r = nb->Value();
		sLONG8 l = (sLONG8)r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG8 l = (sLONG8) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#endif
	return (fValue != NULL) && ((outException == NULL) || (*outException == NULL));
}


bool VJSValue::GetULong(uLONG *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	if (fValue)
	{
		Local<Number>	nb = fValue->ToNumber();
		double r = nb->Value();
		uLONG l = (uLONG)r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		uLONG l = (uLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
#endif
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
}



bool VJSValue::GetReal( Real *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (fValue != NULL)
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope scope(fContext);
		Context::Scope context_scope(fContext, *v8PersContext);
		Local<Number>	nb = fValue->ToNumber();
		*outValue = nb->Value();
		return true;
	}
	return false;
#else
	if (fValue != NULL)
		*outValue = JSValueToNumber( fContext, fValue, outException);
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
#endif
}


bool VJSValue::GetBool( bool *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (fValue != NULL)
	{
		HandleScope scope(fContext);
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		Context::Scope context_scope(fContext, *v8PersContext);
		*outValue = fValue->BooleanValue();
		return true;
	}
	return false;
#else
	if (fValue != NULL)
		*outValue = JSValueToBoolean( fContext, fValue);
	return (fValue != NULL);
#endif
}


bool VJSValue::GetTime( VTime& outTime, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	if (fValue->IsDate())//fPersValue->val_->IsDate())
	{
		//v8::Date*	dateObject = v8::Date::Cast(fPersValue->val_); 
		ok = JS4D::DateObjectToVTime( fContext, fValue, outTime, outException, false);
	}
#else
	if (JS4D::ValueIsInstanceOf( fContext, fValue, CVSTR( "Date"), outException))
	{
		JSObjectRef dateObject = JSValueToObject( fContext, fValue, outException);
		ok = JS4D::DateObjectToVTime( fContext, dateObject, outTime, outException, false);
	}
#endif
	else
	{
		outTime.SetNull( true);
		ok = false;
	}

	return ok;
}


bool VJSValue::GetDuration( VDuration& outDuration, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,*v8PersContext);
	Handle<Value>	tmpVal = Handle<Value>::New(fContext,fValue);
	ok = JS4D::ValueToVDuration(fContext, tmpVal, outDuration, outException);

#else
	ok = JS4D::ValueToVDuration( fContext, fValue, outDuration, outException);
#endif
	return ok;
}


bool VJSValue::GetURL( XBOX::VURL& outURL, JS4D::ExceptionRef *outException) const
{
	VString path;
	if (!GetString( path, outException) && !path.IsEmpty())
		return false;
	
	JS4D::GetURLFromPath( path, outURL);

	return true;
}


VFile *VJSValue::GetFile( JS4D::ExceptionRef *outException) const
{
	JS4DFileIterator *fileIter = NULL;
#if USE_V8_ENGINE

	fileIter = GetObjectPrivateData<VJSFileIterator>(NULL);

	return (fileIter == NULL) ? NULL : fileIter->GetFile();
#else
	if (outException)
	{
		VJSException	except;
		fileIter = GetObjectPrivateData<VJSFileIterator>(&except);
		*outException = except.GetValue();
	}
	else
	{
		fileIter = GetObjectPrivateData<VJSFileIterator>(NULL);
	}
	return (fileIter == NULL) ? NULL : fileIter->GetFile();
#endif
}


VFolder* VJSValue::GetFolder( JS4D::ExceptionRef *outException) const
{
	JS4DFolderIterator *folderIter = NULL;
#if USE_V8_ENGINE
	folderIter = GetObjectPrivateData<VJSFolderIterator>(NULL);

	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
#else
	if (outException)
	{
		VJSException	except;
		folderIter = GetObjectPrivateData<VJSFolderIterator>(&except);
		*outException = except.GetValue();
	}
	else
	{
		folderIter = GetObjectPrivateData<VJSFolderIterator>(NULL);
	}
	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
#endif
}


bool VJSValue::GetJSONValue( VJSONValue& outValue, JS4D::ExceptionRef *outException) const
{
	bool ok = JS4D::ValueToVJSONValue( fContext, fValue, outValue, outException);
	return ok;
}

void VJSValue::GetObject( VJSObject& outObject, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE

	/*HandleScope			handle_scope(fContext);
	Handle<Context>		context = Handle<Context>::New(fContext,V4DContext::GetPersistentContext(fContext));
	Context::Scope		context_scope(context);
	Handle<Primitive>	undefVal = v8::Undefined(fContext);*/

	outObject.CopyFrom(fValue);
	/*outObject.Dispose();
	if (fValue)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(fValue);
		outObject.fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}*/
#else
	xbox_assert( outObject.fContext.fContext == outObject.fContext.fContext );
	outObject.SetObjectRef( ((fValue != NULL) && JSValueIsObject( fContext, fValue)) ? JSValueToObject( fContext, fValue, outException) : NULL);

	/*outObject = VJSObject(
					outObject.fContext.fContext ,
					( (fValue != NULL) && JSValueIsObject( fContext, fValue) )
					? JSValueToObject( fContext, fValue, outException)
					: NULL);*/
#endif
}


VJSObject VJSValue::GetObject( VJSException* outException ) const
{
	return GetObject( VJSException::GetExceptionRefIfNotNull(outException) );
}

VJSObject VJSValue::GetObject( JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (fValue && fValue->IsObject())
	{
		VJSObject resultObj(fContext,fValue);
		return resultObj;
	}
	else
	{
		VJSObject resultObj(fContext);
		resultObj.MakeEmpty();
		return resultObj;
	}
#else
	VJSObject resultObj(fContext);
	if ((fValue != NULL) && JSValueIsObject( fContext, fValue))
		resultObj.SetObjectRef( JSValueToObject( fContext, fValue, outException));
	else
		//resultObj.SetObjectRef(JSValueToObject(fContext, JSValueMakeNull(fContext), outException));
		resultObj.MakeEmpty();
	return resultObj;
#endif
}

#if USE_V8_ENGINE

void VJSValue::SetUndefined( VJSException *outException)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope			handle_scope(fContext);
    Handle<Context>		context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope		context_scope(context);
	Handle<Primitive>	undefVal = v8::Undefined(fContext);
	
	Dispose();
	Set(undefVal.val_);
}

void VJSValue::SetNull( VJSException *outException)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope			handle_scope(fContext);
    Handle<Context>		context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope		context_scope(context);
	Handle<Primitive>	nullVal = v8::Null(fContext);
	
	Dispose();
	Set(nullVal.val_);
}

void VJSValue::SetFile( XBOX::VFile *inFile, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFileToObject(tmpObj, inFile, VJSException::GetExceptionRefIfNotNull(outException));
	JS4D::ObjectToValue(*this, tmpObj);
}

void VJSValue::SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFilePathToObjectAsFileOrFolder(	tmpObj,
											inPath,
											VJSException::GetExceptionRefIfNotNull(outException));
	JS4D::ObjectToValue(*this, tmpObj);
}


void VJSValue::SetString( const XBOX::VString& inString, VJSException *outException)
{ 
	JS4D::VStringToValue(inString,*this);
}


void VJSValue::SetTime( const XBOX::VTime& inTime, VJSException *outException, bool simpleDate)
{ 
	JS4D::VTimeToObject( fContext, inTime, fValue, VJSException::GetExceptionRefIfNotNull(outException), simpleDate ); 

}

void VJSValue::SetDuration( const XBOX::VDuration& inDuration, VJSException *outException)
{ 
	JS4D::VDurationToValue( fContext, inDuration,*this); 
}


#else

void VJSValue::SetUndefined( VJSException *outException)
{
	fValue = (JS4D::MakeUndefined( fContext)).fValue;
}

void VJSValue::SetNull( VJSException *outException)
{
	fValue = (JS4D::MakeNull( fContext)).fValue;
}

void VJSValue::SetFile( XBOX::VFile *inFile, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFileToObject(tmpObj, inFile, VJSException::GetExceptionRefIfNotNull(outException));
	fValue = JS4D::ObjectToValue(fContext, tmpObj.fObject);
}

void VJSValue::SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFilePathToObjectAsFileOrFolder(
		tmpObj,
		inPath,
		VJSException::GetExceptionRefIfNotNull(outException));
	fValue = JS4D::ObjectToValue(fContext, tmpObj.fObject);
}


void VJSValue::SetString( const XBOX::VString& inString, VJSException *outException)
{
	fValue = JS4D::VStringToValue( fContext, inString);
}


void VJSValue::SetTime( const XBOX::VTime& inTime, VJSException *outException, bool simpleDate)
{ 
	fValue = JS4D::VTimeToObject(fContext, inTime, VJSException::GetExceptionRefIfNotNull(outException), simpleDate);
}

void VJSValue::SetDuration( const XBOX::VDuration& inDuration, VJSException *outException)
{ 
	fValue = JS4D::VDurationToValue( fContext, inDuration);
}


#endif

void VJSValue::SetBool(bool inValue, VJSException *outException)
{
	JS4D::BoolToValue(*this, inValue);
}

void VJSValue::SetVValue(const XBOX::VValueSingle& inValue, VJSException *outException, bool simpleDate)
{

	JS4D::VValueToValue(*this, inValue, VJSException::GetExceptionRefIfNotNull(outException), simpleDate);
}

void VJSValue::SetJSONValue( const VJSONValue& inValue, VJSException *outException)
{

	JS4D::VJSONValueToValue(*this, inValue, VJSException::GetExceptionRefIfNotNull(outException));
}

void VJSValue::SetFolder( XBOX::VFolder *inFolder, VJSException *outException)
{
	VJSObject tmpObj(fContext);
	JS4D::VFolderToObject(tmpObj, inFolder, VJSException::GetExceptionRefIfNotNull(outException));
#if USE_V8_ENGINE
	JS4D::ObjectToValue( *this, tmpObj );
#else
	fValue = JS4D::ObjectToValue( fContext, tmpObj.fObject );
#endif
}

bool VJSValue::IsInstanceOf( const XBOX::VString& inConstructorName, JS4D::ExceptionRef *outException) const
{
	return JS4D::ValueIsInstanceOf( fContext, fValue, inConstructorName, outException);
}

bool VJSValue::IsFunction() const
{
#if USE_V8_ENGINE
    return ( fValue ? fValue->IsFunction() : false );
#else
	JSObjectRef obj = ( (fValue != NULL) && JSValueIsObject( fContext, fValue) ) ? JSValueToObject( fContext, fValue, NULL) : NULL;
	return (obj != NULL) ? JSObjectIsFunction( fContext, obj) : false;
#endif
}

#if USE_V8_ENGINE
VJSObject::~VJSObject()
{
	//DebugMsg("VJSObject::~VJSObject called\n");
	if (fObject)
	{
	 	V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
  		fObject = NULL;
	}
}



void VJSObject::Dispose()
{
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = NULL;
	}
}


void VJSObject::CopyFrom(JS4D::ValueRef inOther)
{
	Dispose();
	if (inOther)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther);
		fObject = reinterpret_cast<Value*>(V8::CopyPersistent(p));
	}
}

void VJSObject::CopyFrom(const VJSObject& inOther)
{
	xbox_assert(fContext == inOther.fContext);
	Dispose();
	if (inOther.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
		fObject = reinterpret_cast<Value*>(V8::CopyPersistent(p));
	}
}

void VJSObject::Set(JS4D::ObjectRef inValue)
{
	xbox_assert(fObject == NULL);
	if (inValue)
	{
		intptr_t memAmount = fContext.fContext->AdjustAmountOfExternalAllocatedMemory(1*2048);
		//DebugMsg("VJSObject::Set memAmount=%u\n", (uintptr_t)memAmount);
		internal::Object** p = reinterpret_cast<internal::Object**>(inValue);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext.fContext),
			p));
		V8::MakeWeak(reinterpret_cast<internal::Object**>(fObject),
			NULL,
			NULL,
			reinterpret_cast<V8::RevivableCallback>(ghRevivCbk));
	}
}
#endif

VJSObject::VJSObject( JS4D::ContextRef inContext) 
:fContext(inContext)
,fObject( NULL)	
{
}

#if USE_V8_ENGINE
VJSObject::VJSObject(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, bool inPersistent)
:fContext(inContext)
,fObject(NULL)	
{
	HandleScope scope(fContext);
	if (inPersistent)
	{
		Set(inObject);
	}
	else
	{
		CopyFrom(inObject);
	}
}
#endif

VJSObject::VJSObject( JS4D::ContextRef inContext, JS4D::ObjectRef inObject) 
:fContext(inContext)
#if USE_V8_ENGINE
,fObject( NULL)	
#else
,fObject(inObject)	
#endif
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject2( CALLED!!!!\n");
	
	HandleScope scope(fContext);
	Set(inObject);
	/*if ( inObject == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inObject);
  		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
							reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
                             p));
	}*/
#endif
}

VJSObject::VJSObject( const VJSContext& inContext, JS4D::ObjectRef inObject) 
:fContext( inContext)
#if USE_V8_ENGINE
,fObject(NULL)
#endif
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject3( CALLED!!!!\n");
	HandleScope scope(fContext);
	Set(inObject);
	/*if ( inObject == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inObject);
  		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
							reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
                             p));
	}*/
#else
	fObject = inObject;
#endif
}

//======================================================
VJSObject::operator VJSValue() const				
{
	return VJSValue(fContext,fObject);
}

VJSObject& VJSObject::operator=( const VJSObject& inOther)
{ 
	/*xbox_assert( fContext == inOther.fContext);*/ 
	fContext = inOther.fContext; 
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::operator=( CALLED!!!!\n");
	xbox_assert(fContext == inOther.fContext);
	CopyFrom(inOther);
	/*if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = NULL;
	}
	if (inOther.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}*/
#else
	fObject = inOther.fObject; 
#endif
	
	return *this; 
}

VJSObject::VJSObject( const VJSContext* inContext)
:fContext((JS4D::ContextRef)NULL)
,fObject(NULL)
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject4( CALLED!!!!\n");
#endif
	if (inContext)
	{
		fContext = VJSContext(*inContext);
	}
}

VJSObject::VJSObject( const VJSContext& inContext)
:fContext(inContext)
,fObject( NULL)
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject4bis( CALLED!!!!\n");
#endif
}

VJSObject::VJSObject( const VJSObject& inOther)
:fContext(inOther.fContext)
#if USE_V8_ENGINE
,fObject(NULL)
#else
,fObject(inOther.fObject)
#endif
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject5( CALLED!!!!\n");
	CopyFrom(inOther);
	/*if ( inOther.fObject == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
  		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
							reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
                             p));
	}*/
#endif
}

VJSObject::VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes) // build an empty object as a property of a parent object
:fContext(inParent.fContext)
#if USE_V8_ENGINE
,fObject( NULL)
#endif
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject6( CALLED!!!!\n");
#endif
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}


VJSObject::VJSObject( const VJSArray& inParent, sLONG inPos) // build an empty object as an element of an array (-1 means push at the end)
:fContext(inParent.GetContext())
#if USE_V8_ENGINE
,fObject( NULL)
#endif
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::VJSObject7( CALLED!!!!\n");
#endif
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}


bool VJSObject::HasRef() const 
{ 
#if USE_V8_ENGINE
	return fObject != NULL; 
#else
	return fObject != NULL; 
#endif
}

void VJSObject::ClearRef()				
{
#if USE_V8_ENGINE
	Dispose();
	/*if ( fObject != NULL)
	{
	 	V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = NULL;
	}*/
#else
	fObject = NULL;
#endif
}


bool VJSObject::IsInstanceOf( const XBOX::VString& inConstructorName, VJSException *outException) const
{
#if USE_V8_ENGINE
	VStringConvertBuffer	bufferTmp(inConstructorName, VTC_UTF_8);
	if (fObject->IsObject())
	{
		if (inConstructorName == "Array")
		{
			return fObject->IsArray();
		}
		Handle<Object> obj = fObject->ToObject();
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Utf8Value  valueStr( constrName );
		if (!strcmp(bufferTmp.GetCPointer(),*valueStr))
		{
			return true;
		}
		DebugMsg("VJSObject::IsInstanceOf strcmp NOK for '%S'\n", &bufferTmp);
	}
	for (;;)
		DebugMsg("VJSObject::IsInstanceOf NOK for '%S'\n", &bufferTmp);
	return false;//IsInstanceOf( inConstructorName, VJSException::GetExceptionRefIfNotNull(outException));

#else
	return JS4D::ValueIsInstanceOf(
			fContext,
			fObject,
			inConstructorName,
			VJSException::GetExceptionRefIfNotNull(outException) );
#endif
}

bool VJSObject::CallFunction(	// WARNING: you need to protect the VJSValue arguments 
						const VJSObject& inFunctionObject, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult)
{
	return CallFunction(inFunctionObject,inValues,outResult,NULL);
}

bool VJSObject::CallFunction(	// WARNING: you need to protect the VJSValue arguments 
						const VJSObject& inFunctionObject, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult, 
						VJSException& outException)
{
	return CallFunction(inFunctionObject,inValues,outResult,outException.GetPtr());
}

bool VJSObject::CallMemberFunction(	
						const XBOX::VString& inFunctionName, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult, 
						VJSException& outException)
{
	return CallMemberFunction(inFunctionName,inValues,outResult,outException.GetPtr());
}

bool VJSObject::CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
						const XBOX::VString& inFunctionName, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult)
{
	return CallMemberFunction(inFunctionName, inValues, outResult, NULL);
}
bool VJSObject::CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
									const XBOX::VString& inFunctionName, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									JS4D::ExceptionRef *outException)
{
	return CallFunction(GetPropertyAsObject(inFunctionName, outException), inValues, outResult, outException);
}


void VJSObject::MakeFile( VFile *inFile, JS4D::ExceptionRef *outException)
{
	if (testAssert( inFile != NULL))
	{
		JS4D::VFileToObject( *this, inFile, outException);
	}
}

void VJSObject::MakeFolder( VFolder *inFolder, JS4D::ExceptionRef *outException)
{ 
	if (testAssert( inFolder != NULL))
	{
		JS4D::VFolderToObject( *this, inFolder, outException);
	}
}

void VJSObject::MakeFilePathAsFileOrFolder( const VFilePath& inPath, JS4D::ExceptionRef *outException)
{ 
	if (testAssert( inPath.IsValid()))
	{
		JS4D::VFilePathToObjectAsFileOrFolder( *this, inPath, outException);		
	}
}

void VJSObject::SetNullProperty( const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNull();
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VValueSingle* inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	if (inValue == NULL || inValue->IsNull())
		jsval.SetNull();
	else
		jsval.SetVValue(*inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG8 inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, Real inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
    Handle<Context>			context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope			context_scope(context);
    Local<Object>			obj = fObject->ToObject();
    VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
    Handle<Number>			propVal = Number::New( static_cast<double>(inValue));

    obj->Set(String::NewSymbol(bufferTmp.GetCPointer()),propVal);
#else
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VString& inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
    Handle<Context>			context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope			context_scope(context);
    Local<Object>			obj = fObject->ToObject();
    VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
    VStringConvertBuffer	valStrTmp(inValue,VTC_UTF_8);
    Handle<String>			propVal = String::NewSymbol(valStrTmp.GetCPointer());

    obj->Set(String::NewSymbol(bufferTmp.GetCPointer()),propVal);
#else

	VJSValue jsval(fContext);
	jsval.SetString(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
#endif
}



bool VJSObject::HasProperty( const VString& inPropertyName) const
{
	bool ok = false;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext,*v8PersContext);
	Context::Scope			context_scope(context);
    Local<Object>			obj = fObject->ToObject();
	ok = obj->Has(v8::String::NewFromTwoByte(fContext,inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()));
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	ok = JSObjectHasProperty( fContext, fObject, jsName);
	JSStringRelease( jsName);
#endif
	return ok;
}


void VJSObject::SetProperty( const VString& inPropertyName, const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::SetProperty1 called for  prop=%S\n",&inPropertyName);
	if (testAssert(inValue.fValue && fObject->IsObject()))
    {
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
		Local<Object>			obj = fObject->ToObject();
        Handle<Value>			propVal = Handle<Value>::New(GetContextRef(),inValue.fValue);
		Local<String>			propStr = v8::String::NewFromTwoByte(fContext, inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength());
		obj->Set(propStr, propVal);
    }
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inValue, inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::SetProperty2 called for  prop=%S\n",&inPropertyName);
	if (testAssert(inObject.fObject && fObject->IsObject()))
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
		if (fObject->IsFunction())
		{
			int a=1;
		}
		Local<Object>			obj = fObject->ToObject();
		VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
		Handle<Value>			propVal = Handle<Value>::New(GetContextRef(),inObject.fObject);

		obj->Set(String::NewSymbol(bufferTmp.GetCPointer()),propVal);
		v8PersContext->Reset(fContext, context);
	}
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inObject.GetObjectRef(), inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSArray& inArray, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
    //DebugMsg("VJSObject::SetProperty3 called for  prop=%S\n",&inPropertyName);
	JS4D::ObjectRef		arrayRef = inArray.fObject;
	if (testAssert(arrayRef->IsArray()))
	{
		VJSObject	tmpObj(fContext, arrayRef,true);
		SetProperty(inPropertyName, tmpObj, inAttributes, outException);
	}
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty(fContext, fObject, jsName, inArray.fObject, inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const VString& inPropertyName, bool inBoolean, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
    Context::Scope			context_scope(context);
    Local<Object>			obj = fObject->ToObject();
    VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
    Handle<v8::Boolean>		propVal = v8::Boolean::New(inBoolean);

    obj->Set(String::NewSymbol(bufferTmp.GetCPointer()),propVal);
#else
	JSStringRef jsName = JS4D::VStringToString(inPropertyName);
	JSObjectSetProperty(
		fContext, 
		fObject, 
		jsName, 
		JSValueMakeBoolean(fContext, inBoolean), inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease(jsName);
#endif
}

VJSValue VJSObject::GetProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (fObject && fObject->IsObject())
	{
		HandleScope				handle_scope(fContext);		
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext,*v8PersContext);
		Context::Scope			context_scope(context);
		VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
		Local<Object>			obj = fObject->ToObject();
		Local<Value>			propVal = obj->Get(v8::String::NewSymbol(bufferTmp.GetCPointer()));
		VJSValue				result(fContext, propVal.val_);
		return result;
	}
	return VJSValue(fContext);

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef valueRef = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
	return VJSValue( fContext, valueRef);
#endif
}


VJSObject VJSObject::GetPropertyAsObject( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	if (fObject && fObject->IsObject())
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
		VStringConvertBuffer	bufferTmp(inPropertyName,VTC_UTF_8);
		Local<Object>			obj = fObject->ToObject();
		Local<Value>			propVal = obj->Get(v8::String::NewSymbol(bufferTmp.GetCPointer()));
		if (propVal->IsObject())
		{
			VJSObject				result(fContext, propVal.val_,true);
			if (inPropertyName == "model")//"require")
			{
				int a = 1;
			}
			return result;
		}
		else
		{
			DebugMsg("VJSObject::GetPropertyAsObject(%S) isNull=%d isUndef=%d\n", &inPropertyName, propVal->IsNull(), propVal->IsUndefined());
		}
	}
	return VJSObject(fContext);

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef value = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSObjectRef objectRef = ((value != NULL) && JSValueIsObject( fContext, value)) ? JSValueToObject( fContext, value, outException) : NULL;
	JSStringRelease( jsName);
	return VJSObject( fContext, objectRef);
#endif
}


bool VJSObject::GetPropertyAsBool(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	bool res = false;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetBool(&res, VJSException::GetExceptionRefIfNotNull((outException)));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


sLONG VJSObject::GetPropertyAsLong(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	sLONG res = 0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetLong(&res, VJSException::GetExceptionRefIfNotNull(outException));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


bool VJSObject::GetPropertyAsString(const VString& inPropertyName, VJSException* outException, VString& outResult) const
{
	bool ok = false;
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	outResult.Clear();
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetString(outResult, VJSException::GetExceptionRefIfNotNull(outException));
	}
	return ok;
}



Real VJSObject::GetPropertyAsReal(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	Real res = 0.0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetReal(&res, VJSException::GetExceptionRefIfNotNull(outException));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


void VJSObject::GetPropertyNames( std::vector<VString>& outNames) const
{
#if USE_V8_ENGINE
	DebugMsg("VJSObject::GetPropertyNames called \n");
	outNames.clear();
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope			context_scope(context);
	if (fObject->IsObject())
	{
		v8::Local<v8::Object>	locObj = fObject->ToObject();
		v8::Local<v8::Array>	propsArray = locObj->GetPropertyNames();
		uint32_t	arrLength = propsArray->Length();
		for (uint32_t idx = 0; idx < arrLength; idx++)
		{
			Local<Value>	tmpVal = propsArray.val_->Get(idx);
			if (tmpVal.val_->IsString())
			{
				Local<String> propName = tmpVal.val_->ToString();
				v8::String::Utf8Value  valueStr(propName);
				outNames.push_back(VString(*valueStr));
			}
		}
	}
#else
	JSPropertyNameArrayRef array = JSObjectCopyPropertyNames( fContext, fObject);
	if (array != NULL)
	{
		size_t size = JSPropertyNameArrayGetCount( array);
		try
		{
			outNames.resize( size);
			size = 0;
			for( std::vector<VString>::iterator i = outNames.begin() ; i != outNames.end() ; ++i)
			{
				JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( array, size++), *i);
			}
		}
		catch(...)
		{
			outNames.clear();
		}
		JSPropertyNameArrayRelease( array);
	}
	else
	{
		outNames.clear();
	}
#endif
}


bool VJSObject::DeleteProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	DebugMsg("VJSObject::DeleteProperty called\n");
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	ok = JSObjectDeleteProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
#endif
	return ok;
}

#if 0 //USE_V8_ENGINE
bool VJSObject::CallFunction(
	const VJSObject &inFunctionObject,
	const std::vector<VJSValue> *inValues,
	VJSPersistentValue& outResult,
	VJSException& outException)
{
	bool ok = true;
	JS4D::ExceptionRef exception = NULL;

	JS4D::ValueRef result = NULL;
	HandleScope			scope(fContext);
	Handle<Context>		context = Handle<Context>::New(fContext, V4DContext::GetPersistentContext(fContext));
	Context::Scope		context_scope(context);

	if (inFunctionObject.fObject->ToObject()->IsCallable())
	{
		int count = ((inValues == NULL) || inValues->empty()) ? 0 : inValues->size();
		Handle<Value>* values = (count == 0) ? NULL : new Handle<Value>[count];
		if (values != NULL)
		{
			int j = 0;
			for (std::vector<VJSValue>::const_iterator i = inValues->begin(); i != inValues->end(); ++i)
			{
				values[j++] = Handle<Value>::New(fContext, i->GetValueRef());
			}
		}
		else
		{
			ok = (count == 0);
		}
		Handle<Value>	objVal = Handle<Value>::New(fContext, fObject);
		Local<Object>	obj = objVal->ToObject();
		Local<Object>	locFunc = inFunctionObject.fObject->ToObject();
		Handle<Value>	result = locFunc->CallAsFunction(locFunc, count, values);
		v8::String::Utf8Value	resStr(result);
		DebugMsg("VJSObject::CallFunction(isFunc=%d) result='%s' (isObj=%d)\n", inFunctionObject.fObject->IsFunction(), *resStr, result->IsObject());
		if (values)
		{
			delete[] values;
		}
		//if (outResult != NULL)
		{
			//*outResult = VJSValue(fContext, result.val_);
			VJSValue	tmpVal(fContext, result.val_);
			outResult.SetValue(tmpVal);
			/*if (outResult->fValue)
			{
				V8::DisposeGlobal(reinterpret_cast<internal::Object**>(outResult->fValue));
				outResult->fValue = NULL;
			}
			if (result.val_)
			{
				internal::Object** p = reinterpret_cast<internal::Object**>(result.val_);
				outResult->fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
					reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
					p));
			}*/
		}

	}
	return ok;
}
#endif

bool VJSObject::CallFunction (
	const VJSObject &inFunctionObject, 
	const std::vector<VJSValue> *inValues, 
	VJSValue *outResult, 
	JS4D::ExceptionRef *outException)
{
	bool ok = true;
    JS4D::ExceptionRef exception = NULL;
#if USE_V8_ENGINE
    JS4D::ValueRef result = NULL;
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope		context_scope(context);

	if (inFunctionObject.fObject->ToObject()->IsCallable())
	{
		int count = ((inValues == NULL) || inValues->empty()) ? 0 : inValues->size();
		Handle<Value>* values = (count == 0) ? NULL : new Handle<Value>[count];
		if (values != NULL)
		{
			int j = 0;
			for (std::vector<VJSValue>::const_iterator i = inValues->begin(); i != inValues->end(); ++i)
			{
				values[j++] = Handle<Value>::New(fContext, i->GetValueRef());
			}
		}
		else
		{
			ok = (count == 0);
		}
		Handle<Value>	objVal = Handle<Value>::New(fContext, fObject);
		Local<Object>	obj = objVal->ToObject();
		Local<Object>	locFunc = inFunctionObject.fObject->ToObject();
		Handle<Value>	result = locFunc->CallAsFunction(locFunc, count, values);
		//v8::String::Utf8Value	resStr(result);
		//DebugMsg("VJSObject::CallFunction(isFunc=%d) result='%s' (isObj=%d)\n", inFunctionObject.fObject->IsFunction(), *resStr, result->IsObject());
		if (values)
		{
			delete[] values;
		}
		if (outResult != NULL)
		{
			outResult->Dispose();
			outResult->Set(result.val_);
		}

	}
	else
	{
		if (testAssert(inFunctionObject.IsFunction()))
		{
			int count = ((inValues == NULL) || inValues->empty()) ? 0 : inValues->size();
			Handle<Value>* values = (count == 0) ? NULL : new Handle<Value>[count];
			if (values != NULL)
			{
				int j = 0;
				for (std::vector<VJSValue>::const_iterator i = inValues->begin(); i != inValues->end(); ++i)
				{
					values[j++] = Handle<Value>::New(fContext, i->GetValueRef());
				}
			}
			else
			{
				ok = (count == 0);
			}
			Handle<Value>	objVal = Handle<Value>::New(fContext, fObject);
			Local<Object>	obj = objVal->ToObject();
			Local<Object>	locFunc = inFunctionObject.fObject->ToObject();
			Handle<Value>	result = locFunc->CallAsFunction(locFunc, count, values);
			if (values)
			{
				delete[] values;
			}
			if (outResult != NULL)
			{
				outResult->Dispose();
				outResult->Set(result.val_);
			}
		}
		else
		{
			ok = false;
		}
	}

    if (exception != NULL)
        ok = false;

    if (outException != NULL)
        *outException = exception;

#else
    JSValueRef result = NULL;

	if (inFunctionObject.IsFunction())
	{
		size_t count = ( (inValues == NULL) || inValues->empty() ) ? 0 : inValues->size();
		JSValueRef *values = (count == 0) ? NULL : new JSValueRef[count];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		else
		{
			ok = (count == 0);
		}

		result = JS4DObjectCallAsFunction( fContext, inFunctionObject.GetObjectRef(), fObject, count, values, &exception);

		delete[] values;
	}
	else
	{
		ok = false;
	}

	if (exception != NULL)
		ok = false;
	
	if (outException != NULL)
		*outException = exception;
	
	if (outResult != NULL)
	{
		if (result == NULL)
			outResult->SetUndefined();
		else
			outResult->fValue = result;
	}
#endif
	return ok;
}

bool VJSObject::CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject, VJSException* outException)
{
	xbox_assert(outCreatedObject != NULL);
#if USE_V8_ENGINE
	DebugMsg("VJSObject::CallAsConstructor\n");
	for (;;)
		DebugMsg("%s  NOT IMPL\n", __PRETTY_FUNCTION__);
	return false;
#else
	if (!JSObjectIsConstructor(GetContextRef(), GetObjectRef()))

		return false;

	uLONG		argumentCount	= inValues == NULL || inValues->empty() ? 0 : (uLONG) inValues->size();
	JSValueRef	*values			= argumentCount == 0 ? NULL : new JSValueRef[argumentCount];

	if (values != NULL) {

		std::vector<VJSValue>::const_iterator	i;
		uLONG									j;

		for (i = inValues->begin(), j = 0; i != inValues->end(); i++, j++)

			values[j] = i->GetValueRef();
		

	}

	JS4D::ObjectRef		createdObject;
	
	createdObject = JSObjectCallAsConstructor(	GetContextRef(),
												GetObjectRef(), 
												argumentCount, 
												values, 
												(JS4D::ExceptionRef*)VJSException::GetExceptionRefIfNotNull(outException));
		
	if (values != NULL)

		delete[] values;


	outCreatedObject->SetContext(fContext);
	if ( (outException && (outException->GetValue() != NULL) ) || createdObject == NULL) {
		
		outCreatedObject->ClearRef();
		return false;

	} else {

		outCreatedObject->SetObjectRef(createdObject);
		return true;

	}
#endif
}

bool VJSObject::IsArray() const
{
#if USE_V8_ENGINE
	return fObject->IsArray();
#else
	return IsInstanceOf("Array");
#endif
}

bool VJSObject::IsFunction() const	// sc 03/09/2009 crash if fObject is null
{
#if USE_V8_ENGINE
	return (fObject != NULL) ? JS4D::ObjectIsFunction(fContext, fObject) : false;
#else
	return (fObject != NULL) ? JS4D::ObjectIsFunction( fContext, fObject) : false;
#endif
}


bool VJSObject::IsObject() const							
{
#if USE_V8_ENGINE
	return (fObject && fObject->IsObject());
#else
	return fObject != NULL; 
#endif
}

bool VJSObject::IsOfClass (JS4D::ClassRef inClassRef) const	
{ 
    return JS4D::ValueIsObjectOfClass( fContext, fObject, inClassRef);
}
	
void VJSObject::MakeObject( JS4D::ClassRef inClassRef, void* inPrivateData)	
{
#if USE_V8_ENGINE
	xbox_assert(false);
#else
	fObject = JS4D::MakeObject( fContext, inClassRef, inPrivateData); 
#endif
}

void VJSObject::MakeEmpty()
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
    Context::Scope		context_scope(context);
	Handle<Object>		emptyObj = Object::New();
	
	Dispose();
	Set(emptyObj.val_);
#else
	fObject = JSObjectMake( fContext, NULL, NULL);
#endif
}
#if USE_V8_ENGINE
void VJSObject::SetSpecifics(JS4D::ClassDefinition& inClassDef,void* inData) const
{
	HandleScope handle_scope(fContext);

	if (inClassDef.staticFunctions)
	{
		for( const JS4D::StaticFunction *i = inClassDef.staticFunctions ; i->name != NULL ; ++i)
		{
			DebugMsg("VJSObject::SetAccessors treat static function %s for %s\n",i->name,inClassDef.className);
	//Local<FunctionTemplate> ft = FunctionTemplate::New(i->callAsFunction);
	//Local<Function> newFunc = ft->GetFunction();
	//global->Set(String::New(i->name),newFunc);
			/*JSStaticFunction function;
			function.name = i->name;
			function.callAsFunction = i->callAsFunction;
			function.attributes = i->attributes;
			staticFunctions.push_back( function);*/
		}
	}
	
	if (inClassDef.staticValues)
	{
		for( const JS4D::StaticValue *i = inClassDef.staticValues ; i->name != NULL ; ++i)
		{
			DebugMsg("VJSObject::SetAccessors treat static value %s for %s\n",i->name,inClassDef.className);
	/*Local<FunctionTemplate> ft = FunctionTemplate::New(i->callAsFunction);
	Local<Function> newFunc = ft->GetFunction();
	global->Set(String::New(i->name),newFunc);*/
		}
	}	
}
#endif
void VJSObject::Protect() const								
{
#if USE_V8_ENGINE
#else
	JS4D::ProtectValue( fContext, fObject);
#endif
}

void VJSObject::Unprotect() const							
{ 
#if USE_V8_ENGINE
#else
	JS4D::UnprotectValue( fContext, fObject);
#endif
}


JS4D::ObjectRef VJSObject::GetObjectRef() const									
{ 
#if USE_V8_ENGINE
	return fObject;
#else
	return fObject;
#endif
}

void VJSObject::SetObjectRef( JS4D::ObjectRef inObject)	
{ 
#if USE_V8_ENGINE
	fObject = inObject;
#else
	fObject = inObject;
#endif
}
		
void* VJSObject::GetPrivateRef() 
{
#if USE_V8_ENGINE
	xbox_assert(false);
	return NULL;
#else
	return (void*)fObject;
#endif
}

void VJSObject::MakeCallback( JS4D::ObjectCallAsFunctionCallback inCallbackFunction)
{
	JS4D::ContextRef	contextRef;
#if USE_V8_ENGINE
	contextRef = fContext;
    V4DContext* v8Ctx = (V4DContext*)(contextRef->GetData());

	xbox_assert(v8Ctx->GetIsolate() == v8::Isolate::GetCurrent());
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(v8Ctx->GetIsolate());
	Local<Context>			context = v8::Handle<v8::Context>::New(v8Ctx->GetIsolate(),*v8PersContext);
	Context::Scope context_scope(context);
	ClearRef();
	Handle<Function>	callbackFn = v8Ctx->AddCallback(inCallbackFunction);
	Handle<Function>	fn = Handle<Function>::New(v8::Isolate::GetCurrent(), *callbackFn);

	Set(fn.val_);
	/*
	if (//testAssert(fn->IsFunction()) && fn.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(fn.val_);
  		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
							reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
                             p));

	}*/
#else
	
	if ((contextRef = fContext) != NULL && inCallbackFunction != NULL) 

		// Use NULL for function name (anonymous).

		fObject = JSObjectMakeFunctionWithCallback( contextRef, NULL, inCallbackFunction);

	else

		ClearRef();
#endif
}


XBOX::VJSObject VJSObject::GetPrototype (const XBOX::VJSContext &inContext) 
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::GetPrototype called\n");
	Local<Object>	locObj = fObject->ToObject();
	Local<Value>	protoVal = locObj.val_->GetPrototype();
	if (testAssert(protoVal.val_->IsObject()))
	{
		Local<Object>	protoObj = protoVal.val_->ToObject();
		VJSObject	returnObj(inContext,protoObj.val_);
		return returnObj;
	}
	else
#else
	if (IsObject())

		return XBOX::VJSObject( inContext, JS4D::ValueToObject( inContext, JSObjectGetPrototype(inContext, fObject), NULL));

	else
#endif
		return XBOX::VJSObject( inContext, NULL);
}

//======================================================

VJSPropertyIterator::VJSPropertyIterator(const VJSObject& inObject)
: fObject(inObject)
, fIndex(0)
, fCount(0)
{
#if USE_V8_ENGINE
	if (testAssert(fObject.fObject->IsObject()))
	{
		HandleScope				handle_scope(fObject.fContext);
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fObject.fContext);
		Handle<Context>			context = Handle<Context>::New(fObject.fContext,*v8PersContext);
		Context::Scope			context_scope(context);
		Local<Object>	tmpObj = fObject.fObject->ToObject();
		//V4DContext* v8Ctx = (V4DContext*)(fObject.fContext.GetContextRef()->GetData());
		Local<Array>	propsArray = tmpObj.val_->GetPropertyNames();
		fCount = propsArray.val_->Length();
		for (uint32_t idx = 0; idx < fCount; idx++)
		{
			Local<Value>	tmpVal = propsArray.val_->Get(idx);
			if (tmpVal.val_->IsString())
			{
				Local<String> propName = tmpVal.val_->ToString();
				v8::String::Utf8Value  valueStr(propName);
				fNameArray.push_back(VString(*valueStr));
			}
			else
			{
				v8::String::Utf8Value  valueStr(tmpVal);
				fNameArray.push_back(VString(*valueStr));
			}
		}
	}
#else
	fNameArray = (fObject.GetObjectRef() != NULL) ? JSObjectCopyPropertyNames( inObject.GetContextRef(), fObject.GetObjectRef()) : NULL;
	fCount = (fNameArray != NULL) ? JSPropertyNameArrayGetCount( fNameArray) : 0;
#endif
}


VJSPropertyIterator::~VJSPropertyIterator()
{
#if USE_V8_ENGINE
	fNameArray.clear();
#else
	if (fNameArray != NULL)
		JSPropertyNameArrayRelease( fNameArray);
#endif
}

VJSValue VJSPropertyIterator::GetProperty(VJSException *outException) const
{
	return _GetProperty(VJSException::GetExceptionRefIfNotNull(outException));
}

void VJSPropertyIterator::SetProperty( const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, VJSException *outException) const 
{
	_SetProperty(
		inValue,
		inAttributes,
		VJSException::GetExceptionRefIfNotNull(outException));
}

bool VJSPropertyIterator::GetPropertyNameAsLong(sLONG *outValue) const
{
	//DebugMsg("VJSPropertyIterator::GetPropertyNameAsLong called\n"); 
	if (testAssert(fIndex < fCount))
	{
#if USE_V8_ENGINE
		return JS4D::StringToLong( fNameArray[fIndex], outValue);
#else

		return JS4D::StringToLong( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outValue);
#endif
	}
	else
	{
		return false;
	}
}


void VJSPropertyIterator::GetPropertyName( VString& outName) const
{
#if USE_V8_ENGINE
	//DebugMsg("VJSPropertyIterator::GetPropertyName called\n");
	if (testAssert(fIndex < fCount))
		outName = fNameArray[fIndex];
	else
#else
	if (testAssert( fIndex < fCount))
		JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outName);
	else
#endif

		outName.Clear();
}


VJSObject VJSPropertyIterator::GetPropertyAsObject( VJSException *outException) const
{
	VJSValue	tmpVal = _GetProperty(VJSException::GetExceptionRefIfNotNull(outException));
	if (tmpVal.IsObject())
	{
		return tmpVal.GetObject(outException);
	}
	return VJSObject(fObject.GetContextRef());
}


VJSValue VJSPropertyIterator::_GetProperty(JS4D::ExceptionRef *outException) const
{
	VJSValue	propertyVal(fObject.fContext);
#if USE_V8_ENGINE
	//DebugMsg("VJSPropertyIterator::_GetProperty called for idx=%d\n", fIndex);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fObject.fContext);
	HandleScope scope(fObject.fContext);
	Context::Scope context_scope(fObject.fContext, *v8PersContext);
	VStringConvertBuffer	bufferTmp(fNameArray[fIndex], VTC_UTF_8);
	Local<String>	propName = String::New(bufferTmp.GetCPointer());
	Local<Object>	locObj = fObject.fObject->ToObject();
	Local<Value>	propVal = locObj->Get(propName);

	propertyVal.Set(propVal.val_);
	/*{
		internal::Object** p = reinterpret_cast<internal::Object**>(propVal.val_);
		propertyVal.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fObject.fContext.fContext),
			p));
	}*/
#else
	if (testAssert(fIndex < fCount))
		propertyVal.fValue = JSObjectGetProperty(fObject.GetContextRef(), fObject.GetObjectRef(), JSPropertyNameArrayGetNameAtIndex(fNameArray, fIndex), outException);
	else
		propertyVal.fValue = NULL;
#endif
	return propertyVal;
}


void VJSPropertyIterator::_SetProperty( JS4D::ValueRef inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	DebugMsg("VJSPropertyIterator::_SetProperty called\n");
	for (;;)
		DebugMsg("%s  NOT IMPL\n", __PRETTY_FUNCTION__);
#else
	if (testAssert( fIndex < fCount))
		JSObjectSetProperty( fObject.GetContextRef(), fObject.GetObjectRef(), JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), inValue, inAttributes, outException);
#endif
}


//======================================================
void VJSArray::Protect() const
{
#if USE_V8_ENGINE
#else
	JS4D::ProtectValue(fContext, fObject);
#endif
}

void VJSArray::Unprotect() const
{
#if USE_V8_ENGINE
#else
	JS4D::UnprotectValue(fContext, fObject);
#endif
}

VJSArray& VJSArray::operator=(const VJSArray& inOther)
{
	fContext = inOther.fContext;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
	}
	Context::Scope context_scope(fContext,*v8PersContext);
	if (inOther.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext.fContext),
			p));
	}
	else
	{
		fObject = NULL;
	}
#else
	fObject = inOther.fObject;
#endif
	return *this;
}


VJSArray::~VJSArray()
{
#if USE_V8_ENGINE
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = 0;
	}
#else
	fObject = NULL;
#endif
}

VJSArray::VJSArray(const VJSArray& inOther)
: fContext(inOther.fContext)
, fObject(NULL)
{
#if USE_V8_ENGINE
	if (inOther.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext.fContext),
			p));
	}
#else
	fObject = inOther.fObject;
#endif
}

void VJSArray::SetNull()
{
#if USE_V8_ENGINE

	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = NULL;
	}
#else
	fObject = NULL;
#endif
}


VJSArray::VJSArray( const VJSContext& inContext, VJSException* outException)
: fContext( inContext)
{
#if USE_V8_ENGINE
	xbox_assert(v8::Isolate::GetCurrent() == inContext.fContext);
	V4DContext*				v8Ctx = (V4DContext*)inContext.fContext->GetData();
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
	HandleScope				handleScope(inContext);

	Local<v8::Array>	tmpArray = v8::Array::New(0);
	if (tmpArray.val_ == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(tmpArray.val_);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
#else
	// build an array
	fObject = JSObjectMakeArray( fContext, 0, NULL, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}

VJSArray::VJSArray( const VJSContext& inContext, const std::vector<VJSValue>& inValues, VJSException* outException)
: fContext( inContext)
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	Handle<v8::Array>	tmpArray = v8::Array::New(inValues.size());

	if (tmpArray.val_ == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(tmpArray.val_);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
		uint32_t	idx = 0;
		for (std::vector<VJSValue>::const_iterator it = inValues.begin(); it != inValues.end(); ++it)
		{
			Handle<Value> tmpVal = Handle<Value>::New(fContext, it->fValue);
			tmpArray.val_->Set(idx, tmpVal);
			idx++;
		}
	}
#else	// build an array
	JS4D::ExceptionRef*  except = VJSException::GetExceptionRefIfNotNull(outException);
	if (inValues.empty())
	{
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, 0, NULL, except);
		#else
		fObject = NULL;
		#endif
	}
	else
	{
		JSValueRef *values = new JSValueRef[inValues.size()];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues.begin() ; i != inValues.end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, inValues.size(), values, except);
		#else
		fObject = NULL;
		#endif
		delete[] values;
	}
#endif
}




VJSArray::VJSArray(const VJSObject& inArrayObject, bool inCheckInstanceOfArray)
: fContext(inArrayObject.GetContext())
{
	if (inCheckInstanceOfArray && !inArrayObject.IsInstanceOf(CVSTR("Array")))
		fObject = NULL;
	else
	{
#if USE_V8_ENGINE
		internal::Object** p = reinterpret_cast<internal::Object**>(inArrayObject.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
#else
		fObject = inArrayObject.GetObjectRef();
#endif
	}
}


VJSArray::VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray)
: fContext( inArrayValue.GetContext())
{
	if (inCheckInstanceOfArray && !inArrayValue.IsInstanceOf( CVSTR( "Array")) )
		fObject = NULL;
	else
	{
#if USE_V8_ENGINE
		internal::Object** p = reinterpret_cast<internal::Object**>(inArrayValue.fValue);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
#else
		VJSObject jobj(inArrayValue.GetObject());
		inArrayValue.GetObject(jobj);
		fObject = jobj.GetObjectRef();
#endif
	}
}

VJSArray::operator VJSObject() const
{ 
	return VJSObject(fContext, fObject);
}

VJSArray::operator VJSValue() const	
{ 
	return VJSValue(fContext, fObject); 
}

size_t VJSArray::GetLength() const
{
	size_t length = 0;
#if USE_V8_ENGINE
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);
		length = tmpArray->Length();
	}
#else
	JSStringRef jsString = JSStringCreateWithUTF8CString( "length");
	JSValueRef result = JSObjectGetProperty( fContext, fObject, jsString, NULL);
	JSStringRelease( jsString);
	if (testAssert( result != NULL))
	{
		double r = JSValueToNumber( fContext, result, NULL);
		length = (size_t) r;
	}
	else
	{
		length = 0;
	}
#endif
	return length;
}


VJSValue VJSArray::GetValueAt( size_t inIndex, VJSException *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	VJSValue	result(fContext);
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);
		if (tmpArray && testAssert(inIndex < tmpArray->Length()) )
		{
			Local<Value>	locVal = tmpArray->Get(inIndex);
			internal::Object** p = reinterpret_cast<internal::Object**>(locVal.val_);
			result.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
				p));
		}
	}
	return result;
#else
	return VJSValue(
		fContext,
		JSObjectGetPropertyAtIndex( fContext, fObject, inIndex, VJSException::GetExceptionRefIfNotNull(outException) ));
#endif
}


void VJSArray::SetValueAt( size_t inIndex, const VJSValue& inValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);
		if (inIndex >= tmpArray->Length())
		{
		}
		Handle<Value>	tmpVal = Handle<Value>::New(fContext.fContext, inValue.fValue);
		tmpArray->Set(inIndex, tmpVal);

	}
#else
	JSObjectSetPropertyAtIndex( fContext, fObject, inIndex, inValue, VJSException::GetExceptionRefIfNotNull(outException) );
#endif
}


void VJSArray::PushValue( const VJSValue& inValue, VJSException *outException) const
{
	SetValueAt( GetLength(), inValue, outException);
}


void VJSArray::PushValue( const VValueSingle& inValue, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetVValue(inValue, outException);
	PushValue(val, outException);
}

template<class Type>
void VJSArray::PushNumber( Type inValue , VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetNumber(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushString( const VString& inValue , VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetString(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushFile( XBOX::VFile *inFile, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFile( inFile, outException);
	PushValue(val, outException);
}


void VJSArray::PushFolder( XBOX::VFolder *inFolder, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFolder( inFolder, outException);
	PushValue(val, outException);
}


void VJSArray::PushFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFilePathAsFileOrFolder( inPath, outException);
	PushValue(val, outException);
}


void VJSArray::PushValues( const std::vector<const XBOX::VValueSingle*> inValues, VJSException *outException) const
{
	for (std::vector<const XBOX::VValueSingle*>::const_iterator cur = inValues.begin(), end = inValues.end(); cur != end; cur++)
	{
		const VValueSingle* val = *cur;
		if (val == NULL)
		{
			VJSValue xval(fContext);
			xval.SetNull();
			PushValue(xval, outException);
		}
		else
			PushValue(*val, outException);
	}
}


void VJSArray::_Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue> *inValues, VJSException *outException) const
{
#if USE_V8_ENGINE
	DebugMsg("VJSArray::_Splice called\n");
#else
	JS4D::ExceptionRef*		jsException = VJSException::GetExceptionRefIfNotNull(outException);
	JSStringRef jsString = JSStringCreateWithUTF8CString( "splice");
	JSValueRef splice = JSObjectGetProperty( fContext, fObject, jsString, jsException);
	JSObjectRef spliceFunction = JSValueToObject( fContext, splice, jsException);
    JSStringRelease( jsString);

	JSValueRef *values = new JSValueRef[2 + ((inValues != NULL) ? inValues->size() : 0)];
	if (values != NULL)
	{
		size_t j = 0;
		values[j++] = JSValueMakeNumber( fContext, inWhere);
		values[j++] = JSValueMakeNumber( fContext, inCount);
		if (inValues != NULL)
		{
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
	
		JS4DObjectCallAsFunction( fContext, spliceFunction, fObject, j, values, jsException);
	}
	delete[] values;
#endif
}


// -------------------------------------------------------------------


VJSPictureContainer::VJSPictureContainer(XBOX::VValueSingle* inPict,/* bool ownsPict,*/ const VJSContext& inContext) :
fContext(inContext)
{
	assert(inPict == NULL || inPict->GetValueKind() == VK_IMAGE);
	fPict = (VPicture*)inPict;
	fMetaInfoIsValid = false;
	//fOwnsPict = ownsPict;
	fMetaBag = NULL;
}


VJSPictureContainer::~VJSPictureContainer()
{
	if (/*fOwnsPict &&*/ fPict != NULL)
		delete fPict;
	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	QuickReleaseRefCountable(fMetaBag);
}


void VJSPictureContainer::SetMetaInfo(JS4D::ValueRef inMetaInfo, const VJSContext& inContext) 
{
#if !VERSION_LINUX

	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	JS4D::ProtectValue(inContext, inMetaInfo);
	fMetaInfo = inMetaInfo;
	fMetaInfoIsValid = true;
	fContext = inContext;
#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

#endif
}


const XBOX::VValueBag* VJSPictureContainer::RetainMetaBag()
{
#if !VERSION_LINUX

	if (fMetaBag == NULL)
	{
		if (fPict != NULL)
		{
			const VPictureData* picdata = fPict->RetainNthPictData(1);
			if (picdata != NULL)
			{
				fMetaBag = picdata->RetainMetadatas();
				picdata->Release();
			}
		}
	}
	return RetainRefCountable(fMetaBag);

#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

	return NULL;

#endif
}


void VJSPictureContainer::SetMetaBag(const XBOX::VValueBag* metaBag)
{
	QuickReleaseRefCountable(fMetaBag);
	fMetaBag = RetainRefCountable(metaBag);
}


// -------------------------------------------------------------------

VJSFunction::~VJSFunction()
{
	ClearParamsAndResult();
}


void VJSFunction::ClearParamsAndResult()
{
	for( std::vector<VJSValue>::const_iterator i = fParams.begin() ; i != fParams.end() ; ++i)
		i->Unprotect();
	fParams.clear();

	fResult.Unprotect();
	fResult = VJSValue(fContext);
	
	JS4D::UnprotectValue( fContext, fException.GetValue());
	fException.SetValue(NULL);
}


void VJSFunction::AddParam( const VValueSingle* inVal)
{
	VJSValue val(fContext);
	if (inVal == NULL)
		val.SetNull();
	else
		val.SetVValue(*inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VJSValue& inVal)
{
	inVal.Protect();
	fParams.push_back( inVal);
}


void VJSFunction::AddParam( const VJSONValue& inVal)
{
	VJSValue val( fContext);
	val.SetJSONValue( inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VString& inVal)
{
	VJSValue val(fContext);
	val.SetString(inVal);
	AddParam( val);
}


void VJSFunction::AddParam(sLONG inVal)
{
	VJSValue val( fContext);
	val.SetNumber(inVal);
	AddParam( val);
}


void VJSFunction::AddUndefinedParam()
{
	VJSValue val( fContext);
	val.SetUndefined();
	AddParam( val);
}


void VJSFunction::AddNullParam()
{
	VJSValue val( fContext);
	val.SetNull();
	AddParam( val);
}


void VJSFunction::AddBoolParam( bool inVal)
{
	VJSValue val( fContext);
	val.SetBool( inVal);
	AddParam( val);
}


void VJSFunction::AddLong8Param( sLONG8 inVal)
{
	VJSValue val( fContext);
	val.SetNumber( inVal);
	AddParam( val);
}


bool VJSFunction::Call( bool inThrowIfUndefinedFunction)
{
	bool called = false;

	VJSObject functionObject( fFunctionObject);
	
	if (!functionObject.IsObject() && !fFunctionName.IsEmpty())
	{
		VJSValue result( fContext);
		fContext.EvaluateScript( fFunctionName, NULL, &result, NULL, NULL);
		functionObject = result.GetObject();
	}

	if (functionObject.IsFunction())
	{
		JS4D::ExceptionRef exception = NULL;

		VJSValue result( fContext);
		fContext.GetGlobalObject().CallFunction( functionObject, &fParams, &result, &exception);

		result.Protect();	// protect on stack before storing it in class field
		JS4D::ProtectValue( fContext, exception);

		fResult.Unprotect();	// unprotect previous result we may have if Call() is being called twice.
		JS4D::UnprotectValue( fContext, fException.GetValue());
		
		fResult = result;
		fException.SetValue(exception);
		called = exception == NULL;
	}
	else
	{
		if (fFunctionName.IsEmpty())
			vThrowError( VE_JVSC_UNDEFINED_FUNCTION);
		else
			vThrowError( VE_JVSC_UNDEFINED_FUNCTION_BY_NAME, fFunctionName);
	}

	return called;
}

bool VJSFunction::GetResultAsBool() const
{
	bool res = false;
	fResult.GetBool(&res);
	return res;
}

sLONG VJSFunction::GetResultAsLong() const
{
	sLONG res = 0;
	fResult.GetLong(&res);
	return res;
}


sLONG8 VJSFunction::GetResultAsLong8() const
{
	sLONG8 res = 0;
	fResult.GetLong8(&res);
	return res;
}


void VJSFunction::GetResultAsString(VString& outResult) const
{
	fResult.GetString(outResult);
}


bool VJSFunction::GetResultAsJSONValue( VJSONValue& outResult) const
{
	if (fException.GetValue() != NULL)
	{
		outResult.SetUndefined();
		return false;
	}
	return fResult.GetJSONValue( outResult, fException.GetPtr());
}



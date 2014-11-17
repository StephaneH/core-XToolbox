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
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE

//#define DEBUG_MEM_LEAKS 1
#if DEBUG_MEM_LEAKS
#include <atomic>
#include <mutex>
static std::atomic<unsigned long long> sNbTotalCalls;
static std::atomic<unsigned long long> sNbCalls;
static std::atomic<unsigned long long> sNbVals;
static std::atomic<unsigned long long> sNbArrs;

static XBOX::VCriticalSection*	sMtx = new XBOX::VCriticalSection();

typedef std::map< const XBOX::VJSObject*, XBOX::VString > ObjectMap_t;
typedef std::map< XBOX::VString, unsigned int > ObjectBackTraceMap_t;
static ObjectMap_t  sObjMap;
static ObjectBackTraceMap_t  sBTMap;

#define K_NB_CALLS_REFRESH	2000
static void displayMemStats()
{
	++sNbTotalCalls;
	if (++sNbCalls > K_NB_CALLS_REFRESH)
	{
		sMtx->Lock();
		if (sNbCalls < K_NB_CALLS_REFRESH)
		{
			goto out;
		}
		sNbCalls = 0;
#if VERSION_LINUX

		DebugMsg("displayMemStats nbVals=%lld nbObjs=%lld nbBts=%d nbArrs=%lld nbTotalCalls=%lld\n",
			sNbVals.load(), sObjMap.size(), sBTMap.size(), sNbArrs.load(), sNbTotalCalls.load());
		for (ObjectBackTraceMap_t::iterator iter = sBTMap.begin(); iter != sBTMap.end(); ++iter)
		{
			if ((*iter).second > 40)
			{
				VString tmp = ">> (nb=";
				tmp.AppendLong((*iter).second);
				tmp += ") ->";
				tmp += (*iter).first;
				DebugMsg(">>  %S\n\n", &tmp);
			}
		}
#else
		DebugMsg("displayMemStats nbVals=%lld nbObjs=%lld nbArrs=%lld nbTotalCalls=%lld\n", sNbVals.load(), sObjMap.size(), sNbArrs.load(), sNbTotalCalls.load());
#endif
out:
		sMtx->Unlock();
	}
}

static void IncVals()
{
	sNbVals++;
	displayMemStats();
}
static void DecVals()
{
	sNbVals--;
}
static void IncArrs()
{
	sNbArrs++;
	displayMemStats();
}
static void DecArrs()
{
	sNbArrs--;
}

#if VERSION_LINUX

#include <execinfo.h>
static VString buildBacktrace(char *const *buffer, int size)
{
	VString tmp("");
	for( int idx=0; idx<size; idx++ )
	{
		tmp += buffer[idx];
		tmp += '\n';
	}
	return tmp;
}


static void IncObjs(const VJSObject* inObj)
{
#define K_BUF_SIZE	6
	void* buf[K_BUF_SIZE];
	int nbTraces = backtrace(buf,K_BUF_SIZE);
	char ** btSymbs = backtrace_symbols(buf,nbTraces);
	VString str = buildBacktrace(btSymbs,nbTraces);
	free(btSymbs);

	sMtx->Lock();

	sObjMap[inObj] = str;
	unsigned int& count = sBTMap[str];
	count++;
	sMtx->Unlock();
	displayMemStats();
}

static void DecObjs(const VJSObject* inObj)
{
	sMtx->Lock();
	ObjectMap_t::iterator itObj = sObjMap.find( inObj );
	if (testAssert(itObj != sObjMap.end()))
	{
		ObjectBackTraceMap_t::iterator itBT = sBTMap.find((*itObj).second);
		if (itBT != sBTMap.end())
		{
			(*itBT).second--;
		}
		sObjMap.erase(itObj);
	}
	sMtx->Unlock();
}

#else

static void IncObjs(const VJSObject* inObj)
{
	sMtx->Lock();
	sObjMap[inObj] = "";
	sMtx->Unlock();
	displayMemStats();
}

static void DecObjs(const VJSObject* inObj)
{
	sMtx->Lock();
	ObjectMap_t::iterator itObj = sObjMap.find( inObj );
	if (testAssert(itObj != sObjMap.end()))
	{
		sObjMap.erase(itObj);
	}
	sMtx->Unlock();
}
#endif


#else

#define IncVals()

#define IncObjs(objRef)

#define IncArrs()

#define DecVals()

#define DecArrs()

#define DecObjs(objRef)

#endif


#if USE_V8_ENGINE

static bool InternalValueIsObjectOfClass(JS4D::ContextRef inContext, JS4D::ValueRef inValue, const char* inClassName)
{
	bool isSame = false;
	xbox_assert(v8::Isolate::GetCurrent() == inContext);

	V4DContext*				v8Ctx = (V4DContext*)inContext->GetData(0);
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
	HandleScope				handleScope(inContext);
#if	V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(inValue).IsEmpty() && (*ToV8Persistent(inValue))->IsObject())
	{
		Local<Object>		locObj = (*ToV8Persistent(inValue))->ToObject();
#else
	if (inValue && inValue->IsObject())
	{
		Local<Object>		locObj = inValue->ToObject();
#endif
		Local<String>		constName = locObj->GetConstructorName();
		String::Utf8Value	utf8Name(constName);
		isSame = !::strcmp(*utf8Name, inClassName);
		if (!isSame)
		{
			DebugMsg("InternalValueIsObjectOfClass '%s' != '%s'\n", inClassName, *utf8Name);
		}
	}

	return isSame;
}

static bool InternalObjectIsObjectOfClass(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, const char* inClassName)
{
	bool isSame = false;
	xbox_assert(v8::Isolate::GetCurrent() == inContext);

	V4DContext*				v8Ctx = (V4DContext*)inContext->GetData(0);
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
	HandleScope				handleScope(inContext);
#if	V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(inObject).IsEmpty() && (*ToV8Persistent(inObject))->IsObject())
	{
		Local<Object>		locObj = (*ToV8Persistent(inObject))->ToObject();
#else
	if (inObject && inObject->IsObject())
	{
		Local<Object>		locObj = inObject->ToObject();
#endif
		Local<String>		constName = locObj->GetConstructorName();
		String::Utf8Value	utf8Name(constName);
		isSame = !::strcmp(*utf8Name, inClassName);
		if (!isSame)
		{
			DebugMsg("InternalObjectIsObjectOfClass '%s' != '%s'\n", inClassName, *utf8Name);
		}
	}
	return isSame;
}


#if V8_USE_MALLOC_IN_PLACE


VJSValue::~VJSValue()
{
	DecVals();
	//DebugMsg("VJSValue::~VJSValue( CALLED!!!!\n");
	ReleaseRefCountable(&fValue);
}

VJSValue::VJSValue(const VJSContext& inContext)
:fContext(inContext)
{
	IncVals();
	//DebugMsg("VJSValue::VJSValue1( CALLED!!!!\n");
	fValue = new PersistentValue();
}


VJSValue::VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException )
:fContext(inContext)
{
	IncVals();
	//DebugMsg("VJSValue::VJSValue2( CALLED!!!!\n");
	fValue = inException.GetValue();
	fValue->Retain();
}

VJSValue::VJSValue( const VJSValue& inOther)
:fContext( inOther.fContext)
{
	IncVals();
	//DebugMsg("VJSValue::VJSValue3( CALLED!!!!\n");
	
	fValue = inOther.fValue;
	fValue->Retain();
}

VJSValue&	VJSValue::operator=( const VJSValue& inOther)
{
	//DebugMsg("VJSValue::VJSValue3bin( CALLED!!!!\n");
	fContext = inOther.fContext;

	ReleaseRefCountable(&fValue);
	fValue = inOther.fValue;
	fValue->Retain();
	return *this;
}

bool VJSValue::operator ==(const VJSValue& inOther) const
{
	const v8::Value& rhs = *(*ToV8Persistent(fValue));
	return rhs.Equals(*ToV8Persistent(inOther.fValue));
}


VJSValue::VJSValue( JS4D::ContextRef inContext) 
:fContext( inContext)
{
	IncVals();
	//DebugMsg("VJSValue::VJSValue4( CALLED!!!!\n");
	if (!inContext)
	{
		xbox_assert(inContext);
	}
	fValue = new PersistentValue();
}

VJSValue::VJSValue(JS4D::ContextRef inContext, const v8::Handle<v8::Value>& inValue)
:fContext(inContext)
{
	IncVals();
	//DebugMsg("VJSValue::VJSValue5( CALLED!!!!\n");

	fValue = new PersistentValue(inContext,inValue);
}

VJSValue::VJSValue(JS4D::ContextRef inContext, JS4D::ValueRef inValue)
:fContext(inContext)
,fValue(inValue)
{
	IncVals();
	if (fValue)
	{
		fValue->Retain();
	}
}

#else
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

/*
static void ghCbk(const WeakCallbackData<Value, void>& data)
{
	int a = 1;
}
static void ghRevivCbk(const WeakCallbackData<Value, void>& data)
{
	//intptr_t memAmount = data.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(-1 * 2048);
	//DebugMsg("ghRevivCbk memAmount=%u\n", (uintptr_t)memAmount);
	Value*	tmpVal = data.GetValue().val_;
	void* tmp = data.GetParameter();
	int a = 1;
}
//WeakReferenceCallbacks<Value, void>::Revivable
*/

void VJSValue::Set(JS4D::ValueRef inValue)
{
	xbox_assert(fValue == NULL);
	if (inValue)
	{
		//intptr_t memAmount = fContext->AdjustAmountOfExternalAllocatedMemory(1 * 2048);
		//DebugMsg("VJSValue::Set memAmount=%u\n", (uintptr_t)memAmount);
		internal::Object** p = reinterpret_cast<internal::Object**>(inValue);
		fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext),
			p));
		/*V8::MakeWeak(
			reinterpret_cast<internal::Object**>(fValue),
			NULL,
			NULL,
			reinterpret_cast<V8::RevivableCallback>(ghRevivCbk));*/
	}
}

VJSValue::~VJSValue()
{
	//DebugMsg("VJSValue::~VJSValue( CALLED!!!!\n");
	Dispose();
}

VJSValue::VJSValue(const VJSContext& inContext)
:fContext(inContext)
, fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue1( CALLED!!!!\n");
}

VJSValue::VJSValue(const VJSContext& inContext, const XBOX::VJSException& inException)
:fContext(inContext)
, fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue2( CALLED!!!!\n");
}

VJSValue::VJSValue(const VJSValue& inOther)
:fContext(inOther.fContext)
,fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue3( CALLED!!!!\n");
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
	CopyFrom(inOther);

}
VJSValue&	VJSValue::operator=(const VJSValue& inOther)
{
	//DebugMsg("VJSValue::VJSValue3bin( CALLED!!!!\n");
	fContext = inOther.fContext;
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);

	CopyFrom(inOther);

	return *this;
}

bool VJSValue::operator == (const VJSValue& inOther) const
{
	if (!fValue)
	{
		return (inOther.fValue == NULL);
	}
	return fValue->Equals(inOther.fValue);
}


VJSValue::VJSValue(JS4D::ContextRef inContext)
:fContext(inContext)
, fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue4( CALLED!!!!\n");
	if (!inContext)
	{
		xbox_assert(inContext);
	}
}

VJSValue::VJSValue(JS4D::ContextRef inContext, const v8::Handle<v8::Value>& inValue)
:fContext(inContext)
,fValue(NULL)
{
	//DebugMsg("VJSValue::VJSValue5( CALLED!!!!\n");
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);

	Set(*inValue);
}


#endif

			
bool VJSValue::IsUndefined() const
{
#if V8_USE_MALLOC_IN_PLACE
	return ToV8Persistent(fValue).IsEmpty() || (*ToV8Persistent(fValue))->IsUndefined();
#else
	return (fValue == NULL) || fValue->IsUndefined();
#endif
}

bool VJSValue::IsNull() const
{
#if V8_USE_MALLOC_IN_PLACE
	return ToV8Persistent(fValue).IsEmpty() || (*ToV8Persistent(fValue))->IsNull();
#else
	return (fValue == NULL) || fValue->IsNull();
#endif
}

bool VJSValue::IsNumber() const
{ 
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsNumber();
#else
	return (fValue != NULL) && fValue->IsNumber();
#endif
}

bool VJSValue::IsBoolean() const
{ 
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsBoolean();
#else
	return (fValue != NULL) && fValue->IsBoolean();
#endif
}

bool VJSValue::IsString() const		
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsString();
#else
	return (fValue != NULL) && fValue->IsString();
#endif
}

bool VJSValue::IsObject() const		
{ 
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsObject();
#else
	return (fValue != NULL) && fValue->IsObject();
#endif
}

bool VJSValue::IsArray() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsArray();
#else
	return fValue ? fValue->IsArray() : false;
#endif
}

bool VJSValue::IsInstanceOfBoolean() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsBooleanObject();
#else
	return fValue ? fValue->IsBooleanObject() : false;
#endif
}
bool VJSValue::IsInstanceOfNumber() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsNumberObject();
#else
	return fValue ? fValue->IsNumberObject() : false;
#endif
}
bool VJSValue::IsInstanceOfString() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsStringObject();
#else
	return fValue ? fValue->IsStringObject() : false;
#endif
}
bool VJSValue::IsInstanceOfDate() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsDate();
#else
	return fValue ? fValue->IsDate() : false;
#endif
}
bool VJSValue::IsInstanceOfRegExp() const
{
#if V8_USE_MALLOC_IN_PLACE
	return (*ToV8Persistent(fValue))->IsRegExp();
#else
	return fValue ? fValue->IsRegExp() : false;
#endif
}

void *VJSValue::InternalGetObjectPrivateData( 	JS4D::ClassRef 	inClassRef,
                                                VJSException*	outException ) const
{
	V4DContext*	v8Ctx = (V4DContext*)fContext->GetData(0);
#if V8_USE_MALLOC_IN_PLACE
	Handle<Value> tmpVal = Handle<Value>::New(fContext, ToV8Persistent(fValue));
	void*		data = v8Ctx->GetObjectPrivateData(fContext, *tmpVal);
#else
	void*		data = v8Ctx->GetObjectPrivateData(fContext, fValue);
#endif
	return data;
}

void* VJSValue::GetObjectPrivateForClass(JS4D::ClassRef inClassRef) const
{
	if (!ToV8Persistent(fValue).IsEmpty() && InternalValueIsObjectOfClass(fContext, fValue, ((JS4D::ClassDefinition*)inClassRef)->className))
	{
		return InternalGetObjectPrivateData(inClassRef);
	}
	return NULL;
}


#endif

JS4D::EType VJSValue::GetType()
{
#if USE_V8_ENGINE
	return JS4D::GetValueType(fContext, *this);
#else
	return JS4D::GetValueType(fContext, fValue);
#endif
}


bool VJSValue::GetJSONValue( VJSONValue& outValue) const {
	return GetJSONValue( outValue, NULL );
}

bool VJSValue::GetJSONValue( VJSONValue& outValue, VJSException* outException) const { 
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	Local<Value> locVal = v8::Local<Value>::New(fContext, ToV8Persistent(fValue));
#else
	Handle<Value> locVal = v8::Handle<Value>::New(fContext, fValue);
#endif
	bool ok = JS4D::ValueToVJSONValue(fContext, locVal, outValue, outException);
#else
	bool ok = JS4D::ValueToVJSONValue(fContext, fValue, outValue, VJSException::GetExceptionRefIfNotNull(outException));
#endif
	return ok;
}

bool VJSValue::GetString(VString& outString, VJSException *outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
	if (!tmpVal.IsEmpty())
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope scope(fContext);
		Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope context_scope(ctx);
		Local<String> cvStr = (*tmpVal)->ToString();
		String::Value  value2(cvStr);
		outString = *value2;
		return true;
	}
	return false;
#else
	if (IsNull())
	{
		return false;
	}
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
	if (fValue)
	{
		Local<String> cvStr = fValue->ToString();
		String::Value  value2(cvStr);
		outString = *value2;
		//DebugMsg("VJSValue::GetString CALLED <%s>",*value2);
	}
	return (fValue != NULL);
#endif
#else
	return (fValue != NULL) ? JS4D::ValueToString(fContext, fValue, outString, VJSException::GetExceptionRefIfNotNull(outException)) : false;
#endif
}


bool VJSValue::GetLong(sLONG *outValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	*outValue = 0;
	if (!ToV8Persistent(fValue).IsEmpty())
	{
		const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
		TryCatch	tryCatch;
		Local<Number>	nb = (*tmpVal)->ToNumber();
		if (tryCatch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fContext, tryCatch.Exception());
			}
			else
			{
				tryCatch.ReThrow();
			}
		}
		else
		{
			double r = nb->Value();
			sLONG l = (sLONG)r;
			if (l == r)
			{
				*outValue = l;
			}
		}

		return (outException == NULL) || (outException->IsEmpty());
	}
	return false;
#else
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
	return (fValue != NULL) && ((outException == NULL) || (outException->IsEmpty()));
#endif
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException));
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
	return (fValue != NULL) && ((outException == NULL) || (outException->GetValue() == NULL));
#endif
}


bool VJSValue::GetLong8(sLONG8 *outValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	*outValue = 0;
	if (!ToV8Persistent(fValue).IsEmpty())
	{
		const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
		Local<Number>	nb = (*tmpVal)->ToNumber();
		TryCatch	tryCatch;
		if (tryCatch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fContext, tryCatch.Exception());
			}
			else
			{
				tryCatch.ReThrow();
			}
		}
		else
		{
			double r = nb->Value();
			sLONG8 l = (sLONG8)r;
			if (l == r)
			{
				*outValue = l;
			}
		}

		return (outException == NULL) || (outException->IsEmpty());
	}
	return false;
#else
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
	return (fValue != NULL) && ((outException == NULL) || (outException->IsEmpty()));
#endif

#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException));
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
	return (fValue != NULL) && ((outException == NULL) || (outException->GetValue() == NULL));
#endif
}


bool VJSValue::GetULong(uLONG *outValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	*outValue = 0;
	if (!ToV8Persistent(fValue).IsEmpty())
	{
		const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
		TryCatch	tryCatch;
		Local<Number>	nb = (*tmpVal)->ToNumber();
		if (tryCatch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fContext, tryCatch.Exception());
			}
			else
			{
				tryCatch.ReThrow();
			}
		}
		else
		{
			double r = nb->Value();
			uLONG l = (uLONG)r;
			if (l == r)
			{
				*outValue = l;
			}
		}
		return (outException == NULL) || (outException->IsEmpty());
	}
	return false;
#else
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
	return (fValue != NULL) && ((outException == NULL) || (outException->IsEmpty()));
#endif
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException));
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
	return (fValue != NULL) && ((outException == NULL) || (outException->GetValue() == NULL));
#endif
}



bool VJSValue::GetReal(Real *outValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	*outValue = 0.0;
	if (!ToV8Persistent(fValue).IsEmpty())
	{
		const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
		TryCatch	tryCatch;
		Local<Number>	nb = (*tmpVal)->ToNumber();
		if (tryCatch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fContext, tryCatch.Exception());
			}
			else
			{
				tryCatch.ReThrow();
			}
		}
		else
		{
			*outValue = nb->Value();
		}
		return (outException == NULL) || (outException->IsEmpty());
	}
#else
	if (fValue != NULL)
	{
		Local<Number>	nb = fValue->ToNumber();
		*outValue = nb->Value();
		return true;
	}
#endif
	return false;
#else
	if (fValue != NULL)
		*outValue = JSValueToNumber(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException));
	return (fValue != NULL) && ( (outException == NULL) || (outException->GetValue() == NULL) );
#endif
}



bool VJSValue::GetBool(bool *outValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(fValue).IsEmpty())
	{
		const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
		TryCatch	tryCatch;
		*outValue = (*tmpVal)->BooleanValue();
		if (tryCatch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fContext, tryCatch.Exception());
			}
			else
			{
				tryCatch.ReThrow();
			}
		}
		else
		{
			return true;
		}
	}
#else
	if (fValue != NULL)
	{
		HandleScope scope(fContext);
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope context_scope(ctx);
		*outValue = fValue->BooleanValue();
		return true;
	}
#endif
	return false;
#else
	if (fValue != NULL)
		*outValue = JSValueToBoolean( fContext, fValue);
	return (fValue != NULL);
#endif
}


bool VJSValue::GetTime(VTime& outTime, VJSException *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(fValue).IsEmpty() && (*ToV8Persistent(fValue))->IsDate())
	{
		Local<Value> locVal = v8::Local<Value>::New(fContext, ToV8Persistent(fValue));
		ok = JS4D::DateObjectToVTime(fContext, locVal, outTime, outException, false);
	}
#else
	if (fValue->IsDate())
	{
		Handle<Value> locVal = v8::Handle<Value>::New(fContext, fValue);
		ok = JS4D::DateObjectToVTime(fContext, locVal, outTime, outException, false);
	}
#endif

#else
	if (JS4D::ValueIsInstanceOf(fContext, fValue, CVSTR("Date"), VJSException::GetExceptionRefIfNotNull(outException)))
	{
		JSObjectRef dateObject = JSValueToObject(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException));
		ok = JS4D::DateObjectToVTime(fContext, dateObject, outTime, VJSException::GetExceptionRefIfNotNull(outException), false);
	}
#endif
	else
	{
		outTime.SetNull( true);
		ok = false;
	}

	return ok;
}


bool VJSValue::GetDuration(VDuration& outDuration, VJSException *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope scope(fContext);
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	Handle<Value>	tmpVal = Handle<Value>::New(fContext, ToV8Persistent(fValue));
#else
	Handle<Value>	tmpVal = Handle<Value>::New(fContext, fValue); 
#endif
	ok = JS4D::ValueToVDuration(fContext, tmpVal, outDuration, outException);
#else
	ok = JS4D::ValueToVDuration(fContext, fValue, outDuration, VJSException::GetExceptionRefIfNotNull(outException));
#endif
	return ok;
}


bool VJSValue::GetURL(XBOX::VURL& outURL, VJSException *outException) const
{
	VString path;
	if (!GetString( path, outException) && !path.IsEmpty())
		return false;
	
	JS4D::GetURLFromPath( path, outURL);

	return true;
}


VFile *VJSValue::GetFile( VJSException *outException) const
{
	JS4DFileIterator *fileIter = NULL;
#if USE_V8_ENGINE

	fileIter = GetObjectPrivateData<VJSFileIterator>(NULL);

	return (fileIter == NULL) ? NULL : fileIter->GetFile();
#else

	fileIter = GetObjectPrivateData<VJSFileIterator>(outException);

	return (fileIter == NULL) ? NULL : fileIter->GetFile();
#endif
}


VFolder* VJSValue::GetFolder(VJSException *outException) const
{
	JS4DFolderIterator *folderIter = NULL;
#if USE_V8_ENGINE
	folderIter = GetObjectPrivateData<VJSFolderIterator>(NULL);

	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
#else

	folderIter = GetObjectPrivateData<VJSFolderIterator>(outException);

	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
#endif
}

XBOX::VValueSingle*	VJSValue::CreateVValue(VJSException *outException, bool simpleDate) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	HandleScope scope(fContext);
	Local<Value> locVal = v8::Local<Value>::New(fContext, ToV8Persistent(fValue));
#else
	Handle<Value> locVal = v8::Handle<v8::Value>::New(fContext, fValue);
#endif
	return JS4D::ValueToVValue(fContext, locVal, outException, simpleDate);
#else
	return JS4D::ValueToVValue(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException), simpleDate);
#endif
}

void VJSValue::GetObject( VJSObject& outObject, VJSException *outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(outObject.fObject).Reset(fContext, ToV8Persistent(fValue));
	ReleaseRefCountable(&outObject.fObject);
	outObject.fObject = fValue;
	if (fValue)
		outObject.fObject->Retain();
#else
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
#endif
#else
	xbox_assert( outObject.fContext.fContext == outObject.fContext.fContext );
	outObject.SetObjectRef( ((fValue != NULL) && JSValueIsObject( fContext, fValue)) 
							? JSValueToObject(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException))
							: NULL);

	/*outObject = VJSObject(
					outObject.fContext.fContext ,
					( (fValue != NULL) && JSValueIsObject( fContext, fValue) )
					? JSValueToObject( fContext, fValue, outException)
					: NULL);*/
#endif
}


VJSObject VJSValue::GetObject( VJSException* outException ) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	const v8::Persistent<v8::Value>& tmpVal = ToV8Persistent(fValue);
	if (!ToV8Persistent(fValue).IsEmpty() && (*ToV8Persistent(fValue))->IsObject() )
	{
		VJSObject resultObj(fContext,fValue);
		this->GetObject(resultObj);
		return resultObj;
	}
	else
	{
		VJSObject resultObj(fContext);
		resultObj.MakeEmpty();
		return resultObj;
	}
#else
	if (fValue && fValue->IsObject())
	{
		//VJSObject resultObj(fContext, fValue);
		VJSObject resultObj(fContext);
		this->GetObject(resultObj);
		return resultObj;
	}
	else
	{
		VJSObject resultObj(fContext);
		resultObj.MakeEmpty();
		return resultObj;
	}
#endif
#else
	JS4D::ExceptionRef* exc = VJSException::GetExceptionRefIfNotNull(outException);
	VJSObject resultObj(fContext);
	if ((fValue != NULL) && JSValueIsObject( fContext, fValue))
		resultObj.SetObjectRef( JSValueToObject( fContext, fValue, exc));
	else
		//resultObj.SetObjectRef(JSValueToObject(fContext, JSValueMakeNull(fContext), outException));
		resultObj.MakeEmpty();
	return resultObj;
#endif
}

#if USE_V8_ENGINE
void VJSObject::SetPrivateData(void* inData, JS4D::ClassDefinition* inClassDef) const
{
	V4DContext::SetObjectPrivateData(fContext.fContext, *this,inData,inClassDef);
}

void* VJSObject::GetObjectPrivate() const
{
#if V8_USE_MALLOC_IN_PLACE
	return V4DContext::GetObjectPrivateData(fContext.fContext, (*ToV8Persistent(fObject)));
#else
	return V4DContext::GetObjectPrivateData(fContext.fContext, fObject);
#endif
}

void* VJSObject::GetObjectPrivateForClass(JS4D::ClassRef inClassRef) const
{
	if (InternalObjectIsObjectOfClass(fContext, fObject, ((JS4D::ClassDefinition*)inClassRef)->className))
	{
		return GetObjectPrivate();
	}
	return NULL;
}


void VJSValue::SetUndefined( VJSException *outException)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope			handle_scope(fContext);
    Handle<Context>		context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope		context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
	//xbox_assert(false);//TBC
	ToV8Persistent(fValue).Reset(fContext, v8::Undefined(fContext));
#else	
	Handle<Primitive>	undefVal = v8::Undefined(fContext);
	Dispose();
	Set(*undefVal);
#endif
}

void VJSValue::SetNull( VJSException *outException)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope			handle_scope(fContext);
    Handle<Context>		context = Handle<Context>::New(fContext,*v8PersContext);
    Context::Scope		context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
	//xbox_assert(false);//TBC
	ToV8Persistent(fValue).Reset(fContext, v8::Null(fContext));
#else
	Handle<Primitive>	nullVal = v8::Null(fContext);
	
	Dispose();
	Set(*nullVal);
#endif
}

void VJSValue::SetFile( XBOX::VFile *inFile, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFileToObject(tmpObj, inFile, outException);
#if USE_V8_ENGINE
	*this = tmpObj;
#else
	JS4D::ObjectToValue(*this, tmpObj);
#endif
}

void VJSValue::SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException)
{
	VJSObject	tmpObj(fContext);
	JS4D::VFilePathToObjectAsFileOrFolder(	tmpObj,
											inPath,
											outException);
#if USE_V8_ENGINE
	*this = tmpObj;
#else
	JS4D::ObjectToValue(*this, tmpObj);
#endif
}


void VJSValue::SetString( const XBOX::VString& inString, VJSException *outException)
{ 
	JS4D::VStringToValue(inString,*this);
}


void VJSValue::SetTime( const XBOX::VTime& inTime, VJSException *outException, bool simpleDate)
{
	double tmp = JS4D::VTimeToDouble(inTime, simpleDate);
#if V8_USE_MALLOC_IN_PLACE
	//xbox_assert(false);//TBC
	if (ToV8Persistent(fValue).IsEmpty())
	{
		ReleaseRefCountable(&fValue);
		fValue = new PersistentValue(fContext, v8::Date::New(fContext, tmp));
	}
	else
	{
		ToV8Persistent(fValue).Reset(fContext, v8::Date::New(fContext, tmp));
	}
#else
	Dispose();
	Handle<Value>	tmpDate = v8::Date::New(fContext, tmp);
	if (*tmpDate)
	{
		Set(*tmpDate);
	}
#endif
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
#if  USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	v8::HandleScope handle_scope(fContext);
	Handle<Context>	ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);

	Handle<v8::Boolean>	tmpBool = v8::Boolean::New(fContext, inValue);
#if V8_USE_MALLOC_IN_PLACE
	if (ToV8Persistent(fValue).IsEmpty())
	{
		ReleaseRefCountable(&fValue);
		fValue = new PersistentValue(fContext, tmpBool);
	}
	else
	{
		ToV8Persistent(fValue).Reset(fContext, tmpBool);
	}
#else
	if (fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fValue));
		fValue = NULL;
	}
	if (*tmpBool)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(*tmpBool);
		fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
#endif
#else
	JS4D::BoolToValue(*this, inValue);
#endif
}

void VJSValue::SetVValue(const XBOX::VValueSingle& inValue, VJSException *outException, bool simpleDate)
{
#if USE_V8_ENGINE
	JS4D::VValueToValue(*this, inValue, outException, simpleDate);
#else
	JS4D::VValueToValue(*this, inValue, VJSException::GetExceptionRefIfNotNull(outException), simpleDate);
#endif
}

void VJSValue::SetJSONValue( const VJSONValue& inValue, VJSException *outException)
{
#if USE_V8_ENGINE
	JS4D::VJSONValueToValue(*this, inValue, outException);
#else
	JS4D::VJSONValueToValue(*this, inValue, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}

void VJSValue::SetFolder( XBOX::VFolder *inFolder, VJSException *outException)
{
	VJSObject tmpObj(fContext);
#if USE_V8_ENGINE
	JS4D::VFolderToObject(tmpObj, inFolder, outException);
	*this = tmpObj;
#else
	JS4D::VFolderToObject(tmpObj, inFolder, VJSException::GetExceptionRefIfNotNull(outException));
	fValue = JS4D::ObjectToValue(fContext, tmpObj.fObject);
#endif
}

bool VJSValue::IsInstanceOf(const XBOX::VString& inConstructorName, VJSException *outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	//HandleScope		hs(fContext);
	//Local<Value> locVal = v8::Local<Value>::New(fContext, ToV8Persistent(fValue));
	//if (locVal->IsObject())
	if (!ToV8Persistent(fValue).IsEmpty() && (*ToV8Persistent(fValue))->IsObject())
	{
		xbox_assert(v8::Isolate::GetCurrent() == fContext);
		V4DContext*				v8Ctx = (V4DContext*)fContext->GetData(0);
		Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(fContext);
		HandleScope				handleScope(fContext);
		Local<Context>			locCtx = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(locCtx);
		//Handle<Object> obj =	locVal->ToObject();
		Handle<Object> obj = (*ToV8Persistent(fValue))->ToObject();
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Value	constrNameValue(constrName);
		VString				constrNameStr(*constrNameValue);
		if (inConstructorName == constrNameStr)
		{
			return true;
		}
	}
#else
	if (fValue->IsObject())
	{
		xbox_assert(v8::Isolate::GetCurrent() == fContext);
		V4DContext*				v8Ctx = (V4DContext*)fContext->GetData(0);
		Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(fContext);
		HandleScope				handleScope(fContext);
		Local<Context>			locCtx = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(locCtx);
		Handle<Object> obj = fValue->ToObject();
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Value	constrNameValue(constrName);
		VString				constrNameStr(*constrNameValue);
		if (inConstructorName == constrNameStr)
		{
			return true;
		}
	}
#endif
	return false;
#else
	return JS4D::ValueIsInstanceOf(fContext, fValue, inConstructorName, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}

bool VJSValue::IsFunction() const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	return (!ToV8Persistent(fValue).IsEmpty() && (*ToV8Persistent(fValue))->IsFunction());
#else
	return ( fValue ? fValue->IsFunction() : false );
#endif
#else
	JSObjectRef obj = ( (fValue != NULL) && JSValueIsObject( fContext, fValue) ) ? JSValueToObject( fContext, fValue, NULL) : NULL;
	return (obj != NULL) ? JSObjectIsFunction( fContext, obj) : false;
#endif
}

#if USE_V8_ENGINE

#if V8_USE_MALLOC_IN_PLACE
VJSObject::~VJSObject()
{
	DecObjs(this);
	//DebugMsg("VJSObject::~VJSObject called\n");
	ReleaseRefCountable(&fObject);

}

VJSObject::VJSObject(JS4D::ContextRef inContext)
:fContext(inContext)
{
	IncObjs(this);
	fObject = new PersistentValue();
}


VJSObject::VJSObject( const VJSContext* inContext)
:fContext((JS4D::ContextRef)NULL)
{
	IncObjs(this);

	//DebugMsg("VJSObject::VJSObject4( CALLED!!!!\n");
	fObject = new PersistentValue();
	if (inContext)
	{
		fContext = VJSContext(*inContext);
	}
}

VJSObject::VJSObject( const VJSContext& inContext)
:fContext(inContext)
{
	IncObjs(this);

	//DebugMsg("VJSObject::VJSObject4bis( CALLED!!!!\n");
	fObject = new PersistentValue();
}

VJSObject::VJSObject( const VJSObject& inOther)
:fContext(inOther.fContext)
{
	IncObjs(this);

	//DebugMsg("VJSObject::VJSObject5( CALLED!!!!\n");
	fObject = inOther.fObject;
	fObject->Retain();
}

VJSObject& VJSObject::operator=(const VJSObject& inOther)
{ 
	fContext = inOther.fContext; 

	//DebugMsg("VJSObject::operator=( CALLED!!!!\n");
	//xbox_assert(fContext == inOther.fContext);
	ReleaseRefCountable(&fObject);
	fObject = inOther.fObject;
	fObject->Retain();
	return *this; 
}

VJSObject::VJSObject(const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes) // build an empty object as a property of a parent object
:fContext(inParent.fContext)
{
	IncObjs(this);
	//DebugMsg("VJSObject::VJSObject6( CALLED!!!!\n");
	fObject = new PersistentValue();
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}

VJSObject::VJSObject( const VJSArray& inParent, sLONG inPos) // build an empty object as an element of an array (-1 means push at the end)
:fContext(inParent.GetContext())
{
	IncObjs(this);
	//DebugMsg("VJSObject::VJSObject7( CALLED!!!!\n");
	fObject = new PersistentValue();
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}

VJSObject::VJSObject(JS4D::ContextRef inContext, const v8::Handle<v8::Object>& inObject)
:fContext(inContext)
{
	IncObjs(this);
	HandleScope scope(fContext);
	fObject = new PersistentValue(inContext,inObject);
}

VJSObject::VJSObject(JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
:fContext(inContext)
{
	IncObjs(this);
	fObject = inObject;
	if (fObject)
	{
		fObject->Retain();
	}
}

#else

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
		//intptr_t memAmount = fContext.fContext->AdjustAmountOfExternalAllocatedMemory(1*2048);
		//DebugMsg("VJSObject::Set memAmount=%u\n", (uintptr_t)memAmount);
		internal::Object** p = reinterpret_cast<internal::Object**>(inValue);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext.fContext),
			p));
		/*V8::MakeWeak(reinterpret_cast<internal::Object**>(fObject),
			NULL,
			NULL,
			reinterpret_cast<V8::RevivableCallback>(ghRevivCbk));*/
	}
}

VJSObject::VJSObject(JS4D::ContextRef inContext)
:fContext(inContext)
,fObject( NULL)	
{
}


VJSObject::VJSObject(JS4D::ContextRef inContext, const v8::Handle<v8::Object>& inObject)
:fContext(inContext)
,fObject(NULL)
{
	HandleScope scope(fContext);
	Set(*inObject);
}


VJSObject::VJSObject(const VJSContext* inContext)
:fContext((JS4D::ContextRef)NULL)
,fObject(NULL)
{
	//DebugMsg("VJSObject::VJSObject4( CALLED!!!!\n");
	if (inContext)
	{
		fContext = VJSContext(*inContext);
	}
}

VJSObject::VJSObject(const VJSContext& inContext)
:fContext(inContext)
,fObject( NULL)
{
	//DebugMsg("VJSObject::VJSObject4bis( CALLED!!!!\n");
}

VJSObject::VJSObject(const VJSObject& inOther)
:fContext(inOther.fContext)
,fObject(NULL)
{
	//DebugMsg("VJSObject::VJSObject5( CALLED!!!!\n");
	CopyFrom(inOther);
}


VJSObject& VJSObject::operator=(const VJSObject& inOther)
{ 
	/*xbox_assert( fContext == inOther.fContext);*/ 
	fContext = inOther.fContext; 

	//DebugMsg("VJSObject::operator=( CALLED!!!!\n");
	xbox_assert(fContext == inOther.fContext);
	CopyFrom(inOther);

	return *this; 
}

VJSObject::VJSObject(const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes) // build an empty object as a property of a parent object
:fContext(inParent.fContext)
,fObject( NULL)
{
	//DebugMsg("VJSObject::VJSObject6( CALLED!!!!\n");
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}

VJSObject::VJSObject(const VJSArray& inParent, sLONG inPos) // build an empty object as an element of an array (-1 means push at the end)
:fContext(inParent.GetContext())
,fObject( NULL)
{
	//DebugMsg("VJSObject::VJSObject7( CALLED!!!!\n");
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}

#endif

#else

VJSObject::VJSObject(JS4D::ContextRef inContext)
:fContext(inContext)
, fObject(NULL)
{
}


VJSObject::VJSObject(JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
:fContext(inContext)
, fObject(inObject)
{
}


VJSObject::VJSObject(const VJSContext& inContext, JS4D::ObjectRef inObject)
:fContext(inContext)
,fObject(inObject)
{
}

VJSObject::VJSObject(const VJSContext* inContext)
:fContext((JS4D::ContextRef)NULL)
,fObject(NULL)
{
	if (inContext)
	{
		fContext = VJSContext(*inContext);
	}
}

VJSObject::VJSObject( const VJSContext& inContext)
:fContext(inContext)
, fObject(NULL)
{
}

VJSObject::VJSObject(const VJSObject& inOther)
:fContext(inOther.fContext)
,fObject(inOther.fObject)
{
}

VJSObject& VJSObject::operator=(const VJSObject& inOther)
{
	/*xbox_assert( fContext == inOther.fContext);*/
	fContext = inOther.fContext;
	fObject = inOther.fObject;

	return *this;
}

VJSObject::VJSObject(const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes) // build an empty object as a property of a parent object
:fContext(inParent.fContext)
{
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}

VJSObject::VJSObject(const VJSArray& inParent, sLONG inPos) // build an empty object as an element of an array (-1 means push at the end)
:fContext(inParent.GetContext())
{
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}

#endif


//======================================================
VJSObject::operator VJSValue() const				
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(fObject).IsEmpty())
	{
		VJSValue newVal(fContext, fObject);
		return newVal;
	}
	else
	{
		return VJSValue(fContext);
	}
#else
	VJSValue	newVal(fContext);
	if (fObject)
	{
		newVal.CopyFrom(fObject);
	}
	return newVal;
#endif
#else
	return VJSValue(fContext, fObject);
#endif
}


#if USE_V8_ENGINE

bool VJSObject::operator == (const VJSObject& inOther) const
{
#if V8_USE_MALLOC_IN_PLACE
	//const v8::Value& rhs = *(*ToV8Persistent(fObject))->ToObject());
	const v8::Value& rhs = *(*ToV8Persistent(fObject));
	return rhs.Equals((*ToV8Persistent(inOther.fObject))->ToObject());
#else
	if (!fObject)
	{
		return (inOther.fObject == NULL);
	}
	return fObject->Equals(inOther.fObject);
#endif
}

#if V8_USE_MALLOC_IN_PLACE
bool VJSObject::HasRef() const
{
	return !ToV8Persistent(fObject).IsEmpty();
}

void VJSObject::ClearRef()				
{
	ToV8Persistent(fObject).Reset();
	//ReleaseRefCountable(&fObject);
}

#else
bool VJSObject::HasRef() const
{

	return fObject != NULL;
}
void VJSObject::ClearRef()
{

	Dispose();
}
#endif


#else
bool VJSObject::HasRef() const
{

	return fObject != NULL;
}
void VJSObject::ClearRef()
{
	fObject = NULL;

}
#endif



bool VJSObject::IsInstanceOf( const XBOX::VString& inConstructorName, VJSException *outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	const v8::Value& rhs = *(*ToV8Persistent(fObject));
	if ( rhs.IsObject() )
#else
	if (testAssert(fObject) && testAssert(fObject->IsObject()))
#endif
	{
		if (inConstructorName == "Array")
		{
#if V8_USE_MALLOC_IN_PLACE
			return rhs.IsArray();
#else
			return fObject->IsArray();
#endif
		}
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object> obj = rhs.ToObject();
#else
		Handle<Object> obj = fObject->ToObject();
#endif
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Value  valueStr(constrName);
		VString tmp(*valueStr);
		if (inConstructorName.EqualToString(tmp, true))
		{
			return true;
		}
		else
		{
			DebugMsg("VJSObject::IsInstanceOf strcmp NOK for '%S'\n", &inConstructorName);
		}
	}
	for (;;)
	{
		DebugMsg("VJSObject::IsInstanceOf NOK for '%S'\n", &inConstructorName);
	}
	return false;

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

bool VJSObject::CallMemberFunction(	
						const XBOX::VString& inFunctionName, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult, 
						VJSException* outException)
{
	return CallFunction(GetPropertyAsObject(inFunctionName, outException), inValues, outResult, outException);
}

bool VJSObject::CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
						const XBOX::VString& inFunctionName, 
						const std::vector<VJSValue> *inValues, 
						VJSValue *outResult)
{
	return CallMemberFunction(inFunctionName, inValues, outResult, NULL);
}


void VJSObject::MakeFile(VFile *inFile, VJSException *outException)
{
	if (testAssert( inFile != NULL))
	{
#if USE_V8_ENGINE
		JS4D::VFileToObject(*this, inFile, outException);
#else
		JS4D::VFileToObject(*this, inFile, VJSException::GetExceptionRefIfNotNull(outException));
#endif
	}
}

void VJSObject::MakeFolder(VFolder *inFolder, VJSException *outException)
{ 
	if (testAssert( inFolder != NULL))
	{
#if USE_V8_ENGINE
		JS4D::VFolderToObject(*this, inFolder, outException);
#else
		JS4D::VFolderToObject(*this, inFolder, VJSException::GetExceptionRefIfNotNull(outException));
#endif
	}
}

void VJSObject::MakeFilePathAsFileOrFolder(const VFilePath& inPath, VJSException *outException)
{ 
	if (testAssert( inPath.IsValid()))
	{
#if USE_V8_ENGINE
		JS4D::VFilePathToObjectAsFileOrFolder(*this, inPath, outException);
#else
		JS4D::VFilePathToObjectAsFileOrFolder(*this, inPath, VJSException::GetExceptionRefIfNotNull(outException));
#endif
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
#if V8_USE_MALLOC_IN_PLACE
	Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
    Local<Object>			obj = fObject->ToObject();
#endif
    Handle<Number>			propVal = Number::New(fContext,static_cast<double>(inValue));

	obj->Set(String::NewFromTwoByte(fContext, inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()), propVal);
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
#if V8_USE_MALLOC_IN_PLACE
	Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
	Local<Object>			obj = fObject->ToObject();
#endif
	obj->Set(	String::NewFromTwoByte(fContext,inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()),
				String::NewFromTwoByte(fContext, inValue.GetCPointer(), v8::String::kNormalString, inValue.GetLength()));
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
#if V8_USE_MALLOC_IN_PLACE
	Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
    Local<Object>			obj = fObject->ToObject();
#endif
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
#if V8_USE_MALLOC_IN_PLACE
	if (testAssert(!ToV8Persistent(inValue.fValue).IsEmpty()))
#else
	if (testAssert(inValue.fValue && fObject->IsObject()))
#endif
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
		Handle<Value>			propVal = Handle<Value>::New(fContext,ToV8Persistent(inValue.fValue));
#else
		Local<Object>			obj = fObject->ToObject();
		Handle<Value>			propVal = Handle<Value>::New(fContext, inValue.fValue);
#endif
		Local<String>			propStr = v8::String::NewFromTwoByte(fContext, inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength());
		obj->Set(propStr, propVal);
    }
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inValue.GetValueRef(), inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE
	//DebugMsg("VJSObject::SetProperty2 called for  prop=%S\n",&inPropertyName);
#if V8_USE_MALLOC_IN_PLACE
	if (testAssert(!ToV8Persistent(inObject.fObject).IsEmpty()))
#else
	if (testAssert(inObject.fObject && fObject->IsObject()))
#endif
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
		/*if (fObject->IsFunction())
		{
			int a=1;
		}*/
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
		Handle<Object>			propVal = (*ToV8Persistent(inObject.fObject))->ToObject();
#else
		Local<Object>			obj = fObject->ToObject();
		Handle<Value>			propVal = Handle<Value>::New(fContext,inObject.fObject);
		Local<Object>			locObj = inObject.fObject->ToObject();
#endif
		//bool isArray = locObj->IsArray();
		obj->Set(v8::String::NewFromTwoByte(fContext, inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()), propVal);
		v8PersContext->Reset(fContext, context);

		Local<Value>			propVal2 = obj->Get(v8::String::NewFromTwoByte(fContext,inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()));
		if (propVal2->IsNull())
		{
			int a = 1;
		}
		if (propVal2->IsUndefined())
		{
			int a = 1;
		}
		if (propVal2->IsObject())
		{
			int a = 1;
		}
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
#if V8_USE_MALLOC_IN_PLACE
	v8::Value*		arrayRef = 	*(*ToV8Persistent(inArray.fObject))->ToObject();
#else
	v8::Value*		arrayRef = inArray.fObject;
#endif
	if (testAssert(arrayRef->IsArray()))
	{
		Handle<Object>	locObj = arrayRef->ToObject();
		VJSObject	tmpObj(fContext.fContext, locObj);
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
#if V8_USE_MALLOC_IN_PLACE
	Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
    Local<Object>			obj = fObject->ToObject();
#endif
	Handle<v8::Boolean>		propVal = v8::Boolean::New(fContext.fContext,inBoolean);

	obj->Set(String::NewFromTwoByte(fContext,inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()), propVal);
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

VJSValue VJSObject::GetProperty(const XBOX::VString& inPropertyName, VJSException* outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	if (!ToV8Persistent(fObject).IsEmpty())
#else
	if (fObject && fObject->IsObject())
#endif
	{
		HandleScope				handle_scope(fContext);		
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext,*v8PersContext);
		Context::Scope			context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
		Local<Object>			obj = fObject->ToObject();
#endif
		Local<Value>			propVal = obj->Get(v8::String::NewFromTwoByte(fContext,inPropertyName.GetCPointer(), v8::String::kNormalString, inPropertyName.GetLength()));
		if (propVal->IsNull())
		{
			int a = 1;
		}
		if (propVal->IsUndefined())
		{
			int a = 1;
		}
		if (propVal->IsObject())
		{
			int a = 1;
		}
		VJSValue				result(fContext, propVal);
		return result;
	}
	return VJSValue(fContext);

#else
	JSStringRef jsName = JS4D::VStringToString(inPropertyName);
	JSValueRef valueRef = JSObjectGetProperty(fContext, fObject, jsName, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease(jsName);
	return VJSValue(fContext, valueRef);
#endif
}

VJSObject VJSObject::GetPropertyAsObject( const VString& inPropertyName, VJSException *outException) const
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	if (testAssert(!ToV8Persistent(fObject).IsEmpty()))
#else
	if (testAssert(fObject) && testAssert(fObject->IsObject()))
#endif
	{
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
		HandleScope				handle_scope(fContext);
		Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
		Context::Scope			context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object>			obj = (*ToV8Persistent(fObject))->ToObject();
#else
		Local<Object>			obj = fObject->ToObject();
#endif
		Local<Value>			propVal = obj->Get(v8::String::NewFromTwoByte(fContext, inPropertyName.GetCPointer()));
		if (propVal->IsObject())
		{
			VJSObject				result(fContext, propVal->ToObject());
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
	JSValueRef value = JSObjectGetProperty(fContext, fObject, jsName, VJSException::GetExceptionRefIfNotNull(outException));
	JSObjectRef objectRef = ((value != NULL) && JSValueIsObject(fContext, value)) ? JSValueToObject(fContext, value, VJSException::GetExceptionRefIfNotNull(outException)) : NULL;
	JSStringRelease( jsName);
	return VJSObject( fContext, objectRef);
#endif
}


bool VJSObject::GetPropertyAsBool(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, outException));
	bool res = false;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetBool(&res, outException);
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


sLONG VJSObject::GetPropertyAsLong(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, outException));
	sLONG res = 0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetLong(&res, outException);
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


bool VJSObject::GetPropertyAsString(const VString& inPropertyName, VJSException* outException, VString& outResult) const
{
	bool ok = false;
	VJSValue result(GetProperty(inPropertyName, outException));
	outResult.Clear();
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetString(outResult, outException);
	}
	return ok;
}



Real VJSObject::GetPropertyAsReal(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, outException));
	Real res = 0.0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetReal(&res, outException);
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
#if V8_USE_MALLOC_IN_PLACE
	if (testAssert(!ToV8Persistent(fObject).IsEmpty()))
	{
		v8::Handle<v8::Object>	locObj = (*ToV8Persistent(fObject))->ToObject();
#else
	if (testAssert(fObject->IsObject()))
	{
		v8::Local<v8::Object>	locObj = fObject->ToObject();
#endif
		v8::Local<v8::Array>	propsArray = locObj->GetPropertyNames();
		uint32_t	arrLength = propsArray->Length();
		for (uint32_t idx = 0; idx < arrLength; idx++)
		{
			Local<Value>	tmpVal = propsArray->Get(idx);
			if (tmpVal->IsString())
			{
				Local<String> propName = tmpVal->ToString();
				v8::String::Value  valueStr(propName);
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


bool VJSObject::DeleteProperty( const VString& inPropertyName, VJSException *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	DebugMsg("VJSObject::DeleteProperty called\n");
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	ok = JSObjectDeleteProperty(fContext, fObject, jsName, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
	return ok;
}



bool VJSObject::CallFunction (
	const VJSObject &inFunctionObject, 
	const std::vector<VJSValue> *inValues, 
	VJSValue *outResult, 
	VJSException *outException)
{
	bool ok = true;
    VJSException exception;
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope		context_scope(context);
#if	V8_USE_MALLOC_IN_PLACE
	Local<Object>		locFunc = (*ToV8Persistent(inFunctionObject.fObject))->ToObject();
#else
	Local<Object>		locFunc = inFunctionObject.fObject->ToObject();
#endif
	if (locFunc->IsCallable())
	{
		TryCatch try_catch;
		int count = ((inValues == NULL) || inValues->empty()) ? 0 : inValues->size();
		Handle<Value>* values = (count == 0) ? NULL : new Handle<Value>[count];
		if (values != NULL)
		{
			int j = 0;
			for (std::vector<VJSValue>::const_iterator i = inValues->begin(); i != inValues->end(); ++i)
			{
#if	V8_USE_MALLOC_IN_PLACE
				values[j++] = Handle<Value>::New(fContext,ToV8Persistent(i->fValue));
#else
				values[j++] = Handle<Value>::New(fContext, i->fValue);
#endif
			}
		}
		else
		{
			ok = (count == 0);
		}
#if	V8_USE_MALLOC_IN_PLACE
		Handle<Value>	objVal = (*ToV8Persistent(fObject))->ToObject();
#else
		Handle<Value>	objVal = Handle<Value>::New(fContext, fObject);
#endif
		Local<Object>	obj = objVal->ToObject();
		Handle<Value>	result = locFunc->CallAsFunction(obj, count, values);
		//v8::String::Utf8Value	resStr(result);
		//DebugMsg("VJSObject::CallFunction(isFunc=%d) result='%s' (isObj=%d)\n", inFunctionObject.fObject->IsFunction(), *resStr, result->IsObject());
		if (values)
		{
			delete[] values;
		}
		if (outResult != NULL)
		{
#if	V8_USE_MALLOC_IN_PLACE
			//ToV8Persistent(outResult->fValue).Reset(fContext, result);
			//xbox_assert(false);//TBC
			*outResult = VJSValue(fContext,result);
#else
			outResult->Dispose();
			outResult->Set(*result);
#endif
		}
		if (try_catch.HasCaught())
		{
			ok = false;
			if (outException != NULL)
			{
				outException->SetValue(fContext, try_catch.Exception());
			}
			try_catch.ReThrow();
		}
	}
	else
	{
		if (testAssert(inFunctionObject.IsFunction()))
		{
			TryCatch try_catch;
			int count = ((inValues == NULL) || inValues->empty()) ? 0 : inValues->size();
			Handle<Value>* values = (count == 0) ? NULL : new Handle<Value>[count];
			if (values != NULL)
			{
				int j = 0;
				for (std::vector<VJSValue>::const_iterator i = inValues->begin(); i != inValues->end(); ++i)
				{
#if	V8_USE_MALLOC_IN_PLACE
					values[j++] = Handle<Value>::New(fContext, ToV8Persistent(i->fValue));
#else
					values[j++] = Handle<Value>::New(fContext, i->fValue);
#endif
				}
			}
			else
			{
				ok = (count == 0);
			}
#if	V8_USE_MALLOC_IN_PLACE
			//Handle<Value>	objVal = (*ToV8Persistent(fObject));
			Local<Object>	obj = (*ToV8Persistent(fObject))->ToObject();//objVal->ToObject();
#else
			Handle<Value>	objVal = Handle<Value>::New(fContext, fObject);
			Local<Object>	obj = objVal->ToObject();
#endif
			Handle<Value>	result = locFunc->CallAsFunction(obj, count, values);
			if (values)
			{
				delete[] values;
			}
			if (outResult != NULL)
			{
#if	V8_USE_MALLOC_IN_PLACE
				//ToV8Persistent(outResult->fValue).Reset(fContext, result);
				xbox_assert(false);//TBC
#else
				outResult->Dispose();
				outResult->Set(*result);
#endif
			}
			if (try_catch.HasCaught())
			{
				ok = false;
				if (outException != NULL)
				{
					outException->SetValue(fContext,try_catch.Exception());
				}
				try_catch.ReThrow();
			}
		}
		else
		{
			ok = false;
			VString	excStr("Type Error : not a function");
			Handle<Value>	locExc = v8::Exception::Error(v8::String::NewFromTwoByte(fContext.fContext, excStr.GetCPointer(), v8::String::kNormalString));
			if (outException != NULL)
			{
				outException->SetValue(fContext,locExc);
			}
			Local<Value> locVal = fContext.fContext->ThrowException(locExc);
		}
	}

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

		result = JS4DObjectCallAsFunction( fContext, inFunctionObject.GetObjectRef(), fObject, count, values, VJSException::GetExceptionRefIfNotNull(&exception));

		delete[] values;
	}
	else
	{
		ok = false;
	}

	if (exception.GetValue() != NULL)
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
	xbox_assert(false);
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
#if V8_USE_MALLOC_IN_PLACE
	return ( !ToV8Persistent(fObject).IsEmpty() && (*ToV8Persistent(fObject))->IsArray() );
#else
	return fObject ? fObject->IsArray() : false;
#endif
#else
	return IsInstanceOf("Array");
#endif
}

bool VJSObject::IsFunction() const	// sc 03/09/2009 crash if fObject is null
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	return ( !ToV8Persistent(fObject).IsEmpty() && (*ToV8Persistent(fObject))->IsFunction());
#else
	return (fObject != NULL) ? fObject->IsFunction() : false;
#endif
#else
	return (fObject != NULL) ? JS4D::ObjectIsFunction( fContext, fObject) : false;
#endif
}


bool VJSObject::IsObject() const							
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	return ( !ToV8Persistent(fObject).IsEmpty() && (*ToV8Persistent(fObject))->IsObject());
#else
	return (fObject && fObject->IsObject());
#endif
#else
	return fObject != NULL; 
#endif
}


bool VJSObject::IsOfClass (JS4D::ClassRef inClassRef) const	
{ 
#if USE_V8_ENGINE
	return InternalValueIsObjectOfClass(fContext.fContext, fObject, ((JS4D::ClassDefinition*)inClassRef)->className);
#else
    return JS4D::ValueIsObjectOfClass( fContext, fObject, inClassRef);
#endif
}

void VJSObject::MakeEmpty()
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(fContext);
	Handle<Context>			context = Handle<Context>::New(fContext, *v8PersContext);
    Context::Scope			context_scope(context);
	Handle<Object>			emptyObj = v8::Object::New(fContext);
#if	V8_USE_MALLOC_IN_PLACE
	ToV8Persistent(fObject).Reset(fContext,emptyObj);
#else	
	Dispose();
	Set(*emptyObj);
#endif
#else
	fObject = JSObjectMake( fContext, NULL, NULL);
#endif
}

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

#if USE_V8_ENGINE

#else
JS4D::ObjectRef VJSObject::GetObjectRef() const									
{ 
	return fObject;
}

void VJSObject::SetObjectRef(JS4D::ObjectRef inObject)
{
	fObject = inObject;
}
#endif

		
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
    V4DContext* v8Ctx = (V4DContext*)(contextRef->GetData(0));

	xbox_assert(v8Ctx->GetIsolate() == v8::Isolate::GetCurrent());
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope				handle_scope(v8Ctx->GetIsolate());
	Local<Context>			context = v8::Handle<v8::Context>::New(v8Ctx->GetIsolate(),*v8PersContext);
	Context::Scope context_scope(context);
	ClearRef();
	Persistent<FunctionTemplate>*	callbackFn = v8Ctx->AddCallback(inCallbackFunction);
#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(fObject).Reset(fContext, (*(*callbackFn))->GetFunction());
	//xbox_assert(false);//TBC
	fObject = new PersistentValue(fContext,(*(*callbackFn))->GetFunction());
#else
	Set(*fn);
#endif
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
#if V8_USE_MALLOC_IN_PLACE
	Handle<Object>	locObj = (*ToV8Persistent(fObject))->ToObject();
#else
	Local<Object>	locObj = fObject->ToObject();
#endif
	if (testAssert(*locObj))
	{
		Local<Value>	protoVal = locObj->GetPrototype();
		if (testAssert(*protoVal) && testAssert(protoVal->IsObject()))
		{
			Local<Object>	protoObj = protoVal->ToObject();
			VJSObject	returnObj(inContext, protoObj);
			return returnObj;
		}
	}
#else
	if (IsObject())

		return XBOX::VJSObject( inContext, JS4D::ValueToObject( inContext, JSObjectGetPrototype(inContext, fObject), NULL));

	else
#endif
		return XBOX::VJSObject(inContext);
}

//======================================================

VJSPropertyIterator::VJSPropertyIterator(const VJSObject& inObject)
:fObject(inObject)
,fIndex(0)
,fCount(0)
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	if (testAssert(!ToV8Persistent(fObject.fObject).IsEmpty()))
#else
	if (testAssert(fObject.fObject->IsObject()))
#endif
	{
		HandleScope				handle_scope(fObject.fContext);
		Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fObject.fContext);
		Handle<Context>			context = Handle<Context>::New(fObject.fContext,*v8PersContext);
		Context::Scope			context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
		Handle<Object>			tmpObj = (*ToV8Persistent(fObject.fObject))->ToObject();
#else
		Local<Object>			tmpObj = fObject.fObject->ToObject();
#endif
		//V4DContext* v8Ctx = (V4DContext*)(fObject.fContext.GetContextRef()->GetData());
		Local<Array>	propsArray = tmpObj->GetPropertyNames();
		fCount = propsArray->Length();
		for (uint32_t idx = 0; idx < fCount; idx++)
		{
			Local<Value>	tmpVal = propsArray->Get(idx);
			if (tmpVal->IsString())
			{
				Local<String> propName = tmpVal->ToString();
				v8::String::Value  valueStr(propName);
				fNameArray.push_back(VString(*valueStr));
			}
			else
			{
				v8::String::Value  valueStr(tmpVal);
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
	return _GetProperty(outException);
}

void VJSPropertyIterator::SetProperty( const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, VJSException *outException) const 
{
	_SetProperty(
		inValue,
		inAttributes,
		outException);
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
	VJSValue	tmpVal = _GetProperty(outException);
	if (tmpVal.IsObject())
	{
		return tmpVal.GetObject(outException);
	}
	return VJSObject(fObject.GetContextRef());
}


VJSValue VJSPropertyIterator::_GetProperty(VJSException *outException) const
{
#if USE_V8_ENGINE
	//DebugMsg("VJSPropertyIterator::_GetProperty called for idx=%d\n", fIndex);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fObject.fContext);
	HandleScope scope(fObject.fContext);
	Handle<Context> ctx = Handle<Context>::New(fObject.fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	Local<Object>	locObj = (*ToV8Persistent(fObject.fObject))->ToObject();
#else
	Local<Object>	locObj = fObject.fObject->ToObject();
#endif
	Local<Value>	propVal = locObj->Get(v8::String::NewFromTwoByte(fObject.fContext, fNameArray[fIndex].GetCPointer(), v8::String::kNormalString, fNameArray[fIndex].GetLength()));
#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(propertyVal.fValue).Reset(fObject.fContext,propVal);
	//xbox_assert(false);//TBC
	return VJSValue(fObject.fContext, propVal);

#else
	propertyVal.Set(*propVal);
#endif

#else
	VJSValue	propertyVal(fObject.fContext);
	if (testAssert(fIndex < fCount))
		propertyVal.fValue = JSObjectGetProperty(fObject.GetContextRef(), fObject.GetObjectRef(), JSPropertyNameArrayGetNameAtIndex(fNameArray, fIndex), 
		VJSException::GetExceptionRefIfNotNull(outException));
	else
		propertyVal.fValue = NULL;
	return propertyVal;
#endif
}


void VJSPropertyIterator::_SetProperty(const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, VJSException *outException) const
{
#if USE_V8_ENGINE
	xbox_assert(false);
	DebugMsg("VJSPropertyIterator::_SetProperty called\n");
	for (;;)
		DebugMsg("%s  NOT IMPL\n", __PRETTY_FUNCTION__);
#else
	if (testAssert( fIndex < fCount))
		JSObjectSetProperty( fObject.GetContextRef(), fObject.GetObjectRef(), JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), inValue.GetValueRef(), inAttributes, 
			VJSException::GetExceptionRefIfNotNull(outException));
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
#if V8_USE_MALLOC_IN_PLACE
	//ToV8PersistentObj(fObject).Reset();
	ReleaseRefCountable(&fObject);
#else
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
	}
#endif
	Handle<Context> ctx = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope context_scope(ctx);
#if V8_USE_MALLOC_IN_PLACE
	//ToV8PersistentObj(fObject).Reset(fContext,ToV8PersistentObj(inOther.fObject));
	fObject = inOther.fObject;
	fObject->Retain();
#else
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
#endif
#else
	fObject = inOther.fObject;
#endif
	xbox_assert(fObject != NULL);
	return *this;
}


VJSArray::~VJSArray()
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	DecArrs();
	//ToV8PersistentObj(fObject).~Persistent();
	ReleaseRefCountable(&fObject);
#else
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = 0;
	}
#endif
#else
	fObject = NULL;
#endif
}

VJSArray::VJSArray(const VJSArray& inOther)
: fContext(inOther.fContext)
#if V8_USE_MALLOC_IN_PLACE
#else
, fObject(NULL)
#endif
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	IncArrs();
	//new (&fObject) v8::Persistent<v8::Object>(inOther.fContext,ToV8PersistentObj(inOther.fObject));
	fObject = inOther.fObject;
	fObject->Retain();
#else
	if (inOther.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inOther.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(fContext.fContext),
			p));
	}
#endif
#else
	fObject = inOther.fObject;
#endif
}

void VJSArray::SetNull()
{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	//ToV8PersistentObj(fObject).Reset();
	ReleaseRefCountable(&fObject);
#else
	if (fObject)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(fObject));
		fObject = NULL;
	}
#endif
#else
	fObject = NULL;
#endif
}


VJSArray::VJSArray(const VJSContext& inContext, VJSException* outException)
: fContext( inContext)
#if V8_USE_MALLOC_IN_PLACE
#else
, fObject(NULL)
#endif
{
#if USE_V8_ENGINE
	xbox_assert(v8::Isolate::GetCurrent() == inContext.fContext);
	V4DContext*				v8Ctx = (V4DContext*)inContext.fContext->GetData(0);
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
	HandleScope				handleScope(inContext);
	Handle<Context>			context = Handle<Context>::New(inContext, *v8PersContext);
	Context::Scope			context_scope(context);

	Local<v8::Array>	tmpArray = v8::Array::New(inContext.fContext,0);
#if V8_USE_MALLOC_IN_PLACE
	IncArrs();
	fObject = new PersistentValue();
	ToV8Persistent(fObject).Reset(inContext.fContext,tmpArray);//TBC
	//new (&fObject) Persistent<Object>(inContext.fContext,tmpArray);
#else
	if (testAssert(*tmpArray != NULL))
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(*tmpArray);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
		xbox_assert(fObject);
	}
#endif
	v8PersContext->Reset(fContext, context);
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
	Handle<v8::Array>	tmpArray = v8::Array::New(inContext,inValues.size());

#if V8_USE_MALLOC_IN_PLACE
	IncArrs();
	if (tmpArray.IsEmpty())
	{
		fObject = new PersistentValue;// new (&fObject) Persistent<Object>();
	}
	else
	{
		fObject = new PersistentValue;
		ToV8Persistent(fObject).Reset(inContext.fContext, tmpArray);
		//new (&fObject) Persistent<Object>(inContext.fContext,tmpArray);
		uint32_t	idx = 0;
		for (std::vector<VJSValue>::const_iterator it = inValues.begin(); it != inValues.end(); ++it)
		{
			Handle<Value> tmpVal = Handle<Value>::New(fContext, ToV8Persistent(it->fValue));
			(tmpArray)->Set(idx,tmpVal);
			idx++;
		}
	}
#else
	if (*tmpArray == NULL)
	{
		fObject = NULL;
	}
	else
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(*tmpArray);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
		uint32_t	idx = 0;
		for (std::vector<VJSValue>::const_iterator it = inValues.begin(); it != inValues.end(); ++it)
		{
			Handle<Value> tmpVal = Handle<Value>::New(fContext, it->fValue);
			tmpArray->Set(idx, tmpVal);
			idx++;
		}
	}
#endif
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
: fContext(inArrayObject.fContext)
{
	IncArrs();
	if (inCheckInstanceOfArray && !inArrayObject.IsInstanceOf(CVSTR("Array")))
#if USE_V8_ENGINE && V8_USE_MALLOC_IN_PLACE
		fObject = new PersistentValue;// new (&fObject) Persistent<Object>();
#else
		fObject = NULL;
#endif
	else
	{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
		//new (&fObject) Persistent<Object>(inArrayObject.fContext, ToV8PersistentObj(inArrayObject.fObject));
		fObject = inArrayObject.fObject;
		fObject->Retain();
#else
		internal::Object** p = reinterpret_cast<internal::Object**>(inArrayObject.fObject);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
#endif
#else
		fObject = inArrayObject.GetObjectRef();
#endif
	}
}


VJSArray::VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray)
: fContext( inArrayValue.GetContext())
{
	IncArrs();
	if (inCheckInstanceOfArray && !inArrayValue.IsInstanceOf( CVSTR( "Array")) )
#if USE_V8_ENGINE && V8_USE_MALLOC_IN_PLACE
		fObject = new PersistentValue;// new(&fObject) Persistent<Object>();
#else
		fObject = NULL;
#endif
	else
	{
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
		//Handle<Object> obj = (*ToV8Persistent(inArrayValue.fValue))->ToObject();
		//new (&fObject) Persistent<Object>(inArrayValue.GetContext(),obj);
		fObject = inArrayValue.fValue;
		fObject->Retain();
#else
		internal::Object** p = reinterpret_cast<internal::Object**>(inArrayValue.fValue);
		fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
#endif
#else
		VJSObject jobj(inArrayValue.GetObject());
		inArrayValue.GetObject(jobj);
		fObject = jobj.GetObjectRef();
#endif
	}
}

VJSArray::operator VJSObject() const
{ 
#if USE_V8_ENGINE
	VJSObject	newObj(fContext);
#if V8_USE_MALLOC_IN_PLACE
	//ToV8PersistentObj(newObj.fObject).Reset(fContext,ToV8PersistentObj(fObject));
	ReleaseRefCountable(&newObj.fObject);
	newObj.fObject = fObject;
	newObj.fObject->Retain();
#else
	if (fObject)
	{
		newObj.CopyFrom(fObject);
	}
#endif
	return newObj;
#else
	return VJSObject(fContext, fObject);
#endif
}

VJSArray::operator VJSValue() const	
{ 
#if USE_V8_ENGINE
	VJSValue	newVal(fContext);
#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(newVal.fValue).Reset(fContext,ToV8PersistentObj(fObject));
	ReleaseRefCountable(&newVal.fValue);
	newVal.fValue = fObject;
	newVal.fValue->Retain();
#else
	if (fObject)
	{
		newVal.CopyFrom(fObject);
	}
#endif
	return newVal;
#else
	return VJSValue(fContext, fObject); 
#endif
}

size_t VJSArray::GetLength() const
{
	size_t length = 0;
#if USE_V8_ENGINE
#if V8_USE_MALLOC_IN_PLACE
	const v8::Object& rhs = **(*ToV8Persistent(fObject))->ToObject();
	if (rhs.IsArray())
	{
		v8::Array* tmpArray = v8::Array::Cast(*ToV8Persistent(fObject));
		length = tmpArray->Length();
	}
#else
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);;
		length = tmpArray->Length();
	}
#endif
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
#if V8_USE_MALLOC_IN_PLACE
	const v8::Object& rhs = **(*ToV8Persistent(fObject))->ToObject();
	if (rhs.IsArray())
	{
		v8::Array* tmpArray = v8::Array::Cast(*ToV8Persistent(fObject));
		if (tmpArray && testAssert(inIndex < tmpArray->Length()) )
		{
			Local<Value>	locVal = tmpArray->Get(inIndex);
			//ToV8Persistent(result.fValue).Reset(fContext,locVal);
			//xbox_assert(false);//TBC
			return VJSValue(fContext, locVal);
		}
	}
	return VJSValue(fContext);

#else
	VJSValue	result(fContext);
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);
		if (tmpArray && testAssert(inIndex < tmpArray->Length()) )
		{
			Local<Value>	locVal = tmpArray->Get(inIndex);
			internal::Object** p = reinterpret_cast<internal::Object**>(*locVal);
			result.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
				p));
		}
	}
	return result;
#endif
#else
	return VJSValue(
		fContext,
		JSObjectGetPropertyAtIndex( fContext, fObject, inIndex, VJSException::GetExceptionRefIfNotNull(outException) ));
#endif
}


void VJSArray::SetValueAt( size_t inIndex, const VJSValue& inValue, VJSException *outException) const
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(fContext);
	HandleScope			handle_scope(fContext);
	Handle<Context>		context = Handle<Context>::New(fContext, *v8PersContext);
	Context::Scope		context_scope(context);
#if V8_USE_MALLOC_IN_PLACE
	const v8::Object& rhs = **(*ToV8Persistent(fObject))->ToObject();
	if (rhs.IsArray())
	{
		v8::Array* tmpArray = v8::Array::Cast(*ToV8Persistent(fObject));
		if (inIndex >= tmpArray->Length())
		{
		}
		Handle<Value>	tmpVal = Handle<Value>::New(fContext.fContext, ToV8Persistent(inValue.fValue));
		tmpArray->Set(inIndex, tmpVal);
	}
#else
	if (testAssert(fObject->IsArray()))
	{
		v8::Array* tmpArray = v8::Array::Cast(fObject);
		if (inIndex >= tmpArray->Length())
		{
		}
		Handle<Value>	tmpVal = Handle<Value>::New(fContext.fContext, inValue.fValue);
		tmpArray->Set(inIndex, tmpVal);
	}
#endif
#else
	JSObjectSetPropertyAtIndex( fContext, fObject, inIndex, inValue.GetValueRef(), VJSException::GetExceptionRefIfNotNull(outException) );
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
#if V8_USE_MALLOC_IN_PLACE
#else
,fMetaInfo(NULL)
#endif
{
	assert(inPict == NULL || inPict->GetValueKind() == VK_IMAGE);
	fPict = (VPicture*)inPict;
	fMetaInfoIsValid = false;
#if V8_USE_MALLOC_IN_PLACE
	//new (&fMetaInfo) v8::Persistent<v8::Object>();
#endif
	//fOwnsPict = ownsPict;
	fMetaBag = NULL;
}


VJSPictureContainer::~VJSPictureContainer()
{
	if (/*fOwnsPict &&*/ fPict != NULL)
		delete fPict;

#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(fMetaInfo).~Persistent();
#else
	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
#endif
	QuickReleaseRefCountable(fMetaBag);
}

#if  USE_V8_ENGINE
XBOX::VJSValue VJSPictureContainer::GetMetaInfo() const
{ 
	VJSValue result(fContext);
#if V8_USE_MALLOC_IN_PLACE
	//ToV8Persistent(result.fValue).Reset(fContext,ToV8PersistentObj(fMetaInfo));
#else
	internal::Object** p = reinterpret_cast<internal::Object**>(fMetaInfo);
	result.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
		reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
		p));
#endif
	return result;
}
#endif

#if  USE_V8_ENGINE
#else
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
#endif


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
#if USE_V8_ENGINE
	fParams.clear();
	fResult = VJSValue(fContext);
#else
	for (std::vector<VJSValue>::const_iterator i = fParams.begin(); i != fParams.end(); ++i)
		i->Unprotect();
	fParams.clear();

	fResult.Unprotect();
	fResult = VJSValue(fContext);
	JS4D::UnprotectValue(fContext, fException.GetValue());
#endif
	fException.Clear();
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
		VJSException exception;

		VJSValue result( fContext);
		fContext.GetGlobalObject().CallFunction( functionObject, &fParams, &result, &exception);

#if USE_V8_ENGINE
#else
		result.Protect();	// protect on stack before storing it in class field
		JS4D::ProtectValue( fContext, exception.GetValue());

		fResult.Unprotect();	// unprotect previous result we may have if Call() is being called twice.

		JS4D::UnprotectValue( fContext, fException.GetValue());
#endif

		fResult = result;
		fException = exception;
#if USE_V8_ENGINE
		called = exception.IsEmpty();
#else
		called = exception.GetValue() == NULL;
#endif
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
#if USE_V8_ENGINE
	if (!fException.IsEmpty())
#else
	if (fException.GetValue() != NULL)
#endif
	{
		outResult.SetUndefined();
		return false;
	}
	return fResult.GetJSONValue( outResult, &fException);
}



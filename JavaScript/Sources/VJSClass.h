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
#ifndef __VJSClass__
#define __VJSClass__


#include "VJSConstructor.h"

BEGIN_TOOLBOX_NAMESPACE


//======================================================

// Template for creating JSObjectCallAsFunctionCallback callbacks.

template<class PRIVATE_DATA_TYPE, void (*callback) (VJSParms_callStaticFunction &, PRIVATE_DATA_TYPE *)>
#if USE_V8_ENGINE
static void __cdecl js_callback( const v8::FunctionCallbackInfo<v8::Value>& args )
{
	XBOX::VJSParms_callStaticFunction	parms(args);
	PRIVATE_DATA_TYPE*	data = (PRIVATE_DATA_TYPE*)parms.GetObjectPrivate();
	callback(parms, data);
}
#else
static JS4D::ValueRef __cdecl js_callback (JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
{
	XBOX::VJSParms_callStaticFunction	parms(inContext, inFunction, inThis, inArgumentCount, inArguments, outException);

	callback(parms, (PRIVATE_DATA_TYPE *) JS4D::GetObjectPrivate(inThis));

	return parms.GetValueRefOrNull(inContext);
}
#endif


//======================================================

// to force single instantiation of sClass static variable (doesn't work on inline static member function, that's why one need another non-member function)
#if COMPIL_GCC
#define GCC_VISIBILITY_DEFAULT __attribute__((__visibility__("default")))
#else
#define GCC_VISIBILITY_DEFAULT
#endif

template<typename DERIVED> JS4D::ClassRef JS4DGetClassRef() GCC_VISIBILITY_DEFAULT;
template<typename DERIVED> JS4D::ClassRef JS4DGetClassRef()
{
	static JS4D::ClassRef sClass = NULL;
	if (sClass == NULL)
	{
		JS4D::ClassDefinition def;
		DERIVED::GetDefinition( def);
		sClass = JS4D::ClassCreate( &def);
	}
	return sClass;
}

typedef void (*fn_callAsFunction)( VJSParms_callAsFunction&);
typedef void (*fn_callAsConstructor)( VJSParms_callAsConstructor&);
typedef bool (*fn_hasInstance) (const VJSParms_hasInstance &);

#if USE_V8_ENGINE
typedef void (*GetPropertyFn)( XBOX::VJSParms_getProperty& ioParms, void* inData);
typedef void (*SetPropertyFn)( XBOX::VJSParms_setProperty& ioParms, void* inData);
XTOOLBOX_API JS4D::V8ObjectGetPropertyCallback* VJSClass__GetPropertyGetterCallback(GetPropertyFn inFunction);
XTOOLBOX_API JS4D::V8ObjectSetPropertyCallback* VJSClass__GetPropertySetterCallback(SetPropertyFn inFunction);
XTOOLBOX_API void* VJSClass__GetV8PropertyGetterCallback(JS4D::V8ObjectGetPropertyCallback* inCbk);
XTOOLBOX_API void* VJSClass__GetV8PropertySetterCallback(JS4D::V8ObjectSetPropertyCallback* inCbk);
XTOOLBOX_API void* VJSClass__GetV8PropertyHandlerCallback(JS4D::ObjectGetPropertyCallback cbkG, JS4D::ObjectSetPropertyCallback cbkS);

class XTOOLBOX_API VJSClassImpl
{
public:
	static	bool CallAsConstructor(JS4D::ClassRef inClassRef, const v8::FunctionCallbackInfo<v8::Value>& args, fn_callAsConstructor fn);
	static	VJSObject CreateInstanceFromPrivateData(const XBOX::VJSContext& inContext,JS4D::ClassRef inClassRef,void *inPrivateData);
	static	void UpdateInstanceFromPrivateData(JS4D::ContextRef inContextRef, const VJSObject& inObjRef, void* inPrivateData, JS4D::ClassDefinition* inClassDef);
	static	void CallStaticFunctionEpilog( const v8::FunctionCallbackInfo<v8::Value>& args);
	static	void* GetHolderPrivateData(const v8::PropertyCallbackInfo<v8::Array>& info);
};

#endif



template<class DERIVED, class PRIVATE_DATA_TYPE>
class VJSClass
{
	typedef void (*fn_initialize)( const VJSParms_initialize&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_finalize)( const VJSParms_finalize&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_getProperty)( VJSParms_getProperty&, PRIVATE_DATA_TYPE*);
	typedef bool (*fn_setProperty)( VJSParms_setProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_getPropertyNames)( VJSParms_getPropertyNames&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_hasProperty)( VJSParms_hasProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_deleteProperty)( VJSParms_deleteProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_callStaticFunction)( VJSParms_callStaticFunction&, PRIVATE_DATA_TYPE*);

	
public:
	typedef	JS4D::StaticValue			StaticValue;
	typedef	JS4D::StaticFunction		StaticFunction;
	typedef	JS4D::ClassDefinition		ClassDefinition;
	typedef	PRIVATE_DATA_TYPE			PrivateDataType;

	template<fn_initialize fn>
#if  USE_V8_ENGINE
	static void __cdecl js_initialize(JS4D::ContextRef inContext, const VJSObject& inObject)
	{
		VJSParms_initialize parms(inContext, inObject);
		fn(parms, (PrivateDataType*)inObject.GetObjectPrivate());
	}
#else
	static void __cdecl js_initialize(JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
	{
		VJSParms_initialize parms(inContext, inObject);
		fn(parms, (PrivateDataType*)JS4D::GetObjectPrivate(inObject));
	}
#endif


	template<fn_finalize fn>
#if  USE_V8_ENGINE
	static void js_finalize(void* inPrivateData)
	{
		VJSParms_finalize parms(0);
		fn(parms, (PrivateDataType*)inPrivateData);
#else
	static void __cdecl js_finalize( JS4D::ObjectRef inObject)
	{
		VJSParms_finalize parms( inObject);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
#endif
	}
	
	template<fn_getProperty fn>
#if  USE_V8_ENGINE
	static void js_getProperty (XBOX::VJSParms_getProperty* ioParms, void *inData)
	{
		fn(*ioParms, (PrivateDataType*)inData);
	}
#else
	static JS4D::ValueRef __cdecl js_getProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
	{
		VJSParms_getProperty parms( inContext, inObject, inPropertyName, outException);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
		return parms.GetValueRefOrNull(inContext);
	}
#endif

	
	template<fn_setProperty fn>
#if  USE_V8_ENGINE
	//static void js_setProperty( v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<v8::Value>& inInfo )
	static void js_setProperty(XBOX::VJSParms_setProperty* ioParms, void *inData)
	{
		fn(*ioParms, (PrivateDataType*)inData);
	}
#else
	static bool __cdecl js_setProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ValueRef inValue, JS4D::ExceptionRef* outException)
	{
		VJSParms_setProperty parms( inContext, inObject, inPropertyName, inValue, outException);
		return fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
	}
#endif

	template<fn_getPropertyNames fn>
#if  USE_V8_ENGINE
	static void __cdecl js_getPropertyNames(const v8::PropertyCallbackInfo<v8::Array>& info, std::vector<XBOX::VString>& outPropertyNames)
	{
		VJSParms_getPropertyNames parms(info, outPropertyNames);
		fn(parms, (PrivateDataType*)VJSClassImpl::GetHolderPrivateData(info));
	}
#else
	static void __cdecl js_getPropertyNames( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::PropertyNameAccumulatorRef inPropertyNames)
	{
		VJSParms_getPropertyNames parms( inContext, inObject, inPropertyNames);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
	}
#endif
  
	template<fn_hasProperty fn>
#if  USE_V8_ENGINE
	static void js_hasProperty(XBOX::VJSParms_hasProperty* ioParms, void *inData)
	{
		fn(*ioParms, (PrivateDataType*)inData);
	}
#else
	static bool __cdecl js_hasProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName)
	{
		VJSParms_hasProperty parms(inContext, inObject, inPropertyName);
		fn(parms, (PrivateDataType *) JS4D::GetObjectPrivate(inObject));
		return parms.GetReturnedBoolean();
	}
#endif

	template<fn_deleteProperty fn>
#if  USE_V8_ENGINE
	static bool js_deleteProperty(XBOX::VJSParms_deleteProperty* ioParms, void *inData)
	{
		fn(*ioParms, (PrivateDataType *)inData);
		return ioParms->GetReturnedBoolean();
	}
#else
	static bool __cdecl js_deleteProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
	{
		VJSParms_deleteProperty parms(inContext, inObject, inPropertyName, outException);
		fn(parms, (PrivateDataType *) JS4D::GetObjectPrivate(inObject));
		return parms.GetReturnedBoolean();
	}
#endif
			
	template<fn_callStaticFunction fn>
#if USE_V8_ENGINE
	static void js_callStaticFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
	{	
		VJSParms_callStaticFunction parms(args);
		VJSClassImpl::CallStaticFunctionEpilog(args);
		PrivateDataType*	data = (PrivateDataType*)parms.GetObjectPrivate();
		fn(parms,data);
	}
#else
	static JS4D::ValueRef __cdecl js_callStaticFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
	
	template<fn_callAsFunction fn>
#if USE_V8_ENGINE
	//static void __cdecl js_callAsFunction(VJSParms_callAsFunction* parms)
	static void __cdecl js_callAsFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
xbox::DebugMsg("js_callAsFunction called !!!\n");
		VJSParms_callAsFunction parms(args);
		fn(parms);
	}
#else
	static JS4D::ValueRef __cdecl js_callAsFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callAsFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		fn( parms);
		return parms.GetValueRefOrNull(inContext);
	}
#endif
	template<fn_callAsConstructor fn>
#if USE_V8_ENGINE
	static void __cdecl js_callAsConstructor(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
xbox::DebugMsg("js_callAsConstructor called !!!\n");
	    JS4D::ClassRef classRef = Class();
		VJSClassImpl::CallAsConstructor(classRef,args,fn);
        {
        }
	}
#else
static JS4D::ObjectRef __cdecl js_callAsConstructor( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callAsConstructor parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
		fn( parms);
		return parms.GetConstructedObject().GetObjectRef();
	}
#endif

	template<fn_hasInstance fn>
#if  USE_V8_ENGINE
	static bool __cdecl js_hasInstance()
	{
		// cannot be called on V8 since V8 already knows what class an object is an instanceof !!!!
xbox::DebugMsg("js_hasInstance called !!!\n");
		xbox_assert(false);
		return false;
	}
#else
	static bool __cdecl js_hasInstance(JS4D::ContextRef inContext, JS4D::ObjectRef inPossibleConstructor, JS4D::ValueRef inObjectToTest, JS4D::ExceptionRef *outException)
	{
		VJSParms_hasInstance	parms(inContext, inPossibleConstructor, inObjectToTest, outException);

		return fn(parms);
	}
#endif

	static	JS4D::ClassRef Class()
	{
		return JS4DGetClassRef<DERIVED>();
	}


	static	VJSObject ConstructInstance(const VJSParms_construct& inParms, PrivateDataType *inData)
	{
#if USE_V8_ENGINE
		JS4D::ClassRef	clRef = Class();
		JS4D::ClassDefinition* classDef = (JS4D::ClassDefinition*)clRef;
		xbox::DebugMsg("VJSClass::ConstructInstance called '%s' !!!\n", classDef->className);
		/*if (!strcmp(classDef->className, "File"))
		{
		if (!inData)
		{
		int a = 1;
		}
		}*/
		//JS4D::ObjectRef objRef = inParms.fConstructedObject.GetObjectRef();
		JS4D::ContextRef	ctxRef = inParms.GetContext();
		VJSClassImpl::UpdateInstanceFromPrivateData(ctxRef, inParms.fConstructedObject, inData, classDef);

		if (classDef->initialize)
		{
			classDef->initialize(ctxRef, inParms.fConstructedObject);
		}
		return inParms.fConstructedObject;
#else
		VJSContext	vjsContext(inParms.GetContext());
		return VJSObject(vjsContext, JS4D::MakeObject(vjsContext, Class(), inData));
#endif
	}

	static	VJSObject CreateInstance( const VJSContext& inContext, PrivateDataType *inData)
	{
#if USE_V8_ENGINE
		JS4D::ClassRef	clRef = Class();
		JS4D::ClassDefinition* classDef = (JS4D::ClassDefinition*)clRef;
//xbox::DebugMsg("VJSClass::CreateInstance called '%s' !!!\n",classDef->className);
if (!strcmp(classDef->className, "Storage"))
{
	if (!inData)
	{
		int a = 1;
	}
}
		VJSObject newObj = VJSClassImpl::CreateInstanceFromPrivateData(inContext, classDef,inData);
		//JS4D::ObjectRef objRef = newObj.GetObjectRef();	
		JS4D::ContextRef	ctxRef = inContext;
		if (classDef->initialize)
		{
			classDef->initialize(ctxRef,newObj);
		}
		return newObj;
#else
		return VJSObject( inContext, JS4D::MakeObject( inContext, Class(), inData));
#endif
	}

private:

};



END_TOOLBOX_NAMESPACE


#endif

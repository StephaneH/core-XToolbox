#if USE_V8_ENGINE


#ifndef V4D__CONTEXT__H__
#define V4D__CONTEXT__H__

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <v8.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif


#include "VJSContext.h"
#if VERSIONMAC
#include <map>
#else
#include <unordered_map>
#include <set>
#endif

#define K_EMPTY_URL "emptyURL"



BEGIN_TOOLBOX_NAMESPACE

#if V8_USE_MALLOC_IN_PLACE


class PersistentValue : public IRefCountable, public VNonCopyable<PersistentValue>
{
public:
	PersistentValue()
	{	
		fPersVal = new v8::Persistent<v8::Value>(); 
	}

	PersistentValue(v8::Isolate* inContext, const v8::Handle<v8::Value>& inValue)
	{
		fPersVal = new v8::Persistent<v8::Value>(inContext, inValue);
	}

	virtual ~PersistentValue()
	{
		fPersVal->Reset();
		delete fPersVal;
	}

	v8::Persistent<v8::Value>*		fPersVal;
private:

};

static inline v8::Persistent<v8::Value>&  ToV8Persistent(PersistentValue* inPersistentHandle)
{
	return *(inPersistentHandle->fPersVal);
}

static inline const v8::Persistent<v8::Value>&  ToV8Persistent(const PersistentValue* inPersistentHandle)
{
	return *(inPersistentHandle->fPersVal);
}

#endif

class V4DContext // final
{
public:
									V4DContext(v8::Isolate* inIsolate, XBOX::JS4D::ClassRef inGlobalClassRef);
									~V4DContext();

			void					Enter();
			void					Exit();

			bool					EvaluateScript(const XBOX::VString& inScript, const VString& inUrl, VJSValue *outResult, bool inCacheScript = true, XBOX::VJSException* outException=NULL);
			bool					CheckScriptSyntax(const XBOX::VString& inScript, const VString& inUrl);
			bool					HasDebuggerAttached();
			void					AttachDebugger();

			void					GetSourceData(sLONG inSourceID, XBOX::VString& outUrl);
			bool					GetSourceID(const XBOX::VString& inUrl, int& outID);

			v8::Persistent<v8::FunctionTemplate>*	GetImplicitFunctionConstructor(const xbox::VString& inName);

	static	void 					AddNativeFunction(const xbox::JS4D::StaticFunction& inStaticFunction);

	static	void 					AddNativeClass(xbox::JS4D::ClassDefinition* inClassDefinition);

	static	void 					AddNativeObjectTemplate(
										const xbox::VString& inName,
										v8::Handle<v8::ObjectTemplate>& inObjTemp);

	static	void*					GetHolderPrivateData(const v8::PropertyCallbackInfo<v8::Array>& info);
	static	void*					GetPrivateData(v8::Isolate* inIsolate, v8::Value* inValue);

			void						CollectGarbage();

			v8::Handle<v8::Object>		GetFunction(const char* inClassName);

			v8::Handle<v8::Function>	CreateFunctionTemplateForClass(JS4D::ClassRef	inClassRef,
				bool			inLockResources = true,
				bool			inAddPersistentConstructor = false);

			v8::Handle<v8::Function>	CreateFunctionTemplateForClassConstructor(
				JS4D::ClassRef inClassRef,
				const JS4D::ObjectCallAsFunctionCallback inConstructorCallback,
				void* inData,
				bool inAcceptCallAsFunction = false,
				JS4D::StaticFunction inContructorFunctions[] = NULL);


			void*					GetGlobalObjectPrivateInstance();
			void					SetGlobalObjectPrivateInstance(void* inPrivateData);
			void					DisposeGloBalObjectPrivateInstance(JS4D::ClassDefinition inClassDef);

			void*					GetObjectPrivateData(v8::Value* inObject);
			void					SetObjectPrivateData(const XBOX::VJSObject& inObject, void* inPrivateData, JS4D::ClassDefinition* inClassDef = NULL);

	static 	v8::Persistent< v8::Context, v8::NonCopyablePersistentTraits<v8::Context> >*		GetPersistentContext(v8::Isolate* inIsolate);
	static	void*					GetObjectPrivateData(v8::Isolate* inIsolate, v8::Value* inObject);
	static	void					SetObjectPrivateData(v8::Isolate* inIsolate, const VJSObject& inObject, void* inPrivateData, JS4D::ClassDefinition* inClassDef);
	static	void					DualCallFunctionHandler(const v8::FunctionCallbackInfo<v8::Value>& info);

			v8::Isolate*			GetIsolate() { return fIsolate; }

			v8::Persistent<v8::FunctionTemplate>*	AddCallback(JS4D::ObjectCallAsFunctionCallback inFunctionCallback);
			v8::Persistent<v8::Object>*				GetStringifyFunction() { return &fStringify; }

			xbox::VString			GetCurrentScriptUrl();

#define V8_CHECK_CONTEXT_REUSE 0
#if V8_CHECK_CONTEXT_REUSE
	void					DisplayStack();
	std::stack<xbox::VTaskID>	fThrIdStack;
#endif
	//std::set<xbox::VTaskID>	fThrIdSet;

private:
	class VChromeDebugContext;
#if VERSIONMAC
	typedef std::map< xbox::VString , v8::Persistent<v8::FunctionTemplate>* >	PersistentImplicitContructorMap_t;
	typedef std::map< xbox::VString, xbox::JS4D::ClassDefinition >				NativeClassesMap_t;
	typedef std::map< int, v8::Persistent<v8::Script>* >						ScriptsByIDMap_t;
	typedef std::map< xbox::VString, int >										ScriptIDsByURLMap_t;
#else
	typedef std::map< xbox::VString, v8::Persistent<v8::FunctionTemplate>* >	PersistentImplicitContructorMap_t;
	typedef std::map< xbox::VString, xbox::JS4D::ClassDefinition >				NativeClassesMap_t;
	typedef std::map< int, v8::Persistent<v8::Script>* >						ScriptsByIDMap_t;
	typedef std::map< xbox::VString, int >										ScriptIDsByURLMap_t;
#endif

			v8::Isolate* 																fIsolate;
			v8::Locker*																	fLocker;
			xbox::JS4D::ClassDefinition* 												fGlobalClassRef;
			v8::Persistent <v8::Context, v8::NonCopyablePersistentTraits<v8::Context> >	fPersistentCtx;

			PersistentImplicitContructorMap_t											fPersImplicitFunctionConstructorMap;
			std::vector<v8::Persistent<v8::FunctionTemplate>*>							fCallbacksVector;
			v8::Persistent<v8::Object>													fStringify;
			xbox::JS4D::ClassDefinition*												fDualCallClass;
			VChromeDebugContext*														fDebugContext;


			ScriptsByIDMap_t															fScriptsMap;
			ScriptIDsByURLMap_t															fScriptIDsMap;

			void						UpdateContext();
			void						AddNativeFunctionTemplate(	const xbox::VString& inName,
																	v8::Handle<v8::FunctionTemplate>& inFuncTemp);

	static	void						DestructorRevivable(const v8::WeakCallbackData<v8::Object, void>& inData);

	static	bool 						CheckIfNew(const XBOX::VString& inName);
	static	void						Print(const v8::FunctionCallbackInfo<v8::Value>& args);
			void						SetAccessors(v8::Handle<v8::ObjectTemplate>& inInstance, JS4D::ClassDefinition* inClassDef);
	static	void						GetPropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
	static	void						SetPropHandler(v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
	static	void						EnumPropHandler(const v8::PropertyCallbackInfo<v8::Array>& info);
	static	void						DeletePropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Boolean>& inInfo);
	static	void						QueryPropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Integer>& inInfo);
	static	void						IndexedGetPropHandler(uint32_t inIndex, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
	static	void						IndexedSetPropHandler(uint32_t inIndex, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
	static void							GetPropertyCallback(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
	static void							SetPropertyCallback(v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<void>& inInfo);

			VChromeDebugContext*		GetDebugContext();

	static	XBOX::VCriticalSection											sLock;

	static	NativeClassesMap_t												sNativeClassesMap;


};
#endif


END_TOOLBOX_NAMESPACE

#endif

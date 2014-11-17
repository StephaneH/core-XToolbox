#include "VJavaScriptPrecompiled.h"

#if USE_V8_ENGINE

#include "V4DContext.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include <v8-debug.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif


#include "VJSClass.h"
#include "VJSValue.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"

using namespace v8;
BEGIN_TOOLBOX_NAMESPACE



XBOX::VCriticalSection						V4DContext::sLock;
V4DContext::NativeClassesMap_t				V4DContext::sNativeClassesMap;



static VString	V8_FRAME_COUNT_STR("frameCount");
static VString	V8_FRAME_STR("frame");
static VString	V8_TOTEXT_STR("toText");
static VString	V8_SCOPE_STR("scope");
static VString	V8_SCOPE_COUNT_STR("scopeCount");
static VString	V8_SET_SELECTED_FRAME_STR("setSelectedFrame");
static VString	V8_SOURCE_LINE_STR("sourceLine");
static VString	V8_FRAME_ID_STR("frameId");
static VString	V8_LOCAL_NAME_STR("localName");
static VString	V8_ARGUMENT_COUNT_STR("argumentCount");


class V4DContext::VChromeDebugContext
{
public:
	VChromeDebugContext() : fDepth(0), fOpaqueDbgCtx((void*)-1), fAbort(false) { ; }
	~VChromeDebugContext();

	static void DebugMessageHandler2(const v8::Debug::Message& inMessage);
	void TreatDebuggerMessage(const v8::Debug::Message& inMessage, WAKDebuggerServerMessage* inDebuggerMessage, VString& outFrames);
	void LookupObject(
		const v8::Debug::Message&		inMessage,
		double							inObjectRef,
		VString&						outVars,
		int								inID);

	void HandleDebuggerServerEvents(
		const v8::Debug::Message&	inMessage,
		sLONG						inSrcLine,
		VString&					inSourceLineText,
		XBOX::VString&				inSourceURL,
		RemoteDebuggerPauseReason	inPauseReason,
		VString&					inExcTxt);

	void DescribeCallStack(const v8::Debug::Message& inMessage, VString& outFrames, CallstackDescriptionMap& outCSDescMap);
	void AddClosureScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj);
	void AddGlobalScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj);
	void AddLocalScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj);
	void valueToJSON(v8::Isolate*		inIsolate,
		Handle<Value>&		inValue,
		bool				inFull,
		VString&			outJSON);
	void DescribeScopes(
		v8::Isolate*								inIsolate,
		Handle<Object>&								inFrame,
		bool										inLocalOnly,
		uLONG8										inFrameId,
		VString&									outScopes);


	uLONG											fDepth;
	void*											fOpaqueDbgCtx;
	bool											fAbort; 


private:
	typedef enum ValueTypeEnum {
		UNKNOWN = 0,
		UNDEFINED,
		NULLVAL,
		NUMBER,
		BOOLEANVAL,
		STRING,
		OBJECT,
		ARRAY,
		FUNCTION
	} ValueType;

	void			AddPersistentObjectInMap(v8::Isolate* inIsolate, const VString& inID, Persistent<Object>* inObj);
	void			AddPersistentObjectInMap(v8::Isolate* inIsolate, const VString& inID, Handle<Object>& inObj);
	Handle<Object>	GetObjectFromObject(v8::Isolate* inIsolate, Handle<Object>& inObject, const VString& inName);
	int64_t			GetIntegerFromObject(v8::Isolate* inIsolate, Handle<Object>& inObject, const VString& inName);
	void			displayScope(uLONG8 inFrameIndex, int inScopeIndex, Handle<Value> inScope, unsigned int inShift = 1);
	void			displayValue(const Handle<Value>& inVal, const char* const inName, unsigned int inShift=1, bool inNameOnly=true);
	void			displayObject(Handle<Object> inObj, unsigned int inShift=1, bool inRecursive=false);

	ValueType		getType(Handle<Value> inVal);

	typedef std::map<const VString, v8::Persistent<Object>*> ScopesByIDMap_t;
	ScopesByIDMap_t									fScopesMap;
};




void V4DContext::GetPropertyCallback(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Value>& inInfo)
{
	String::Utf8Value	utf8Str(inProperty);
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	for (const JS4D::StaticValue* j = classDef->staticValues; j->name != NULL; ++j)
	{
		if (!strcmp(j->name,*utf8Str) && j->getProperty)
		{
			XBOX::VString	propName(*utf8Str);
			XBOX::VJSParms_getProperty	parms(propName, inInfo);
			j->getProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(),*inInfo.Holder()) );
		}
	}
}
void V4DContext::SetPropertyCallback(v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<void>& inInfo)
{
	String::Utf8Value	utf8Str(inProperty);
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	for (const JS4D::StaticValue* j = classDef->staticValues; j->name != NULL; ++j)
	{
		if (!strcmp(j->name, *utf8Str) && j->setProperty)
		{
			XBOX::VString	propName(*utf8Str);
			XBOX::VJSParms_setProperty	parms(propName, inValue, inInfo);
			
			j->setProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()) );
		}
	}
}


void V4DContext::SetAccessors(Handle<ObjectTemplate>& inInstance, JS4D::ClassDefinition* inClassDef)
{
	if (inClassDef->staticValues)
	{
		for (const JS4D::StaticValue *j = inClassDef->staticValues; j->name != NULL; ++j)
		{
			//DebugMsg("V4DContext::SetAccessors treat static value %s for %s\n", j->name, inClassDef->className);

			if (j->getProperty || j->setProperty)
			{
				Handle<External>			extPtr = External::New(fIsolate,inClassDef);
				inInstance->SetAccessor(
					v8::String::NewFromUtf8(fIsolate,j->name),
					GetPropertyCallback,
					SetPropertyCallback,
					extPtr,
					v8::DEFAULT,
					(v8::PropertyAttribute)(j->attributes) );
			}
		}
	}
}

void V4DContext::Enter()
{
#if V8_CHECK_CONTEXT_REUSE
	int sz = fThrIdStack.size();
	if (sz > 0)
	{
		xbox::VTaskID tmp = fThrIdStack.top();
		if (tmp != VTask::GetCurrent()->GetID())
		{
			int a = 1;
		}
	}
	fThrIdStack.push(VTask::GetCurrent()->GetID());
#endif
	if (testAssert(!fLocker))
	{
		fLocker = new v8::Locker(fIsolate);
	}
	/*xbox::VTaskID tmp = VTask::GetCurrent()->GetID();
	if (fThrIdSet.find(tmp) == fThrIdSet.end())
	{
		fThrIdSet.insert(tmp);
		DebugMsg("V4DContext::Enter() iso=%x entered by :");
		for (std::set<VTaskID>::iterator itThr = fThrIdSet.begin(); itThr != fThrIdSet.end(); itThr++)
		{
			DebugMsg(" %lld", *itThr);
		}
		DebugMsg("\n");
	}*/
	fIsolate->Enter();
}
void V4DContext::Exit()
{
#if V8_CHECK_CONTEXT_REUSE
	checkStack(this);
	fThrIdStack.pop();
#endif
	fIsolate->Exit();
	if (testAssert(fLocker))
	{
		delete fLocker;
		fLocker = NULL;
	}
	//fIsolate->Dispose();
}

#if V8_CHECK_CONTEXT_REUSE
static void checkStack(V4DContext* inCtx, bool inForDelete = false)
{
	xbox::VTaskID tmp = inCtx->fThrIdStack.top();
	if (!testAssert(tmp == VTask::GetCurrent()->GetID()))
	{
		for (;;)
			DebugMsg("V4DContext::checkStack pb  !!!!\n");
	}
	if (inForDelete and inCtx->fThrIdStack.size() > 1)
	{
		int sz = inCtx->fThrIdStack.size();
		for (;;)
			DebugMsg("V4DContext::checkStack SIZE pb  !!!!\n");
	}
}

void V4DContext::DisplayStack()
{
	DebugMsg("V4DContext::DisplayStack thrId=%lld \n", VTask::GetCurrent()->GetID());
	std::stack<xbox::VTaskID> tmp = fThrIdStack;
	while (!tmp.empty())
	{
		DebugMsg("    %lld\n", tmp.top());
		tmp.pop();
	}
}
#endif

V4DContext::~V4DContext()
{
#if V8_CHECK_CONTEXT_REUSE
checkStack(this,true);
fThrIdStack.pop();
#endif

	PersistentImplicitContructorMap_t::iterator	itPersFunc = fPersImplicitFunctionConstructorMap.begin();
	while (itPersFunc != fPersImplicitFunctionConstructorMap.end())
	{
		delete (*itPersFunc).second;
		itPersFunc++;
	}
	fPersImplicitFunctionConstructorMap.clear();

	ScriptsByIDMap_t::iterator	itScript = fScriptsMap.begin();
	while (itScript != fScriptsMap.end())
	{
		(*itScript).second->Reset();
		delete (*itScript).second;
		++itScript;
	}
	fScriptsMap.clear();
	fScriptIDsMap.clear();

	for (size_t idx = 0; idx < fCallbacksVector.size(); idx++)
	{
		delete fCallbacksVector[idx];
	}
	fCallbacksVector.clear();
	fStringify.Reset();
	fPersistentCtx.Reset();

	fIsolate->Exit();
	if (testAssert(fLocker))
	{
		delete fLocker;
		fLocker = NULL;
	}
}

bool V4DContext::HasDebuggerAttached()
{
	return (fDebugContext != NULL);
}

#define V8_ACTIVATE_DEBUGGER 1
void V4DContext::AttachDebugger()
{
#if V8_ACTIVATE_DEBUGGER
	xbox_assert(fDebugContext == NULL);
	fDebugContext = new VChromeDebugContext();
	fDebugContext->fOpaqueDbgCtx = VJSGlobalContext::sChromeDebuggerServer->AddContext(fIsolate);
	if (testAssert((sLONG8)fDebugContext->fOpaqueDbgCtx != -1))
	{
		fIsolate->SetBreakOnException();
		//v8::Debug::SetDebugMessageDispatchHandler(debugMessageDispatchHandler);
		v8::Debug::SetMessageHandler2(VChromeDebugContext::DebugMessageHandler2);
	}
	else
	{
		delete fDebugContext;
		fDebugContext = NULL;
	}
#endif
}


bool V4DContext::GetSourceID(const XBOX::VString& inUrl, int& outID)
{
	outID = -1;
	ScriptIDsByURLMap_t::const_iterator	itID = fScriptIDsMap.find(inUrl);
	if (itID != fScriptIDsMap.end())
	{
		outID = (*itID).second;
		return true;
	}
	return false;
}

void V4DContext::GetSourceData(sLONG inSourceID, XBOX::VString& outUrl)
{
	ScriptsByIDMap_t::const_iterator	itScript = fScriptsMap.find(inSourceID);
	if (itScript != fScriptsMap.end())
	{
		/*Handle<Script>	locScr = Handle<Script>::New(fIsolate, *(*itScript).second);
		Handle<Value>	scrName = locScr.val_->GetScriptName();
		Handle<String>	scrStr = scrName.val_->ToString();
		outUrl = VString(*scrStr);*/
		ScriptIDsByURLMap_t::const_iterator	itID = fScriptIDsMap.begin();
		while (itID != fScriptIDsMap.end())
		{
			if ((*itID).second == inSourceID)
			{
				outUrl = (*itID).first;
				break;
			}
			++itID;
		}
	}
}

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator
{
public:
	ArrayBufferAllocator() {};

	void* Allocate(size_t length) {
		void* tmp = calloc(length,1);
		//DebugMsg("ArrayBufferAllocator::Allocate ptr=%x\n", tmp);
		return tmp;
	}
	void* AllocateUninitialized(size_t length) { return (void*)malloc(length); }

	void Free(void* data, size_t length) {
		//DebugMsg("ArrayBufferAllocator::Free ptr=%x\n", data);
		(void)length;
		free(data);
	}
};

static ArrayBufferAllocator sArrayBufferAllocator;

V4DContext::V4DContext(v8::Isolate* isolate, JS4D::ClassRef inGlobalClassRef)
: fIsolate(isolate)
, fGlobalClassRef((JS4D::ClassDefinition*)inGlobalClassRef)
, fDualCallClass(NULL)
, fDebugContext(NULL)
{
	static bool sIsV8Initialized = false;
	if (!sIsV8Initialized)
	{
		V8::SetArrayBufferAllocator(&sArrayBufferAllocator);
		sIsV8Initialized = true;
	}
	//XBOX::VPtr	stackAddr = VTask::GetCurrent()->GetStackAddress();
	//size_t		stackSize = VTask::GetCurrent()->GetStackSize();
#if V8_CHECK_CONTEXT_REUSE
fThrIdStack.push(VTask::GetCurrent()->GetID());
#endif
	//fThrIdSet.insert(VTask::GetCurrent()->GetID());

	fLocker = new v8::Locker(fIsolate);
	fIsolate->Enter();
	fIsolate->SetData(0,this);
	xbox_assert(fIsolate == Isolate::GetCurrent());
	HandleScope handle_scope(fIsolate);
	Handle<Context> initCtx = Context::New(fIsolate,NULL);
	fPersistentCtx.Reset(fIsolate,initCtx);

	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(context);
	Handle<Object> global = context->Global();
	bool	isFound;
	Handle<FunctionTemplate> 	blobFuncTemplate = FunctionTemplate::New(fIsolate);
	if (fGlobalClassRef->staticFunctions)
	{
		for( const JS4D::StaticFunction *i = fGlobalClassRef->staticFunctions ; i->name != NULL ; ++i)
		{
			//DebugMsg("V4DContext::V4DContext treat static function %s for %s\n",i->name,fGlobalClassRef->className);
			Handle<FunctionTemplate> ft = FunctionTemplate::New(fIsolate,i->callAsFunction);
			Handle<Function> newFunc = ft->GetFunction();
			global->Set(String::NewFromUtf8(fIsolate,i->name), newFunc);
		}
	}
	if (fGlobalClassRef->staticValues)
	{
		for (const JS4D::StaticValue *sv = fGlobalClassRef->staticValues; sv->name != NULL; ++sv)
		{
			//DebugMsg("V4DContext::V4DContext treat static value1 %s for %s\n", sv->name, fGlobalClassRef->className);

			if (sv->getProperty || sv->setProperty)
			{
				if (!global->Has(v8::String::NewFromUtf8(fIsolate,sv->name)))
				{
					Handle<External>			extPtr = External::New(fIsolate,fGlobalClassRef);
					global->SetAccessor(v8::String::NewFromUtf8(fIsolate,sv->name),
						GetPropertyCallback,
						SetPropertyCallback,
						extPtr,
						v8::DEFAULT,
						(v8::PropertyAttribute)(sv->attributes));
				}
				else
				{
					DebugMsg("V4DContext::V4DContext global obj already has static value1 %s for %s\n", sv->name, fGlobalClassRef->className);
				}
			}
		}
	}

	Handle<Value>	jsonVal = global->Get(v8::String::NewFromUtf8(fIsolate,"JSON"));
	if (testAssert(jsonVal->IsObject()))
	{
		Handle<Value>	stringVal = jsonVal->ToObject()->Get(v8::String::NewFromUtf8(fIsolate,"stringify"));
		if (testAssert(stringVal->IsObject()))
		{
			Handle<Object>	stringObj = stringVal->ToObject();
			if (stringObj->IsCallable())
			{
				fStringify.Reset(fIsolate,v8::Persistent<v8::Object>(fIsolate, stringObj));
			}
		}
	}
	fPersistentCtx.Reset(fIsolate, context);

}

V4DContext::VChromeDebugContext* V4DContext::GetDebugContext()
{
	return fDebugContext;
}

void V4DContext::CollectGarbage()
{
	/*static uint64_t count = 0;
	count++;
	if (count % 10 == 0)
	{
	intptr_t memAmount = fIsolate->AdjustAmountOfExternalAllocatedMemory(0);
	DebugMsg("V4DContext::CollectGarbage memAmount=%d\n", memAmount);
	}*/
	if (testAssert(fIsolate == v8::Isolate::GetCurrent()))
	{
		fIsolate->CollectGarbage();
	}
}

Handle<Object> V4DContext::GetFunction(const char* inClassName)
{
	EscapableHandleScope		handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();

	Handle<Value>		funcVal = global->Get(String::NewFromUtf8(fIsolate,inClassName));
	Local<Object>		result = Local<Object>();
	if (funcVal->IsFunction())
	{
		result = funcVal->ToObject();
	}
	return handle_scope.Escape(result);
}


void V4DContext::IndexedGetPropHandler(uint32_t inIndex, const PropertyCallbackInfo<Value>& inInfo)
{
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();

	XBOX::DebugMsg("IndexedGetPropHandler called for %s[%d]\n", classDef->className,inIndex);
	if (strcmp(classDef->className,"EntityCollection")) // see if other classes use indexed props
	{
		int a = 1;
	}
	if (testAssert(classDef->getProperty))
	{
		VString		numericPropName;
		numericPropName.AppendLong(inIndex);
		XBOX::VJSParms_getProperty	parms(numericPropName, inInfo);
		/*if (inIndex == 11)
		{
			int a = 1;
		}*/
		classDef->getProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
	}

}

void V4DContext::IndexedSetPropHandler(uint32_t inIndex, Local<Value> inValue, const PropertyCallbackInfo<Value>& inInfo)
{
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();

	XBOX::DebugMsg("IndexedSetPropHandler called for %s[%d]\n", classDef->className, inIndex);
	if (testAssert(classDef->setProperty))
	{
		VString		numericPropName;
		numericPropName.AppendLong(inIndex);
		/*if (inIndex == 11)
		{
			int a = 1;
		}*/
		XBOX::VJSParms_setProperty	parms(numericPropName, inValue, inInfo);
		classDef->setProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
	}

}

void V4DContext::GetPropHandler(Local<String> inProperty, const PropertyCallbackInfo<Value>& inInfo)
{
	bool	isFound = false;
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	String::Utf8Value	utf8Str(inProperty);

	//XBOX::DebugMsg("GetPropHandler called for '%s'\n", *utf8Str);
	if (!strcmp(*utf8Str, "Storage"))
	{
		int a = 1;
	}

	std::vector<XBOX::VString>	propNames;

	//if (classDef->getPropertyNames)
	if (classDef->staticFunctions)
	{
		//classDef->getPropertyNames(inInfo.GetIsolate(), inInfo.Holder().val_, propNames);
		//for (int idx = 0; idx < propNames.size(); idx++)
		for (const JS4D::StaticFunction *j = classDef->staticFunctions; j->name != NULL; ++j)
		{
			if (!strcmp(*utf8Str, "getItem"))
			{
				int a = 1;
			}
			//if (propNames[idx] == VString(*utf8Str))
			if (!strcmp(j->name,*utf8Str))
			{
				isFound = true;
				return;
			}
		}
	}
	//if (isFound)
	{
		if (classDef->getProperty)
		{
			XBOX::VString	propName(*utf8Str);
			if (propName.Find("testKey4"))
			{
				int a = 1;
			}
			XBOX::VJSParms_getProperty	parms(propName, inInfo);
			bool isSameObj = (*inInfo.Holder() == *inInfo.This());
			if (!isSameObj)
			{
				int a = 1;
			}
				/*if (!strcmp(classDef->className, "dataClass"))
			{
				displayProps(inInfo.Holder().val_);
			}*/
			classDef->getProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
			/*if (propName == "ID")
			{
				const VJSValue tmpVal = parms.GetReturnValue();
				VJSObject tmpObj = tmpVal.GetObject();
				v8::Value*	valPtr = tmpObj.fObject;
				if (valPtr->IsObject())
				{
					Handle<Object>	locObj = valPtr->ToObject();
					displayProps(locObj.val_);
				}
			}*/
		}
	}
}

void V4DContext::SetPropHandler(Local<String> inProperty, Local<Value> inValue, const PropertyCallbackInfo<Value>& inInfo)
{
	bool	isFound = false;
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	if (!strcmp(classDef->className, "Storage"))
	{
		int a = 1;
	}
	String::Utf8Value	utf8Str(inProperty);
	//XBOX::DebugMsg("SetPropHandler called for '%s'\n", *utf8Str);
	std::vector<XBOX::VString>	propNames;
	/*if (!strcmp(*utf8Str, "Content-Type"))
	{
		int a = 1;
	}*/
	if (classDef->staticFunctions)
	{
		for (const JS4D::StaticFunction *j = classDef->staticFunctions; j->name != NULL; ++j)
		{
			if (!strcmp(j->name, *utf8Str))
			{
				return;
			}
		}
	}
	if (classDef->setProperty)
	{
		/*classDef->getPropertyNames(inInfo.GetIsolate(), inInfo.Holder().val_, propNames);
		for (int idx = 0; idx < propNames.size(); idx++)
		{
			if (propNames[idx] == VString(*utf8Str))
			{
				isFound = true;
			}
		}
		if (isFound)*/
		{
			v8::AccessorSetterCallback accSetterCbk = NULL;
			if (classDef->setProperty)
			{
				XBOX::VString	propName(*utf8Str);
				if (propName.Find("testKey4"))
				{
					int a = 1;
				}
				XBOX::VJSParms_setProperty	parms(propName, inValue, inInfo);
				classDef->setProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
			}
		}
	}
}

void V4DContext::DeletePropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Boolean>& inInfo)
{
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	String::Utf8Value	utf8Str(inProperty);
	if (classDef->deleteProperty)
	{
		XBOX::VString	propName(*utf8Str);
		if (propName.Find("testKey4"))
		{
			int a = 1;
		}
		XBOX::VJSParms_deleteProperty	parms(propName, inInfo);
		bool isOK = classDef->deleteProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
		if (isOK)
		{
			bool isDel = isOK;
		}
		inInfo.GetReturnValue().Set(isOK);
	}
}

void V4DContext::EnumPropHandler(const PropertyCallbackInfo<Array>& info)
{
	Handle<External> extHdl = Handle<External>::Cast(info.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	std::vector<XBOX::VString>	propNames;
	if (!strcmp(classDef->className, "Storage"))
	{
		int a = 1;
	}
	if (classDef->getPropertyNames)
	{
		classDef->getPropertyNames(info, propNames);
		Local<Array>	locArray = v8::Array::New(info.GetIsolate(),propNames.size());
		for (size_t idx = 0; idx < propNames.size(); idx++)
		{
			XBOX::DebugMsg("EnumPropHandler found %S for '%s'\n", &propNames[idx], classDef->className);
			Handle<String>	tmpStr = v8::String::NewFromTwoByte(info.GetIsolate(), propNames[idx].GetCPointer(), v8::String::kNormalString, propNames[idx].GetLength());
			locArray->Set(idx, tmpStr);
		}
		info.GetReturnValue().Set(locArray);
	}
}
void V4DContext::QueryPropHandler(Local<String> inProperty, const PropertyCallbackInfo<Integer>& inInfo)
{
	Handle<External> extHdl = Handle<External>::Cast(inInfo.Data());
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)extHdl->Value();
	if (!strcmp(classDef->className, "Storage"))
	{
		int a = 1;
	}
	String::Utf8Value	utf8Str(inProperty);
	if (classDef->hasProperty)
	{
		XBOX::VString	propName(*utf8Str);
		if (propName.Find("testKey4"))
		{
			int a = 1;
		}
		XBOX::VJSParms_hasProperty	parms(propName, inInfo);
		classDef->hasProperty(&parms, V4DContext::GetObjectPrivateData(inInfo.GetIsolate(), *inInfo.Holder()));
		// if non-existent param leave returnValue empty
		if (!parms.GetReturnedBoolean())
		{
			if (propName.Find("testKey4"))
			{
				inInfo.GetReturnValue().Set(v8::DontEnum);
			}
			return;
		}
	}
	inInfo.GetReturnValue().Set(v8::ReadOnly);
}

Handle<Function> V4DContext::CreateFunctionTemplateForClassConstructor(
						JS4D::ClassRef inClassRef,
						const JS4D::ObjectCallAsFunctionCallback inConstructorCallback,
						void* inData,
						bool inAcceptCallAsFunction,
						JS4D::StaticFunction inContructorFunctions[] )
{
	EscapableHandleScope		handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	if (!strcmp(classDef->className, "EntityCollection"))
	{
		int a = 1;
	}
	//if (testAssert(!global->Has(v8::String::NewSymbol(classDef->className))))
	{
		Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New(fIsolate);
		//if (inLockResources)
		sLock.Lock();

		JS4D::ClassDefinition*	parentClassDef = (JS4D::ClassDefinition*)classDef->parentClass;
    	Handle<ObjectTemplate> 	instance = classFuncTemplate->InstanceTemplate();

		classFuncTemplate->SetClassName(String::NewFromUtf8(fIsolate,classDef->className));
		instance->SetInternalFieldCount(0);

		if (classDef->staticFunctions)
		{
			for( const JS4D::StaticFunction *j = classDef->staticFunctions ; j->name != NULL ; ++j)
			{
				//DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor treat static function %s for %s\n",j->name,classDef->className);
					
				Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(fIsolate, j->callAsFunction, v8::Undefined(fIsolate));
				functionTemplate->RemovePrototype();
				classFuncTemplate->PrototypeTemplate()->Set(
					v8::String::NewFromUtf8(fIsolate,j->name),
					functionTemplate,
					(v8::PropertyAttribute)(j->attributes));
			}
		}
		SetAccessors(instance, classDef);

		if (classDef->getProperty || classDef->setProperty || classDef->deleteProperty)
		{
			Handle<External> extPtr = External::New(fIsolate,(void*)classDef);
			instance->SetNamedPropertyHandler(
				(classDef->getProperty ? GetPropHandler : NULL),
				(classDef->setProperty ? SetPropHandler : NULL ),
				(classDef->hasProperty ? QueryPropHandler : NULL),
				(classDef->deleteProperty ? DeletePropHandler : NULL),
				(classDef->getPropertyNames ? EnumPropHandler : NULL),
				extPtr);
		}
		if (classDef->getProperty || classDef->setProperty || classDef->deleteProperty)
		{
			Handle<External> extPtr = External::New(fIsolate,(void*)classDef);
			instance->SetIndexedPropertyHandler(
				(classDef->getProperty ? IndexedGetPropHandler : NULL),
				(classDef->setProperty ? IndexedSetPropHandler : NULL),
				NULL, NULL, NULL, extPtr);
		}

		/*DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' parent= '%s'\n",
						classDef->className,
						(parentClassDef	? parentClassDef->className : "None"));*/

		if (parentClassDef)
		{
			//if (testAssert(!strcmp(parentClassDef->className,"Blob")))
			{
				/*DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' needs parent '%s'\n",
					 classDef->className,
					 parentClassDef->className );*/
				PersistentImplicitContructorMap_t::iterator itIFT =
						fPersImplicitFunctionConstructorMap.find(parentClassDef->className);
				if (testAssert(itIFT != fPersImplicitFunctionConstructorMap.end()))
				{
					Handle<FunctionTemplate>	parentFT = Handle<FunctionTemplate>::New(fIsolate, *((*itIFT).second));
					classFuncTemplate->Inherit(parentFT);
				}
				else
				{
					DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' needs NOT FOUND parent '%s !!!!!!'\n",
						classDef->className,
						parentClassDef->className);
				}
				v8::Local<v8::ObjectTemplate> prototype = classFuncTemplate->PrototypeTemplate();
				prototype->SetInternalFieldCount(0);
			}

		}

		classFuncTemplate->SetLength(0);
		//DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor treat static constructor %s\n",classDef->className);


		Handle<External> extPtr = External::New(fIsolate,inData);

		if (!strcmp("require", classDef->className))
		{
			//should not pass here
			int a = 1;
		}
		classFuncTemplate->SetCallHandler(inConstructorCallback,extPtr);
		
		if (!testAssert(!classDef->callAsFunction))
		{
			int a = 1;
		}
		Local<Function> localFunction = classFuncTemplate->GetFunction();
		if (inContructorFunctions)
		{
			for (JS4D::StaticFunction* tmpFunc = inContructorFunctions; tmpFunc->name; tmpFunc++)
			{
				/*DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor adding %s method in %s constructor\n",
					tmpFunc->name,
					classDef->className);*/
				Handle<FunctionTemplate>	newFuncTempl = v8::FunctionTemplate::New(fIsolate,tmpFunc->callAsFunction);
				localFunction->Set(
					v8::String::NewFromUtf8(fIsolate,tmpFunc->name),
					newFuncTempl->GetFunction(),
					(v8::PropertyAttribute)tmpFunc->attributes);
			}
		}
		//if (inAddPersistentConstructor)
		{
			AddNativeFunctionTemplate(classDef->className,classFuncTemplate);
		}

		sLock.Unlock();
		
		return handle_scope.Escape(localFunction);
	}
	/*else
	{
		Handle<Function> localFunction = Handle<Function>::New(fIsolate,NULL);
		return handle_scope.Close(localFunction);
	}*/
}


void V4DContext::DualCallFunctionHandler(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	if (testAssert(info.Data()->IsExternal()))
	{
		Handle<External> extHdl = Handle<External>::Cast(info.Data());
		JS4D::ClassDefinition* dualCallClass = (JS4D::ClassDefinition*)extHdl->Value();
		if (info.IsConstructCall())
		{
			dualCallClass->callAsConstructor(info);
		}
		else
		{
			dualCallClass->callAsFunction(info);
		}
	}
}


Handle<Function> V4DContext::CreateFunctionTemplateForClass(JS4D::ClassRef	inClassRef,
															bool			inLockResources,
															bool			inAddPersistentConstructor )
{
	EscapableHandleScope		handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	
	//DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat class %s \n",classDef->className);
	if (!strcmp(classDef->className, "EntityCollection"))
	{
		int a = 1;
	}
	//if (!global->Has(v8::String::NewSymbol(classDef->className)))  -> sometime use adds a function on globalObj that has the same name as a class so do not check global->Has
	{
		Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New(fIsolate);
		if (inLockResources)
			sLock.Lock();

		JS4D::ClassDefinition*	parentClassDef = (JS4D::ClassDefinition*)classDef->parentClass;
		/*DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat class %s parent=%s\n",
						classDef->className,
						(parentClassDef	? parentClassDef->className : "None"));*/
    	Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
		
		classFuncTemplate->SetClassName(String::NewFromUtf8(fIsolate,classDef->className));
		instance->SetInternalFieldCount(0);

		if (classDef->staticFunctions)
		{
			for( const JS4D::StaticFunction *j = classDef->staticFunctions ; j->name != NULL ; ++j)
			{
				//DebugMsg("V4DContext::CreateFunctionTemplateForClass treat static function %s for %s\n",j->name,classDef->className);
					
				Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(fIsolate,j->callAsFunction, v8::Undefined(fIsolate));
				functionTemplate->RemovePrototype();
				classFuncTemplate->PrototypeTemplate()->Set(
					v8::String::NewFromUtf8(fIsolate,j->name),
					functionTemplate,
					(v8::PropertyAttribute)(j->attributes) );
			}
		}

		SetAccessors(instance, classDef);

		if (classDef->getProperty || classDef->setProperty || classDef->deleteProperty)
		{
			Handle<External> extPtr = External::New(fIsolate,(void*)classDef);
			instance->SetNamedPropertyHandler(
				(classDef->getProperty ? GetPropHandler : NULL),
				(classDef->setProperty ? SetPropHandler : NULL),
				(classDef->hasProperty ? QueryPropHandler : NULL),
				(classDef->hasProperty ? DeletePropHandler : NULL),
				(classDef->getPropertyNames ? EnumPropHandler : NULL),
				extPtr);
		}
		if (classDef->getProperty || classDef->setProperty)
		{
			Handle<External> extPtr = External::New(fIsolate,(void*)classDef);
			instance->SetIndexedPropertyHandler(
				(classDef->getProperty ? IndexedGetPropHandler : NULL),
				(classDef->setProperty ? IndexedSetPropHandler : NULL), NULL, NULL, NULL, extPtr);
		}
		if (parentClassDef)
		{
			PersistentImplicitContructorMap_t::iterator	itIFT =
				fPersImplicitFunctionConstructorMap.find(parentClassDef->className);
			if (testAssert(itIFT != fPersImplicitFunctionConstructorMap.end()))
			{
				Handle<FunctionTemplate>	parentFT = Handle<FunctionTemplate>::New(fIsolate, *((*itIFT).second));
				classFuncTemplate->Inherit(parentFT);
			}
			else
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClass class='%s' has parent='%s' NOT FOUND\n",
					classDef->className,
					parentClassDef->className);
			}
		}

		classFuncTemplate->SetLength(0);
		
		if (classDef->callAsFunction)
		{
			if (classDef->callAsConstructor)
			{
				// for the moment only dataClass shoudl have both callAs* defined
				if (testAssert(strcmp(classDef->className, "dataClass") == 0))
				{
					if (testAssert(!fDualCallClass))
					{
						fDualCallClass = classDef;
						Handle<External> extPtr = External::New(fIsolate,(void*)fDualCallClass);
						instance->SetCallAsFunctionHandler(DualCallFunctionHandler,extPtr);
					}
				}
				else
				{
					DebugMsg("V4DContext::CreateFunctionTemplateForClass class='%s' unauthorized definition\n", classDef->className);
				}
			}
			else
			{
				if (!strcmp(classDef->className, "require"))
				{
					int a = 1;
				}
				instance->SetCallAsFunctionHandler(classDef->callAsFunction);
			}
		}
		else
		{
			int a = 1;
		}

		Local<Function> localFunction = classFuncTemplate->GetFunction();

		if (inAddPersistentConstructor)
		{
			AddNativeFunctionTemplate(classDef->className,classFuncTemplate);
		}
		if (inLockResources)
			sLock.Unlock();
		
		return handle_scope.Escape(localFunction);
	}

}

Persistent<FunctionTemplate>* V4DContext::AddCallback(JS4D::ObjectCallAsFunctionCallback inFunctionCallback)
{
	EscapableHandleScope		handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate, fPersistentCtx);
	Context::Scope	context_scope(context);
	Handle<FunctionTemplate> 		funcTemplate = FunctionTemplate::New(fIsolate);
	funcTemplate->SetCallHandler(inFunctionCallback);
	Persistent<FunctionTemplate>*	newCbk = new Persistent<FunctionTemplate>();
	newCbk->Reset(fIsolate,funcTemplate);

	fPersistentCtx.Reset(fIsolate, context);
	sLock.Lock();
	fCallbacksVector.push_back(newCbk);
	sLock.Unlock();
	return newCbk;
}

void V4DContext::AddNativeFunctionTemplate(	const xbox::VString& inName,
											Handle<FunctionTemplate>& inFuncTemp )
{
	//DebugMsg("V4DContext::AddNativeFunctionTemplate called for '%S' \n",&inName);
	PersistentImplicitContructorMap_t::iterator	itIFT =	fPersImplicitFunctionConstructorMap.find(inName);
	if (inName == "Server")
	{
		int a = 1;
	}
	if (testAssert(itIFT == fPersImplicitFunctionConstructorMap.end()))
	{
		VStringConvertBuffer	bufferTmp(inName,VTC_UTF_8);
		HandleScope handle_scope(fIsolate);
		Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
		Context::Scope context_scope(context);
		Persistent<FunctionTemplate>*	newPersFT = new Persistent<FunctionTemplate>();
		std::pair<VString , Persistent<FunctionTemplate>* > newElt(inName,newPersFT );
		newElt.second->Reset(fIsolate,inFuncTemp);
		fPersImplicitFunctionConstructorMap.insert(newElt);
		/*Handle<Object> global = context->Global();
		Local<Function> localFunction = inFuncTemp->GetFunction();
		global->Set(String::New(bufferTmp.GetCPointer()),localFunction);*/
		fPersistentCtx.Reset(fIsolate,context);
	}
}

v8::Persistent<v8::FunctionTemplate>*	V4DContext::GetImplicitFunctionConstructor(const xbox::VString& inName)
{
	Persistent<FunctionTemplate>*	result = NULL;
	sLock.Lock();
	if (inName == "dataClass")
	{
		int a = 1;
	}
	PersistentImplicitContructorMap_t::iterator	itIFT = fPersImplicitFunctionConstructorMap.find(inName);
	if (itIFT != fPersImplicitFunctionConstructorMap.end() )
	{
		result = (*itIFT).second;
	}
	else
	{
		NativeClassesMap_t::iterator	itClass = sNativeClassesMap.find(inName);
		if (itClass != sNativeClassesMap.end())
		{
			HandleScope handle_scope(fIsolate);
			Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
			Handle<Object> global = context->Global();

			JS4D::ClassDefinition* clRef = (JS4D::ClassDefinition*)&((*itClass).second);
			Handle<Object> newFunc = V4DContext::CreateFunctionTemplateForClass( clRef, false, true );
			//global->Set(String::NewSymbol(clRef->className),newFunc);
			fPersistentCtx.Reset(fIsolate,context);
			itIFT = fPersImplicitFunctionConstructorMap.find(inName);
			if (itIFT != fPersImplicitFunctionConstructorMap.end() )
			{
				result = (*itIFT).second;
			}
			else
			{
				DebugMsg("V4DContext::GetImplicitFunctionConstructor %S NOT FOUND2\n", &inName);
			}
		}
		else
		{
			DebugMsg("V4DContext::GetImplicitFunctionConstructor %S NOT FOUND\n",&inName);
		}
	}
	sLock.Unlock();
	return result;	
}

class WakandaPrivateData
{
public:
	WakandaPrivateData() : fData(NULL), fFinalizeCallback(NULL) { ; }
	WakandaPrivateData(void* inPrivateData, void* inCbk) : fData(inPrivateData), fFinalizeCallback(inCbk) { ; }
	void*						fData;
	void*						fFinalizeCallback;
	v8::Persistent<v8::Object>	fGCCallbackObj;
};

void* V4DContext::GetGlobalObjectPrivateInstance()
{
	WakandaPrivateData*	prvData = NULL;
	if (!testAssert(fIsolate == v8::Isolate::GetCurrent()))
	{
		DebugMsg("V4DContext::GetGlobalObjectPrivateInstance bad ctx=%x / %x  taskID=%d\n",
			fIsolate,
			v8::Isolate::GetCurrent(),
			VTask::GetCurrent()->GetID());
	}
	else
	{
		/*DebugMsg("V4DContext::GetGlobalObjectPrivateInstance OK ctx=%x / %x  taskID=%d\n",
			fIsolate,
			v8::Isolate::GetCurrent(),
			VTask::GetCurrent()->GetID());*/
	}
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object> global = context->Global();

	Local<Value>	extVal = global->GetHiddenValue(v8::String::NewFromUtf8(fIsolate,K_WAK_PRIVATE_DATA_ID));
	if (extVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(extVal);
		prvData = (WakandaPrivateData*)extHdl->Value();
	}
	return prvData->fData;
}

#define K_SIMULATED_DATA_SIZE	(512)

void V4DContext::DestructorRevivable(const WeakCallbackData<v8::Object, void>& inData)
{
	//DebugMsg("V4DContext::DestructorRevivable\n");
	XBOX::VJSParms_finalize	prms(NULL);
	WakandaPrivateData*	tmp = (WakandaPrivateData*)inData.GetParameter();
	tmp->fGCCallbackObj.Reset();
	if (tmp->fFinalizeCallback && tmp->fData)
	{
		((JS4D::ObjectFinalizeCallback)tmp->fFinalizeCallback)(tmp->fData);
	}
	else
	{
		if (tmp->fData)
		{
			int a = 1;
		}
	}
	delete tmp;
	intptr_t memAmount = inData.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(-K_SIMULATED_DATA_SIZE);
}

void V4DContext::SetGlobalObjectPrivateInstance(void* inPrivateData)
{
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object> global = context->Global();
	WakandaPrivateData*		wakData = new WakandaPrivateData(inPrivateData,NULL);
	wakData->fGCCallbackObj.Reset(fIsolate, global);
	wakData->fGCCallbackObj.SetWeak<v8::Object, void>((void*)wakData, DestructorRevivable);
	intptr_t memAmount = fIsolate->AdjustAmountOfExternalAllocatedMemory(K_SIMULATED_DATA_SIZE);
	Handle<External> extPtr = External::New(fIsolate,(void*)wakData);
	global->SetHiddenValue(String::NewFromUtf8(fIsolate,K_WAK_PRIVATE_DATA_ID),extPtr);
	fPersistentCtx.Reset(fIsolate,context);
}

void V4DContext::DisposeGloBalObjectPrivateInstance(JS4D::ClassDefinition inClassDef)
{
	WakandaPrivateData*	prvData = NULL;
	if (!testAssert(fIsolate == v8::Isolate::GetCurrent()))
	{
		DebugMsg("V4DContext::DisposeGloBalObjectPrivateInstance bad ctx=%x / %x  taskID=%d\n",
			fIsolate,
			v8::Isolate::GetCurrent(),
			VTask::GetCurrent()->GetID());
	}
	else
	{
		/*DebugMsg("V4DContext::DisposeGloBalObjectPrivateInstance OK ctx=%x / %x  taskID=%d\n",
		fIsolate,
		v8::Isolate::GetCurrent(),
		VTask::GetCurrent()->GetID());*/
	}
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate, fPersistentCtx);
	Handle<Object> global = context->Global();

	Local<Value>	extVal = global->GetHiddenValue(v8::String::NewFromUtf8(fIsolate, K_WAK_PRIVATE_DATA_ID));
	if (extVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(extVal);
		prvData = (WakandaPrivateData*)extHdl->Value();
		inClassDef.finalize(prvData->fData);
	}

	global->SetHiddenValue(v8::String::NewFromUtf8(fIsolate, K_WAK_PRIVATE_DATA_ID),v8::Undefined(fIsolate));
}

bool V4DContext::CheckScriptSyntax(const XBOX::VString& inScript, const VString& inUrl)
{
	bool	ok=false;
	DebugMsg("V4DContext::CheckScriptSyntax <%S> ....\n",&inUrl);

	HandleScope handle_scope(fIsolate);
	Local<Context> locCtx = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(locCtx);
	Handle<String> scriptStr =
		String::NewFromTwoByte(fIsolate, inScript.GetCPointer(), v8::String::kNormalString, inScript.GetLength());
	TryCatch try_catch;
	Handle<Script> compiled_script = Script::Compile(scriptStr);
	if (compiled_script.IsEmpty())
	{
		String::Utf8Value error(try_catch.Exception());
		DebugMsg("V4DContext::CheckScriptSyntax error -> '%s'  on script '%S'\n",*error,&inUrl);
	}
	else
	{
		ok = true;
	}
	return ok;
}

bool V4DContext::EvaluateScript(const VString& inScript, const VString& inUrl, VJSValue *outResult, bool inCacheScript, VJSException* outException)
{
	bool	ok=false;
	DebugMsg("V4DContext::EvaluateScript <%S> ....\n",&inUrl);
	//DebugMsg("V4DContext::EvaluateScript <%S> ....\n",&inScript);
#if V8_ACTIVATE_DEBUGGER
	if (!inCacheScript)
	{
		int a = 1;
	}
	if (fDebugContext)
	{
		testAssert(!fDebugContext->fAbort);
		fDebugContext->fDepth++;
	}
#endif
	if (inUrl.Find("unitTest.js"))
	{
		int a = 1;
	}

	{
		// do not enableAgent -> does crash when connection opened and chrome does not use it
		//v8::Debug::EnableAgent("WAKSrv", 5959, false);
	}
	HandleScope handle_scope(fIsolate);
	Local<Context> locCtx = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(locCtx);

	if (inScript.Find("try") && inScript.Find("catch"))
	{
		//DebugMsg("V4DContext::EvaluateScript  found TryCatch in \n \n<%S>\n\n", &inScript);
	}
	TryCatch try_catch;
	try_catch.SetVerbose(true);

	Handle<Script> compiledScript;

	if (inUrl.Find(":///") != 1 && inUrl.Find(K_EMPTY_URL) != 1)
	{
		ScriptIDsByURLMap_t::iterator	itID = fScriptIDsMap.find(inUrl);
		if (inCacheScript && (itID != fScriptIDsMap.end()))
		{
			ScriptsByIDMap_t::iterator	itScript = fScriptsMap.find((*itID).second);
			if (testAssert(itScript != fScriptsMap.end()))
			{
				compiledScript = Handle<Script>::New(fIsolate, *(*itScript).second);
			}
		}
		else
		{
			Handle<String> scriptStr = String::NewFromTwoByte(fIsolate, inScript.GetCPointer(), v8::String::kNormalString, inScript.GetLength());
			Handle<String> fileStr = String::NewFromTwoByte(fIsolate, inUrl.GetCPointer(), v8::String::kNormalString, inUrl.GetLength());
			// here either we do not cache or the script his a new one
			compiledScript = Script::Compile(scriptStr, fileStr);
			if (!compiledScript.IsEmpty())
			{
				// if old script
				if (itID != fScriptIDsMap.end() && testAssert(!inCacheScript))
				{
					ScriptsByIDMap_t::iterator	itScript = fScriptsMap.find((*itID).second);
					if (testAssert(itScript != fScriptsMap.end()))
					{
						itScript->second->Reset();
						delete itScript->second;
						fScriptsMap.erase(itScript);
					}
					fScriptIDsMap.erase(itID);
				}
				fScriptIDsMap.insert(std::pair<VString, int>(inUrl, compiledScript->GetId()));
				fScriptsMap.insert(std::pair<int, Persistent<Script>*>(compiledScript->GetId(), new Persistent<Script>(fIsolate, compiledScript)));
				if (VJSGlobalContext::sChromeDebuggerServer)
				{
#if V8_ACTIVATE_DEBUGGER
					if (fDebugContext && ((sLONG8)fDebugContext->fOpaqueDbgCtx != -1))
					{
						std::set< unsigned >	breakpointsLineNbs;
						VJSGlobalContext::sChromeDebuggerServer->SendSource(
							fDebugContext->fOpaqueDbgCtx,
							(intptr_t)compiledScript->GetId(),
							inUrl,
							inScript,
							breakpointsLineNbs);
						std::set< unsigned >::iterator	itBkpts = breakpointsLineNbs.begin();
						while (itBkpts != breakpointsLineNbs.end())
						{
							static unsigned int gh_v8_break_point = 0;
							//Handle<Object>	bkptID = v8::NumberObject::New(++gh_v8_break_point)->ToObject();
							int	srcPos = (*itBkpts);
							fIsolate->SetBreakPointForScript(compiledScript, ++gh_v8_break_point, &srcPos);
							++itBkpts;
						}
					}
#endif
				}
			}
		}
	}
	else
	{
		Handle<String> scriptStr = String::NewFromTwoByte(fIsolate, inScript.GetCPointer(), v8::String::kNormalString, inScript.GetLength());
		Handle<String> fileStr = String::NewFromTwoByte(fIsolate, inUrl.GetCPointer(), v8::String::kNormalString, inUrl.GetLength());
		compiledScript = Script::Compile(scriptStr, fileStr);
	}

	if (compiledScript.IsEmpty())
	{
		if (try_catch.HasCaught())
		{
			if (outException)
			{
				outException->SetValue(fIsolate,try_catch.Exception());
			}
			try_catch.ReThrow();
		}
		String::Utf8Value error(try_catch.Exception());
		DebugMsg("V4DContext::EvaluateScript error -> '%s'  on script '%S'\n",*error,&inUrl);
	}
	else
	{
		if (inUrl.Find("testMethod"))
		{
			int srcLine = 20;
			//fIsolate->SetBreakPointForScript(compiled_script, &srcLine);
		}
		//fUrlStack.push(inUrl);
		Handle<Value>	result = compiledScript->Run();
		//fUrlStack.pop();

		if (try_catch.HasCaught() || result.IsEmpty())
		{
			if (try_catch.HasCaught())
			{
				if (outException)
				{
					outException->SetValue(fIsolate,try_catch.Exception());
				}
				try_catch.ReThrow();
			}
			String::Utf8Value error(try_catch.Exception());
			DebugMsg("V4DContext::EvaluateScript exec error on script '%S' -> %s \n",&inUrl,*error);
			Local<Value>	st = try_catch.StackTrace();
			if (!st.IsEmpty())
			{
				String::Utf8Value stValue(st);
				DebugMsg("V4DContext::EvaluateScript Stack -> <%s>\n",*stValue);			
			}
		}
		else
		{
			//Local<v8::Boolean>	bres = result->ToBoolean();
			//DebugMsg("V4DContext::EvaluateScript result is bool=%d val=%d\n",result->IsBoolean(),bres->Value());
			if (outResult)
			{
				*outResult = VJSValue(fIsolate,result);
			}
			ok = true;
		}
	}
#if V8_ACTIVATE_DEBUGGER
	if (fDebugContext)
	{
		fDebugContext->fDepth--;
		if (!fDebugContext->fDepth && VJSGlobalContext::sChromeDebuggerServer)
		{
			VJSGlobalContext::sChromeDebuggerServer->DeactivateContext(fDebugContext->fOpaqueDbgCtx, true);
		}
	}
#endif
	return ok;
}

xbox::VString V4DContext::GetCurrentScriptUrl()
{
	/*HandleScope handle_scope(fIsolate);
	Local<Context> locCtx = Handle<Context>::New(fIsolate, fPersistentCtx);
	Context::Scope context_scope(locCtx);*/
	Handle<StackTrace>	st = StackTrace::CurrentStackTrace(fIsolate,16, StackTrace::kDetailed);
	int frameCount = st->GetFrameCount();
	if (frameCount > 0)
	{
		/*DebugMsg("V4DContext::GetCurrentScriptUr::VJSParms_callAsFunction frameCount=%d\n", frameCount);
		for (uint32_t idx = 0; idx < frameCount; idx++)
		{
			Handle<StackFrame>	sf = st->GetFrame(idx);
			Handle<String>	strName = sf->GetScriptName();
			String::Utf8Value name(strName);
			DebugMsg("V4DContext::GetCurrentScriptUr::VJSParms_callAsFunction %d/%d %s\n", idx + 1, frameCount, *name);
		}*/
		Handle<StackFrame>	sf = st->GetFrame(0);
		Handle<String>	strName = sf->GetScriptNameOrSourceURL();
		String::Value	strVal(strName);
		VString	result((UniChar*)*strVal);
		return result;
	}

	/*if (fUrlStack.size())
	{
		return fUrlStack.top();
	}*/
	return VString("");
}

void* V4DContext::GetHolderPrivateData(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	return GetObjectPrivateData(info.GetIsolate(), *info.Holder());
}
void* V4DContext::GetPrivateData(v8::Isolate* inIsolate, v8::Value* inValue)
{
	return GetObjectPrivateData(inIsolate, inValue);
}

void V4DContext::AddNativeClass( xbox::JS4D::ClassDefinition* inClassDefinition )
{
//DebugMsg("V4DContext::AddNativeClass called for '%s' !!!\n",inClassDefinition->className);

	sLock.Lock();

	//for (const char** tmp = sClassList; *tmp; ++tmp)
	{
		//if (!strcmp(inClassDefinition->className,*tmp))
		{
			NativeClassesMap_t::iterator	itClass = sNativeClassesMap.find(inClassDefinition->className);
			if (itClass == sNativeClassesMap.end())
			{
				std::pair< xbox::VString, xbox::JS4D::ClassDefinition >	newClass =
					std::pair< xbox::VString, xbox::JS4D::ClassDefinition >(inClassDefinition->className,*inClassDefinition);
				sNativeClassesMap.insert( newClass );
				//break;
			}
		}
	}

	sLock.Unlock();
}



void* V4DContext::GetObjectPrivateData(v8::Value* inObject)
{
	void*	result = NULL;
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	v8::Local<v8::Object>	locObj = inObject->ToObject();
	Local<Value>	extVal = locObj->GetHiddenValue(v8::String::NewFromUtf8(fIsolate,K_WAK_PRIVATE_DATA_ID));
	if (*extVal)
	{
		if (!extVal->IsNull() && testAssert(extVal->IsExternal()))
		{
			External* 	ext = External::Cast(*extVal);
			result = (ext ? ((WakandaPrivateData*)(ext->Value()))->fData : NULL);
		}
	}
//DebugMsg("V4DContext::GetObjectPrivateData called prvData=%x!!!\n",result);
	return result;
}


void V4DContext::SetObjectPrivateData(const VJSObject& inObject, void* inPrivateData,JS4D::ClassDefinition* inClassDef)
{
//DebugMsg("V4DContext::SetObjectPrivateData called !!!\n");
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
#if V8_USE_MALLOC_IN_PLACE
	v8::Local<v8::Object>	locObj = (*ToV8Persistent(inObject.fObject))->ToObject();
#else
	v8::Local<v8::Object>	locObj = inObject.fObject->ToObject();
#endif
	WakandaPrivateData*		wakData = new WakandaPrivateData(inPrivateData, (void*)((inClassDef->finalize ? inClassDef->finalize : NULL)));
	wakData->fGCCallbackObj.Reset(fIsolate, locObj);
	wakData->fGCCallbackObj.SetWeak<v8::Object, void>((void*)wakData, DestructorRevivable);
	intptr_t memAmount = fIsolate->AdjustAmountOfExternalAllocatedMemory(K_SIMULATED_DATA_SIZE);
	Handle<External> extPtr = External::New(fIsolate,(void*)wakData);
	Local<Value> locVal = locObj->GetHiddenValue(v8::String::NewFromUtf8(fIsolate, K_WAK_PRIVATE_DATA_ID));
	if (!locVal.IsEmpty())
	{
		int a = 1;
	}
	testAssert(locObj->SetHiddenValue(v8::String::NewFromUtf8(fIsolate,K_WAK_PRIVATE_DATA_ID), extPtr));
	fPersistentCtx.Reset(fIsolate,context);
	
}


v8::Persistent< v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >*		V4DContext::GetPersistentContext(v8::Isolate* inIsolate)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData(0);
	if (testAssert(inIsolate == v8::Isolate::GetCurrent()))
	{
		return &v8Ctx->fPersistentCtx;
	}
	DebugMsg("V4DContext::GetPersistentContext bad ctx=%x / %x  taskID=%d\n",
		inIsolate,
		v8::Isolate::GetCurrent(),
		VTask::GetCurrent()->GetID());
	return NULL;
}
void* V4DContext::GetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData(0);
	return v8Ctx->GetObjectPrivateData(inObject);

}

void V4DContext::SetObjectPrivateData(v8::Isolate* inIsolate, const VJSObject& inObject, void* inPrivateData, JS4D::ClassDefinition* inClassDef)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData(0);
	return v8Ctx->SetObjectPrivateData(inObject,inPrivateData,inClassDef);
}





const char*		K_VALUES_STR_ARRAY[] = {
	"unknown",
	"undefined",
	"null",
	"Number",
	"Boolean",
	"String",
	"Object",
	"Array",
	"Function"
};

V4DContext::VChromeDebugContext::ValueType	V4DContext::VChromeDebugContext::getType(Handle<Value> inVal)
{
	if (inVal->IsUndefined())
		return UNDEFINED;

	if (inVal->IsNull())
		return NULLVAL;

	if (inVal->IsNumber())
		return NUMBER;

	if (inVal->IsBoolean())
		return BOOLEANVAL;

	if (inVal->IsString())
		return STRING;

	if (inVal->IsArray())
		return ARRAY;

	if (inVal->IsFunction())
		return FUNCTION;

	if (inVal->IsObject())
		return OBJECT;

	return UNKNOWN;
}

void V4DContext::VChromeDebugContext::displayScope(uLONG8 inFrameIndex, int inScopeIndex, Handle<Value> inScope, unsigned int inShift )
{
	VString shiftStr;
	for (int idx = 0; idx < inShift; idx++)
	{
		shiftStr += " ";
	}
	DebugMsg("%  DISPLAY FRAME(%lld).SCOPE(%d)\n", inFrameIndex, inScopeIndex);
	Local<Object>	locObj = inScope->ToObject();
	Local<Array> locArr = locObj->GetPropertyNames();
	for (uint32_t idx = 0; idx < locArr->Length(); idx++)
	{
		Handle<Value> locKey = locArr->Get(idx);
		Local<Value>     locVal = locObj->Get(locKey);
		ValueType	valType = getType(locVal);
		String::Utf8Value utf8(locKey);
		DebugMsg("%S %s type=%s\n", &shiftStr, *utf8, K_VALUES_STR_ARRAY[valType]);
		if (!strcmp(*utf8, "type_"))
		{
			String::Utf8Value utfStr(locVal);
			DebugMsg("%S                 val=%s\n", &shiftStr, *utfStr);
		}
		if (!strcmp(*utf8, "scopeType"))
		{
			//displayObject(locVal->ToObject(), inShift + 4);
			//Handle<Value>	args[1];
			//args[0] = v8::Integer::New(idx);
			Local<Value>	scopeTypeVal = locVal->ToObject()->CallAsFunction(locObj, 0, NULL);
			displayValue(scopeTypeVal, "scopeType",inShift + 4);
		}
		if (!strcmp(*utf8, "scopeObject"))
		{
			displayObject(locVal->ToObject(), inShift + 4);
			Local<Value>	scopeObjVal = locVal->ToObject()->CallAsFunction(locObj, 0, NULL);
			//displayObject(scopeObjVal->ToObject(), inShift + 4);
			Local<Array> locArray = scopeObjVal->ToObject()->GetPropertyNames();
			//Local<Array> locArray = scopeObjVal->ToObject()->GetOwnPropertyNames();
			for (uint32_t idy = 0; idy < locArray->Length(); idy++)
			{
				Handle<Value> localKey = locArray->Get(idy);
				Local<Value>     locKeyValue = scopeObjVal->ToObject()->Get(localKey);
				ValueType	valueType = getType(locKeyValue);
				String::Utf8Value utf8(localKey);
				DebugMsg("..%S %s type=%s\n", &shiftStr, *utf8, K_VALUES_STR_ARRAY[valueType]);
				if (!strcmp(*utf8, "propertyNames"))
				{
					Local<Value>	propNamesVal = locKeyValue->ToObject()->CallAsFunction(scopeObjVal->ToObject(), 0, NULL);
					ValueType	propNamesValueType = getType(propNamesVal);
					DebugMsg(".. .. %S %s type=%s\n", &shiftStr, *utf8, K_VALUES_STR_ARRAY[propNamesValueType]);
					v8::Array*	propNamesArr = v8::Array::Cast(*propNamesVal);
					for (uint32_t idz = 0; idz < propNamesArr->Length(); idz++)
					{
						Local<Value>	propNameVal = (*propNamesArr).Get(idz);
						ValueType	propNameValueType = getType(propNameVal);
						String::Utf8Value propNameStr(propNameVal);
						DebugMsg(".. .. .... %S prop(%d)=%s type=%s\n", &shiftStr, idz,*propNameStr, K_VALUES_STR_ARRAY[propNameValueType]);
					}
				}
				if (!strcmp(*utf8, "properties"))
				{
					Local<Value>	propsVal = locKeyValue->ToObject()->CallAsFunction(scopeObjVal->ToObject(), 0, NULL);
					ValueType	propsValueType = getType(propsVal);
					DebugMsg(".. .. %S %s type=%s\n", &shiftStr, *utf8, K_VALUES_STR_ARRAY[propsValueType]);
					if (propsVal->IsArray())
					{
						v8::Array*	propsArray = v8::Array::Cast(*propsVal);
						for (uint32_t idz = 0; idz < propsArray->Length(); idz++)
						{
							Local<Value>	curtVal = propsArray->Get(idz);
							ValueType		curtValValueType = getType(curtVal);
							DebugMsg(".. .. %S %s(%d) type=%s", &shiftStr, *utf8, idz, K_VALUES_STR_ARRAY[curtValValueType]);
							if (curtVal->IsNumber())
							{
								DebugMsg(" val=%f", curtVal->ToNumber()->Value());
							}
							else
							{
								displayObject(curtVal->ToObject(), 16,false);
							}
							DebugMsg("\n");
						}
					}
				}
			}
		}
		//displayValue(locVal, *utf8, inShift + 4, !inRecursive);
	}
	DebugMsg("\n");
}

void V4DContext::VChromeDebugContext::displayValue(const Handle<Value>& inVal, const char* const inName, unsigned int inShift, bool inNameOnly)
{
	VString shiftStr;
	for (int idx = 0; idx < inShift; idx++)
	{
		shiftStr += " ";
	}
	ValueType	valType = getType(inVal);
	/*DebugMsg("%SdisplayValue[%s] IsUndef=%d IsNull=%d IsNumber=%d IsBoolean=%d IsString = %d IsObject = %d IsArray = %d IsFunc = %d",
	&shiftStr,
	inName,
	inVal->IsUndefined(),
	inVal->IsNull(),
	inVal->IsNumber(),
	inVal->IsBoolean(),
	inVal->IsString(),
	inVal->IsObject(),
	inVal->IsArray(),
	inVal->IsFunction());*/

	DebugMsg("%SdisplayValue[%s] type=%s", &shiftStr, inName, K_VALUES_STR_ARRAY[valType]);
	if (inVal->IsBoolean())
	{
		DebugMsg("  val=%d\n", inVal->ToBoolean()->Value());
	}
	if (inVal->IsNumberObject())
	{
		DebugMsg("  val=%f\n", inVal->NumberValue());
	}
	if (inVal->IsNumber())
	{
		DebugMsg("  val=%f\n", inVal->NumberValue());
	}
	if (inVal->IsString())
	{
		String::Utf8Value utfStr(inVal);
		DebugMsg("  val='%s'\n", *utfStr);
	}
	if (inVal->IsArray())
	{
		v8::Array*	locArr = v8::Array::Cast(*inVal);
		DebugMsg("  val.length='%d'\n", locArr->Length());
	}
	if (inVal->IsFunction())
	{
		DebugMsg("  funcName='....'\n");
		return;
	}
	if (inVal->IsObject())
	{
		DebugMsg("\n");
		if (inNameOnly)
		{
			return;
		}
		Local<Object>    locObj = inVal->ToObject();
		displayObject(locObj, inShift);// , ::strcmp(inName, "details_"));
	}
}


void V4DContext::VChromeDebugContext::displayObject(Handle<Object> inObj, unsigned int inShift, bool inRecursive)
{

	Local<Array> locArr = inObj->GetPropertyNames();
	for (uint32_t idx = 0; idx < locArr->Length(); idx++)
	{
		Handle<Value> locKey = locArr->Get(idx);
		String::Utf8Value utf8(locKey);
		Local<Value>     locVal = inObj->Get(locKey);
		displayValue(locVal, *utf8, inShift + 4, !inRecursive);
	}
	DebugMsg("\n");
}


void V4DContext::VChromeDebugContext::HandleDebuggerServerEvents(
	const v8::Debug::Message&	inMessage,
	sLONG						inSrcLine,
	VString&					inSourceLineText,
	XBOX::VString&				inSourceURL,
	RemoteDebuggerPauseReason	inPauseReason,
	VString&					inExcTxt)
{
	bool isNew = true;
	bool shouldAbort = false;
again:
	bool l_is_ok = VJSGlobalContext::sChromeDebuggerServer->BreakpointReached(	fOpaqueDbgCtx,
																				inSourceURL,
																				inSrcLine,
																				inSourceLineText,
																				inPauseReason,
																				inExcTxt,
																				shouldAbort,
																				isNew);

	if (l_is_ok)
	{
		isNew = false;
		WAKDebuggerServerMessage::WAKDebuggerServerMsgType_t			l_command = WAKDebuggerServerMessage::SRV_EVALUATE_MSG;
		while (l_command == WAKDebuggerServerMessage::SRV_EVALUATE_MSG ||
			l_command == WAKDebuggerServerMessage::NO_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_REMOVE_BREAKPOINT_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_SET_BREAKPOINT_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_GET_EXCEPTION_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_LOOKUP_MSG ||
			l_command == WAKDebuggerServerMessage::SRV_ABORT)
		{
			WAKDebuggerServerMessage		*receivedMsg;
			receivedMsg = VJSGlobalContext::sChromeDebuggerServer->WaitFrom(fOpaqueDbgCtx);
			xbox_assert(receivedMsg);
			l_command = receivedMsg->fType;
			VString response;
			TreatDebuggerMessage(inMessage, receivedMsg, response);
			delete receivedMsg;
			if (l_command == WAKDebuggerServerMessage::SRV_CONNEXION_INTERRUPTED)
			{
				goto again;
			}
			if (fAbort)
			{
				break;
			}
		}
	}
}

void V4DContext::VChromeDebugContext::DebugMessageHandler2(const v8::Debug::Message& inMessage)
{

	V4DContext*				v8Ctx = (V4DContext*)inMessage.GetIsolate()->GetData(0);
	xbox_assert(v8Ctx);
	VChromeDebugContext*	dbgCtx = v8Ctx->GetDebugContext();
	xbox_assert(dbgCtx);
	if (dbgCtx->fAbort)
	{
		if (inMessage.IsResponse())
		{
			return;
		}
		VString	commandContinue(
			"{\"seq\":0,"
			"\"type\":\"request\","
			"\"command\":\"continue\"}");

		v8::Debug::SendCommand(commandContinue.GetCPointer(), commandContinue.GetLength(), NULL, inMessage.GetIsolate());
		return;
	}
	HandleScope handle_scope(inMessage.GetIsolate());
	if (inMessage.WillStartRunning())
	{
		/*Handle<String>	jsonStr = message.GetJSON();
		v8::String::Value	jsonVal(jsonStr);
		VString	tmp(*jsonVal);
		DebugMsg("debugMessageHandler2 <%S>\n", &tmp);*/
	}
	if (inMessage.IsEvent())
	{
		DebugEvent	dbgEvt = inMessage.GetEvent();
		DebugMsg("debugMessageHandler2 received evt=%d\n", dbgEvt);
		if (dbgEvt == v8::Exception)
		{
			Handle<String>	jsonStr = inMessage.GetJSON();
			v8::String::Value	jsonVal(jsonStr);
			VString	tmp(*jsonVal);
			DebugMsg("debugMessageHandler2 on Exc <%S>\n", &tmp);
			VJSONValue	jsonMsg;
			VError err = VJSONImporter::ParseString(tmp, jsonMsg);
			VJSONValue	jsonBody = jsonMsg.GetProperty("body");
			bool shouldAbort = false;
			if (jsonBody.IsObject())
			{
				VJSONValue		vjsonUncaughtVal = jsonBody.GetProperty("uncaught");
				if (vjsonUncaughtVal.GetBool())
				{
					VJSONObject*	vjsonBodyObj = jsonBody.GetObject();
					VJSONValue		vjsonExc = jsonBody.GetProperty("exception");
					VJSONObject*	vjsonBodyExc = vjsonExc.GetObject();
					sLONG	srcLine;
					vjsonBodyObj->GetPropertyAsNumber("sourceLine", &srcLine);
					VString		sourceLineText;
					vjsonBodyObj->GetPropertyAsString("sourceLineText", sourceLineText);

					VString		excTxt;
					vjsonBodyExc->GetPropertyAsString("text", excTxt);
					if (excTxt.Find("\""))
					{
						excTxt.ExchangeAll("\"", "\\\"");
						//VStringConvertBuffer buffer2(excTxt, VTC_UTF_8);
						//DebugMsg("str=< %s >\n", buffer2.GetCPointer());
					}
					VJSONValue	vjsonScript;
					sLONG	scrId = -1;
					vjsonScript = vjsonBodyObj->GetProperty("script");
					if (vjsonScript.GetType() != JSON_undefined)
					{
						VJSONObject*	vjsonBodyScript = vjsonScript.GetObject();
						vjsonBodyScript->GetPropertyAsNumber("id", &scrId);
					}
					VString	sourceURL;
					v8Ctx->GetSourceData(scrId, sourceURL);
					VString	jsonStr;
					if (sourceLineText.Find("\""))
					{
						sourceLineText.GetJSONString(jsonStr);
					}
					else
					{
						jsonStr = sourceLineText;
					}
					dbgCtx->HandleDebuggerServerEvents(inMessage, srcLine, jsonStr, sourceURL, EXCEPTION_RSN, excTxt);
				}
			}
		}
		if (dbgEvt == v8::Break)
		{
			v8::Isolate*	tmpIsolate = inMessage.GetIsolate();
			Handle<Object>   execState = inMessage.GetExecutionState();
			Local<Object>	frameFuncObj = dbgCtx->GetObjectFromObject(tmpIsolate, execState, V8_FRAME_STR);
			Local<Object>	setSelectedFrameFuncObj = dbgCtx->GetObjectFromObject(tmpIsolate, execState, V8_SET_SELECTED_FRAME_STR);
			Handle<Value>	args[1];
			args[0] = v8::Integer::New(tmpIsolate,0);
			Handle<Value>	result = setSelectedFrameFuncObj->ToObject()->CallAsFunction(execState, 1, args);
			Local<Value>	frameVal = frameFuncObj->CallAsFunction(execState, 1, args);
			Handle<Object>	frameObj = frameVal->ToObject();
			dbgCtx->displayObject(frameObj);
			Local<Object>	sourcePositionFunc = dbgCtx->GetObjectFromObject(tmpIsolate, frameObj, VString("sourcePosition"))->ToObject();
			Handle<Value>	sourcePosition = sourcePositionFunc->CallAsFunction(frameObj, 0, NULL);
			ValueType	sourcePositionType = dbgCtx->getType(sourcePosition);
			dbgCtx->displayObject(sourcePosition->ToObject(), 10);
			Local<Object>	sourceLineTextFunc = dbgCtx->GetObjectFromObject(tmpIsolate, frameObj, VString("sourceLineText"))->ToObject();
			Handle<Value>	sourceLineTextVal = sourceLineTextFunc->CallAsFunction(frameObj, 0, NULL);
			dbgCtx->displayValue(sourceLineTextVal, "srcLineTxt");
			Local<Object>	sourceLineFunc = dbgCtx->GetObjectFromObject(tmpIsolate, frameObj, VString("sourceLine"))->ToObject();
			Handle<Value>	sourceLineVal = sourceLineTextFunc->CallAsFunction(frameObj, 0, NULL);
			dbgCtx->displayValue(sourceLineVal, "srcLine");
			Local<Object>	sourceLocationFunc = dbgCtx->GetObjectFromObject(tmpIsolate, frameObj, VString("sourceLocation"))->ToObject();
			Handle<Value>	sourceLocation = sourceLocationFunc->CallAsFunction(frameObj, 0, NULL);
			ValueType	sourceLocationType = dbgCtx->getType(sourceLocation);
			dbgCtx->displayObject(sourceLocation->ToObject(), 10);

			Handle<String>	jsonStr = inMessage.GetJSON();
			v8::String::Value	jsonVal(jsonStr);
			VString	tmp(*jsonVal);
			DebugMsg("debugMessageHandler2 on Break <%S>\n", &tmp);
			VJSONValue	jsonMsg;
			VError err = VJSONImporter::ParseString(tmp, jsonMsg);
			VJSONValue	jsonBody = jsonMsg.GetProperty("body");
			bool shouldAbort = false;
			if (jsonBody.IsObject())
			{
				VJSONObject*	vjsonBodyObj = jsonBody.GetObject();
				sLONG	srcLine;
				bool isOk = vjsonBodyObj->GetPropertyAsNumber("sourceLine", &srcLine);
				if (isOk)
				{
					VString		sourceLineText;
					vjsonBodyObj->GetPropertyAsString("sourceLineText", sourceLineText);
					VJSONValue		vjsonScript = jsonBody.GetProperty("script");
					VJSONObject*	vjsonBodyScript = vjsonScript.GetObject();
					sLONG	scrId;
					isOk = vjsonBodyScript->GetPropertyAsNumber("id", &scrId);
					xbox_assert(isOk);
					VString	sourceURL;
					v8Ctx->GetSourceData(scrId, sourceURL);
					VString	jsonStr;
					if (sourceLineText.Find("\""))
					{
						sourceLineText.GetJSONString(jsonStr);
					}
					else
					{
						jsonStr = sourceLineText;
					}
					VString	emptyStr("");
					dbgCtx->HandleDebuggerServerEvents(inMessage, srcLine, jsonStr, sourceURL, BREAKPOINT_RSN, emptyStr);
				}
			}
			//Handle<Object>    execState = inMessage.GetExecutionState();
			dbgCtx->displayObject(execState);
		}

		xbox_assert(v8::Isolate::GetCurrent() == inMessage.GetIsolate());

		VString	commandContinue(
			"{\"seq\":0,"
			"\"type\":\"request\","
			"\"command\":\"continue\"}");

		v8::Debug::SendCommand(commandContinue.GetCPointer(), commandContinue.GetLength(), NULL, inMessage.GetIsolate());
	}
	else
	{
		if (inMessage.IsResponse())
		{
			Handle<String>	jsonStr = inMessage.GetJSON();
			v8::String::Value	jsonVal(jsonStr);
			VString	tmp(*jsonVal);
			DebugMsg("debugMessageHandler2 resp-><%S>\n", &tmp);
		}
	}
	ScopesByIDMap_t::iterator	itObj = dbgCtx->fScopesMap.begin();
	while (itObj != dbgCtx->fScopesMap.end())
	{
		(*itObj).second->Reset();
		++itObj;
	}
	dbgCtx->fScopesMap.clear();

}

V4DContext::VChromeDebugContext::~VChromeDebugContext()
{
	ScopesByIDMap_t::iterator	itScopes = fScopesMap.begin();
	while (itScopes != fScopesMap.end())
	{
		delete (*itScopes).second;
		++itScopes;
	}
}

void V4DContext::VChromeDebugContext::TreatDebuggerMessage(const v8::Debug::Message& inMessage, WAKDebuggerServerMessage* inDebuggerMessage, VString& outFrames)
{

	V4DContext* v8Ctx = (V4DContext*)inMessage.GetIsolate()->GetData(0);
	VChromeDebugContext*	dbgCtx = (VChromeDebugContext*)v8Ctx->GetDebugContext();

	switch (inDebuggerMessage->fType)
	{
	case WAKDebuggerServerMessage::SRV_EVALUATE_MSG:
	{
		xbox_assert(false);
		break;
	}
	case WAKDebuggerServerMessage::SRV_STEP_OVER_MSG:
	{
		inMessage.GetIsolate()->PerformStep(v8::Isolate::StepNext);
		break;
	}
	case WAKDebuggerServerMessage::SRV_STEP_INTO_MSG:
	{
		inMessage.GetIsolate()->PerformStep(v8::Isolate::StepIn);
		break;
	}
	case WAKDebuggerServerMessage::SRV_STEP_OUT_MSG:
	{
		inMessage.GetIsolate()->PerformStep(v8::Isolate::StepOut);
		break;
	}
	case WAKDebuggerServerMessage::SRV_CONTINUE_MSG:
	{
		break;
	}
	case WAKDebuggerServerMessage::SRV_ABORT:
	{
		//V8::TerminateExecution(inMessage.GetIsolate());
		dbgCtx->fAbort = true;
		break;
	}
	case WAKDebuggerServerMessage::SRV_CONNEXION_INTERRUPTED:
	{
		break;
	}
	case WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG:
	{
		VString	response;
		CallstackDescriptionMap		callstackDesc;
		response = ",\"params\":{\"callFrames\":[";
		DescribeCallStack(inMessage, response, callstackDesc);
		response += "],\"reason\":\"other\"}}";
		VString	excInfos;
		VIndex					idx = 1;
		bool					result = true;

		/*			while (result && ((idx = inCallstackStr.Find(K_SCRIPT_ID_STR, idx)) > 0) && (idx <= inCallstackStr.GetLength()))
		{
		VString				fileName;
		VIndex				endIndex;
		VString				scriptIDStr;
		sLONG8				sourceId;
		VCallstackDescription		callstackElement;

		idx += K_SCRIPT_ID_STR.GetLength();
		endIndex = inCallstackStr.Find(CVSTR("\""), idx);
		if (endIndex)
		{
		inCallstackStr.GetSubString(idx, endIndex - idx, scriptIDStr);
		sourceId = scriptIDStr.GetLong8();
		if (testAssert(sStaticSettings->GetData(
		inContext,
		(intptr_t)sourceId,
		callstackElement.fFileName,
		callstackElement.fSourceCode)))
		{
		if (callstackDesc.find(scriptIDStr) == callstackDesc.end())
		{
		std::pair< std::map< XBOX::VString, VCallstackDescription >::iterator, bool >	newElement =
		callstackDesc.insert(std::pair< XBOX::VString, VCallstackDescription >(scriptIDStr, callstackElement));
		testAssert(newElement.second);
		}
		}
		}
		}*/
		VJSGlobalContext::sChromeDebuggerServer->SendCallStack(
			dbgCtx->fOpaqueDbgCtx,
			response,
			excInfos,//(receivedMsg->fWithExceptionInfos ? (const char *)exceptionInfos.characters() : NULL),
			callstackDesc/*(receivedMsg->fWithExceptionInfos ? exceptionInfos.length() : 0)*/);
		break;
	}
	case WAKDebuggerServerMessage::SRV_LOOKUP_MSG:
	{
		VString			lookupResult("{\"result\":{\"result\":[");
		{

			LookupObject(inMessage, inDebuggerMessage->fObjRef, lookupResult, 0);

			lookupResult += "]},\"id\":";
			lookupResult.AppendLong8(inDebuggerMessage->fRequestId);
			lookupResult += "}";
			VJSGlobalContext::sChromeDebuggerServer->SendLookup(dbgCtx->fOpaqueDbgCtx, lookupResult);

		}
		break;
	}
	default:
		break;
	}

}


void V4DContext::VChromeDebugContext::LookupObject(
	const v8::Debug::Message&		inMessage,
	double							inObjectRef,
	VString&						outVars,
	int								inID)
{
	bool	isFirst = true;
	VString	scopeID;
	scopeID.AppendReal(inObjectRef);
	ScopesByIDMap_t::iterator	itScope = fScopesMap.find(scopeID);
	if (testAssert(itScope != fScopesMap.end()))
	{
		Handle<Object>	scopeObj = v8::Handle<Object>::New(inMessage.GetIsolate(), *(*itScope).second);
#if 1
		Local<String>	scopeObjStr = v8::String::NewFromUtf8(inMessage.GetIsolate(), "scopeObject");
		if (scopeObj->Has(scopeObjStr))
		{
			Handle<Value>	scopeObjectFunc = scopeObj->Get(scopeObjStr);
			Handle<Value>	scopeObjVal = scopeObjectFunc->ToObject()->CallAsFunction(scopeObj, 0, NULL);
			Handle<Value>    scopePropNamesValue = scopeObjVal->ToObject()->Get(v8::String::NewFromUtf8(inMessage.GetIsolate(),"propertyNames"));
			Handle<Value>	scopePropNamesVal = scopePropNamesValue->ToObject()->CallAsFunction(scopeObjVal->ToObject(), 0, NULL);
			Handle<Value>    scopePropsValue = scopeObjVal->ToObject()->Get(v8::String::NewFromUtf8(inMessage.GetIsolate(),"properties"));
			Handle<Value>	scopePropsVal = scopePropsValue->ToObject()->CallAsFunction(scopeObjVal->ToObject(), 0, NULL);
			v8::Array*		scopePropNamesArr = v8::Array::Cast(*scopePropNamesVal);
			v8::Array*		scopePropsArr = v8::Array::Cast(*scopePropsVal);
			xbox_assert(scopePropNamesArr->Length() == scopePropsArr->Length());
			for (uint32_t idy = 0; idy < scopePropsArr->Length(); idy++)
			{
				/*if (!strcmp(*utf8, "propertyNames"))
				{
				Local<Value>	propNamesVal = locKeyValue->ToObject().val_->CallAsFunction(scopeObjVal->ToObject(), 0, NULL);
				ValueType	propNamesValueType = getType(propNamesVal);
				DebugMsg(".. ..  %s type=%s\n", *utf8, K_VALUES_STR_ARRAY[propNamesValueType]);
				v8::Array*	propNamesArr = v8::Array::Cast(*propNamesVal);
				outVars += generateLookupString(propNamesArr);
				for (uint32_t idz = 0; idz < propNamesArr->Length(); idz++)
				{
				Local<Value>	propNameVal = (*propNamesArr).Get(idz);
				ValueType	propNameValueType = getType(propNameVal);
				String::Utf8Value propNameStr(propNameVal);
				DebugMsg(".. .. .... %s type=%s\n", *propNameStr, K_VALUES_STR_ARRAY[propNameValueType]);
				}
				}*/

				Handle<Value>	propsVal = scopePropsArr->Get(idy);
				//displayObject(propsVal->ToObject(), 16, false);
				Handle<Value>	nameFn = propsVal->ToObject()->Get(v8::String::NewFromUtf8(inMessage.GetIsolate(),"name"));
				Handle<Value>	propNameValue = nameFn->ToObject()->CallAsFunction(propsVal->ToObject(), 0, NULL);
				String::Utf8Value	utf8PropName(propNameValue);
				const char* propChar = *utf8PropName;
				displayValue(propNameValue, "propName", 1, false);
				Local<Value>	valueFn = propsVal->ToObject()->Get(v8::String::NewFromUtf8(inMessage.GetIsolate(),"value"));
				Local<Value>	valueVal = valueFn->ToObject()->CallAsFunction(propsVal->ToObject(), 0, NULL);
				if (strcmp(propChar, "storage"))
				{
					//displayValue(valueVal, "value", 1, false);
				}
				else
				{
					int a = 1;
				}
				VString	lJson;
				Local<Value>	value_ = valueVal->ToObject()->Get(String::NewFromUtf8(inMessage.GetIsolate(), "value_"));
				valueToJSON(inMessage.GetIsolate(), value_, value_->IsObject(), lJson);
				if (isFirst)
				{
					isFirst = false;
				}
				else
				{
					outVars += ",";
				}
				outVars += lJson;
				outVars += ",\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"";
				Local< String > toStr = propNameValue->ToString();
				String::Utf8Value	utf8Val(toStr);
				outVars += *utf8Val;
				outVars += "\"}";
			}
		}
		else
		{
			Local<Array>	propNamesArr = scopeObj->GetPropertyNames();
			for (uint32_t idx = 0; idx < propNamesArr->Length(); idx++)
			{
				if (isFirst)
				{
					isFirst = false;
				}
				else
				{
					outVars += ",";
				}
				Handle<Value>	locKey = propNamesArr->Get(idx);
				Local<Value>     locVal = scopeObj->Get(locKey);
				VString	lJson;
				valueToJSON(inMessage.GetIsolate(), locVal, locVal->IsObject(), lJson);
				outVars += lJson;
				outVars += ",\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"";
				Local< String > toStr = locKey->ToString();
				String::Utf8Value	utf8Val(toStr);
				outVars += *utf8Val;
				outVars += "\"}";
			}
		}
#endif
		/*if (testAssert(scopePropNamesVal->IsArray()))
		{
		outVars += generateLookupString(scopePropsVal->ToObject());
		}*/
	}
	DebugMsg("LookupObject-> <%S>\n", &outVars);
#if 0

	if (bIsEntityModel)
	{
		Identifier		idLength(excState, "length");
		JSValue			jsLength = jsObj->get(excState, idLength);
		nEntityCount = jsLength.asUInt32();
		if (nEntityCount > 100)
			nEntityCount = 100;

		bool			bNeedLastComma = (arrPropertyNames.size() > 0);
		for (uint32_t j = 0; j < nEntityCount; j++)
		{
			//outJSON = makeUString <UString, UString> ( outJSON, UString ( "\t\t\t\"" ) );
			UString		strIndex = UString::number(j);
			//outJSON = makeUString <UString, UString> ( outJSON, strIndex );

			Identifier	idIndex(excState, strIndex);
			JSValue		jsEntityValue = jsObj->get(excState, idIndex);
			// GH valueToJSON ( excState, jsEntityValue, jsEntityValue. isObject ( ), outJSON );

			if (excState->hadException())
				excState->clearException();

			if (j + 1 < nEntityCount || bNeedLastComma)
			{
				//outJSON = makeUString <UString, UString> ( outJSON, UString ( "," ) );
			}
		}
	}

	if (ustrClassName == UString("Array") && nPropertyCount > 100)
		nPropertyCount = 100;

	bool				bIsFunction = false;
	for (size_t i = 0; i < nPropertyCount; i++)
	{
		bIsFunction = false;

		Identifier&		prop = arrPropertyNames[i];

		UString			strPropName = prop.ustring();
		UChar			chSecond = strPropName[1];
		UChar			chFifth = strPropName[4];
		UChar			chSpace = ' ';
		UChar			chF = 'f';

		if (prop.length() > 5 && chFifth == chSpace)
		{
			strPropName = strPropName.substringSharingImpl(5);
			//outJSON = makeUString <UString, UString> ( outJSON, strPropName );

			/*if ( szchName [ 1 ] == 'v' )
			ustrODescription = ustrODescription + UString ( " - static property" );
			else*/ if (chSecond == chF)
			{
				bIsFunction = true;
			}
			/*else
			ustrODescription = ustrODescription + UString ( " - unknown static" );*/
		}
		else
		{
			//outJSON = makeUString <UString, UString> ( outJSON, strPropName );
		}

		if (bIsFunction)
		{
			//outJSON = makeUString <UString, UString> ( outJSON, UString ( "{\"type\":\"function\"," ) );
			Identifier		propFunction(excState, strPropName);
			JSValue			jsFuncValue = jsObj->get(excState, propFunction);
			if (jsFuncValue.isObject())
			{
				protectJSValue(jsFuncValue);
			}
		}
		else
		{
			Identifier		propReal(excState, strPropName);
			JSValue			jsPropValue = jsObj->get(excState, propReal);
			JSC::UString	l_json;
			if (jsPropValue.isObject())
			{
				valueToJSON(excState, jsPropValue, true, l_json);
				if (l_json.length() > 0)
				{
					if (l_first)
					{
						l_first = false;
					}
					else
					{
						outVars = Concatenate(outVars, UString(","));
					}
					outVars = Concatenate(outVars, l_json);
					outVars = Concatenate(
						outVars,
						UString(",\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\""));
					outVars = Concatenate(
						outVars,
						strPropName);
					outVars = Concatenate(
						outVars,
						UString("\"}"));
				}
			}
			else
			{
				if (l_first)
				{
					l_first = false;
				}
				else
				{
					outVars = Concatenate(outVars, UString(","));
				}
				valueToJSON(excState, jsPropValue, false, l_json);
				outVars = Concatenate(outVars, l_json);
				outVars = Concatenate(
					outVars,
					UString(",\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\""));
				outVars = Concatenate(
					outVars,
					strPropName);
				outVars = Concatenate(
					outVars,
					UString("\"}"));
			}

			if (excState->hadException())
				excState->clearException();
		}

		if (i + 1 < nPropertyCount)
		{
			//outJSON = makeUString <UString, UString> ( outJSON, UString ( "," ) );
		}
	}
#endif
}

void V4DContext::VChromeDebugContext::DescribeCallStack(const v8::Debug::Message& inMessage, VString& outFrames, CallstackDescriptionMap& outCSDescMap)
{
	v8::Isolate*		tmpIsolate = inMessage.GetIsolate();
	V4DContext* v8Ctx = (V4DContext*)tmpIsolate->GetData(0);
	Handle<Object>    execState = inMessage.GetExecutionState();
	displayObject(execState);
	Handle<Object>	frameCountObj = GetObjectFromObject(tmpIsolate, execState, V8_FRAME_COUNT_STR);
	Local<Value>	frameCountVal = frameCountObj->CallAsFunction(execState, 0, NULL);
	int64_t		frameCount = frameCountVal->ToInt32()->Value();
	DebugMsg("DescribeCallStack frameCount=%lld\n", frameCount);
	Local<Object>	frameFuncObj = GetObjectFromObject(tmpIsolate, execState, V8_FRAME_STR);
	Local<Object>	setSelectedFrameFuncObj = GetObjectFromObject(tmpIsolate, execState, V8_SET_SELECTED_FRAME_STR);
#if 0
	VString		frames;
	for (int idx = 0; idx < 1/*frameCount*/; idx++)
	{
		Handle<Value>	args[1];
		args[0] = v8::Integer::New(idx);
		Local<Value>	frameVal = frameFuncObj.val_->CallAsFunction(execState, 1, args);
		displayValue(frameVal, "frame", 10);
		Handle<String> toTextStr = v8::String::NewFromTwoByte(tmpIsolate, V8_TOTEXT_STR.GetCPointer(), v8::String::kNormalString, V8_TOTEXT_STR.GetLength());
		Local<Object>	curtFrameObj = frameVal.val_->ToObject();

		Handle<Object>	scopeCountObj = GetObjectFromObject(tmpIsolate, curtFrameObj, V8_SCOPE_COUNT_STR);
		Local<Value>	scopeCountVal = scopeCountObj.val_->CallAsFunction(curtFrameObj, 0, NULL);
		int64_t		scopeCount = frameCountVal.val_->ToInt32()->Value();
		DebugMsg("TreatDebuggerMessage frameCount=%lld\n", frameCount);
		Handle<Object>	scopeFuncObj = GetObjectFromObject(tmpIsolate, curtFrameObj, V8_SCOPE_STR);
		for (int idy = 0; idy < scopeCount; idy++)
		{
			Handle<Value>	argv[1];
			argv[0] = v8::Integer::New(idy);
			Handle<Value>	scopeVal = scopeFuncObj.val_->CallAsFunction(curtFrameObj, 1, argv);
			Handle<Object>	scopeObj = scopeVal->ToObject();
			Local<Object>	toTextFuncObj = GetObjectFromObject(tmpIsolate, scopeObj, V8_TOTEXT_STR);
			Local<Value>	scopeStr = toTextFuncObj->CallAsFunction(scopeObj, 1, argv);
			if (scopeStr->IsString())
			{
				v8::String::Utf8Value	strVal(scopeStr);
				DebugMsg("TreatDebuggerMessage scope(%d) = %s\n", idy, *strVal);
			}
			displayScope(idx, idy, scopeVal, 16);
			if (!idy)
			{
				s125 = generateLookupString(scopeObj);
			}
			if (idy == 1)
			{
				s124 = generateLookupString(scopeObj);
			}
		}
	}
	frames += ",\"params\":{\"callFrames\":[";

	frames += "],\"reason\":\"other\"}}";
#endif

	if (!testAssert(frameCount > 0))
		return;

	//outExceptionInfos = UString("");
	int								l_idx = -1;
	bool	l_first = true;
	VString							ustrCallStack;

	/*	const ScopeChainNode*			nodeClosure = m_currentCallFrame->scopeChain();
	int								nClosureIndex = -1;
	while (nodeClosure != 0)
	{
	nodeClosure = nodeClosure->next.get();
	nClosureIndex++;
	}
	*/

	for (int idx = 0; idx < frameCount; idx++)
	{
		Handle<Value>	args[1];
		args[0] = v8::Integer::New(tmpIsolate,idx);
		Handle<Value>	result = setSelectedFrameFuncObj->ToObject()->CallAsFunction(execState, 1, args);
		Local<Value>	frameVal = frameFuncObj->CallAsFunction(execState, 1, args);
		Handle<Object>	frameObj = frameVal->ToObject();
		Local<Object>	sourceLineTextFunc = GetObjectFromObject(tmpIsolate, frameObj, VString("sourceLineText"))->ToObject();
		Handle<Value>	sourceLineTextVal = sourceLineTextFunc->CallAsFunction(frameObj, 0, NULL);
		displayValue(sourceLineTextVal, "srcLine");
		Local<Object>	detailsObj = GetObjectFromObject(tmpIsolate, frameObj, VString("details_"))->ToObject();
		displayObject(detailsObj, 1, false);
		Local<Object>	localNameFuncObj = GetObjectFromObject(tmpIsolate, detailsObj, V8_LOCAL_NAME_STR)->ToObject();
		args[0] = v8::Integer::New(tmpIsolate,0);
		Handle<Value>	localNameVal = localNameFuncObj->CallAsFunction(detailsObj, 1, args);
		displayValue(localNameVal, "localName");
		Local<Object>	argCountFuncObj = GetObjectFromObject(tmpIsolate, detailsObj, V8_ARGUMENT_COUNT_STR)->ToObject();
		Handle<Value>	argCountVal = argCountFuncObj->CallAsFunction(detailsObj, 0, NULL);
		displayValue(argCountVal, "argCount");
		Local<Object>	receiverFuncObj = GetObjectFromObject(tmpIsolate, detailsObj, VString("receiver"))->ToObject();
		Handle<Value>	receiverVal = receiverFuncObj->CallAsFunction(detailsObj, 0, NULL);
		displayValue(receiverVal, "receiver");

		/*nodeClosure = m_currentCallFrame->scopeChain();
		while (nodeClosure != 0)
		{
		JSValue							jsvalClosureObject(nodeClosure->object.get());

		protectJSValue(jsvalClosureObject);
		ustrCallStack = makeUString <UString, UString>(ustrCallStack, UString::number(m_nCurrentRef - 1));

		nClosureIndex--;

		nodeClosure = nodeClosure->next.get();
		}*/


		//while (jsCallFrame != 0 && nDepth > 0)
		//{
		l_idx++;
		//nDepth--;

		VString						strName;
		/*if (jsCallFrame->type() == DebuggerCallFrame::FunctionType)
		{
		JSValue		jsException;
		JSValue		jsFunctionSource = jsCallFrame->evaluate("arguments.callee.toString ( )", jsException);
		UString		ustrFunctionSource = jsFunctionSource.getString(inDebuggerCallFrame.dynamicGlobalObject()->globalExec());
		ustrFunctionSource = ustrFunctionSource.substringSharingImpl(0, ustrFunctionSource.find("{"));
		strName = makeUString <UString, UString>(strName, ustrFunctionSource);
		if (strName.find("function ()") == 0)
		strName = "";

		UString					strSignature = strName;
		//			getFuntionParamNames ( strSignature, vctrParamNames );

		if (jsCallFrame->functionName().isNull() || jsCallFrame->functionName().isEmpty())
		strName = "";
		}
		else*/
		{
			strName += "Global scope";
		}

		//if (l_nb_frames < K_NB_MAX_DBG_FRAMES)
		{
			Local<Object>	sourceLineFuncObj = GetObjectFromObject(tmpIsolate, frameObj, V8_SOURCE_LINE_STR)->ToObject();
			Handle<Value>	sourceLineVal = sourceLineFuncObj->CallAsFunction(frameObj, 0, NULL);
			int l_line_nb = sourceLineVal->ToNumber()->Value();// ((nDepth + 1 == nMaxDepth) ? inVirtualLineNumber - 1 : jsCallFrame->line());
			int l_col_nb = 1;// ((nDepth + 1 == nMaxDepth) ? 0 : jsCallFrame->column());

			Local<Object>	sourceAndPositionTextFunc = GetObjectFromObject(tmpIsolate, frameObj, VString("sourceAndPositionText"))->ToObject();
			Handle<Value>	sourceAndPositionText = sourceAndPositionTextFunc->CallAsFunction(frameObj, 0, NULL);
			ValueType	sourcePositionTextType = getType(sourceAndPositionText);
			String::Utf8Value	sourceAndPositionTextStr(sourceAndPositionText);

			Local<Object>	sourcePositionFunc = GetObjectFromObject(tmpIsolate, frameObj, VString("sourcePosition"))->ToObject();
			Handle<Value>	sourcePosition = sourcePositionFunc->CallAsFunction(frameObj, 0, NULL);
			ValueType	sourcePositionType = getType(sourcePosition);
			displayObject(sourcePosition->ToObject(), 10);
			Local<Object>	sourceLocationFunc = GetObjectFromObject(tmpIsolate, frameObj, VString("sourceLocation"))->ToObject();
			Handle<Value>	sourceLocation = sourceLocationFunc->CallAsFunction(frameObj, 0, NULL);
			ValueType	sourceLocationType = getType(sourceLocation);
			displayObject(sourceLocation->ToObject(), 10);
			Handle<Object> srcLocObj = sourceLocation->ToObject();
			Local<Object>	scriptObj = GetObjectFromObject(tmpIsolate, srcLocObj, VString("script"))->ToObject();
			displayObject(scriptObj, 15);
			Local<Object>	detailsObj = GetObjectFromObject(tmpIsolate, frameObj, VString("details_"))->ToObject();
			Local<Object>	frameIdFuncObj = GetObjectFromObject(tmpIsolate, detailsObj, V8_FRAME_ID_STR)->ToObject();
			uLONG8 l_frame_id = frameIdFuncObj->CallAsFunction(detailsObj, 0, NULL)->ToNumber()->Value();
			if (!l_first)
			{
				outFrames += ",";
			}
			else
			{
				l_first = false;
			}
			VCallstackDescription		callstackElement;
			callstackElement.fFileName = VString(*sourceAndPositionTextStr);
			VIndex	l_index = callstackElement.fFileName.Find(" ", 1);
			if (l_index)
			{
				callstackElement.fFileName.SubString(1, l_index - 1);
			}
			int sourceID;
			v8Ctx->GetSourceID(callstackElement.fFileName, sourceID);
			VString	scriptIDStr;
			scriptIDStr.AppendULong8(sourceID);
			if (outCSDescMap.find(scriptIDStr) == outCSDescMap.end())
			{
				std::pair< std::map< XBOX::VString, VCallstackDescription >::iterator, bool >	newElement =
					outCSDescMap.insert(std::pair< XBOX::VString, VCallstackDescription >(scriptIDStr, callstackElement));
				testAssert(newElement.second);
			}
			outFrames += "{\"callFrameId\":\"{\\\"ordinal\\\":";
			outFrames.AppendLong(idx);
			outFrames += ",\\\"injectedScriptId\\\":3}\",\"functionName\":\"";
			outFrames += strName;
			outFrames += "\",\"location\":{\"scriptId\":\"";
			outFrames.AppendLong8(sourceID);// += UString::number((long long)jsCallFrame->sourceID()));
			outFrames += "\",\"lineNumber\":";
			outFrames.AppendLong(l_line_nb);
			outFrames += ",\"columnNumber\":";
			outFrames.AppendLong(l_col_nb);
			outFrames += "},\"scopeChain\":[";

			/*if (inWithExceptionInfos && !outExceptionInfos.length())
			{
			#define K_MAX_FILENAME_SIZE		(512)
			char			fileName[K_MAX_FILENAME_SIZE];
			int				filenameLength = K_MAX_FILENAME_SIZE;
			sChromeDebuggerServer->GetFilenameFromId(
			fContext,
			jsCallFrame->sourceID(),
			fileName,
			filenameLength);
			JSC::UString	filenameUString((const UChar*)fileName, filenameLength);

			outExceptionInfos = UString("\"line\":");
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString::number(l_line_nb));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString(",\"url\":\""));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, filenameUString);
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString("\",\"repeatCount\":1,\"stackTrace\":["));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString("{\"functionName\":\"\",\"url\":\""));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, filenameUString);
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString("\",\"lineNumber\":"));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString::number(l_line_nb));
			outExceptionInfos = makeUString <UString, UString>(outExceptionInfos, UString(",\"columnNumber\":2}]"));
			}*/
			VString		l_tmp_str;
			DescribeScopes(tmpIsolate, frameObj, false, l_frame_id, l_tmp_str);

			outFrames += l_tmp_str;

		}

		//jsCallFrame = jsCallFrame->caller();

	}

}

void V4DContext::VChromeDebugContext::AddPersistentObjectInMap(v8::Isolate* inIsolate, const VString& inID, Persistent<Object>* inObj)
{
	ScopesByIDMap_t::iterator	itObj = fScopesMap.find(inID);
	if (itObj != fScopesMap.end())
	{
		(*itObj).second->Reset();
		fScopesMap.erase(inID);
	}
	fScopesMap.insert(std::pair<const VString, v8::Persistent<Object>*>(inID, inObj));

}

void V4DContext::VChromeDebugContext::AddPersistentObjectInMap(v8::Isolate* inIsolate, const VString& inID, Handle<Object>& inObj)
{
	ScopesByIDMap_t::iterator	itObj = fScopesMap.find(inID);
	if (itObj != fScopesMap.end())
	{
		(*itObj).second->Reset(inIsolate, inObj);
	}
	else
	{
		Persistent<Object>*	newObj = new Persistent<Object>(inIsolate, inObj);
		fScopesMap.insert(std::pair<const VString, v8::Persistent<Object>*>(inID, newObj));
	}
}

Handle<Object> V4DContext::VChromeDebugContext::GetObjectFromObject(v8::Isolate* inIsolate, Handle<Object>& inObject, const VString& inName)
{
	EscapableHandleScope handle_scope(inIsolate);

	Handle<String> locStr = v8::String::NewFromTwoByte(inIsolate, inName.GetCPointer(), v8::String::kNormalString, inName.GetLength());
	Local<Object>	resultObj = inObject->Get(locStr)->ToObject();
	return handle_scope.Escape(resultObj);
}

void V4DContext::VChromeDebugContext::AddClosureScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj)
{
	AddPersistentObjectInMap(inIsolate, inScopeId, inObj);
	outScopes += "{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
	outScopes += inScopeId;// UString::number(m_nCurrentRef - 1));
	outScopes += "}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"closure\"}";
}

void V4DContext::VChromeDebugContext::AddGlobalScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj)
{
	AddPersistentObjectInMap(inIsolate, inScopeId, inObj);
	outScopes += "{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
	outScopes += inScopeId;// UString::number(m_nCurrentRef - 1));
	outScopes += "}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"global\"}";
}
void V4DContext::VChromeDebugContext::AddLocalScope(VString& outScopes, v8::Isolate* inIsolate, const VString& inScopeId, Handle<Object>& inObj)
{
	AddPersistentObjectInMap(inIsolate, inScopeId, inObj);
	outScopes += "{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
	outScopes += inScopeId;// UString::number(m_nCurrentRef - 1));
	outScopes += "}\",\"className\":\"Object\",\"description\":\"Object\"},\"type\":\"local\"}";
}


void V4DContext::VChromeDebugContext::DescribeScopes(
	v8::Isolate*								inIsolate,
	Handle<Object>&								inFrame,
	bool										inLocalOnly,
	uLONG8										inFrameId,
	VString&									outScopes)
{

	if (inLocalOnly)
	{
		xbox_assert(false);// on ne devrait pas passer ici , positionner .id si on passe par la

		/*JSObject*						obj = inJsCallFrame->scopeChain()->object.get();
		JSValue							jsvalScopeObject(obj);

		protectJSValue(jsvalScopeObject);
		outScopes = Concatenate(outScopes, UString("GH local scope"));*/

	}
	else
	{
		int			nIndex = 0;
		int			l_first = 1;
		VString		appScopeID;
		Handle<Object>	scopeCountObj = GetObjectFromObject(inIsolate, inFrame, V8_SCOPE_COUNT_STR);
		Local<Value>	scopeCountVal = scopeCountObj->CallAsFunction(inFrame, 0, NULL);
		int64_t		scopeCount = scopeCountVal->ToInt32()->Value();
		DebugMsg("DescribeScopes scopeCount=%lld\n", scopeCount);
		Handle<Object>	scopeFuncObj = GetObjectFromObject(inIsolate, inFrame, V8_SCOPE_STR);
		for (int idy = 0; idy < scopeCount; idy++)
		{
			Handle<Value>	argv[1];
			argv[0] = v8::Integer::New(inIsolate, idy);
			Local<Value>	scopeVal = scopeFuncObj->CallAsFunction(inFrame, 1, argv);
			Handle<Object>	scopeObj = scopeVal->ToObject();
			Local<Object>	toTextFuncObj = GetObjectFromObject(inIsolate, scopeObj, V8_TOTEXT_STR);
			Local<Value>	scopeStr = toTextFuncObj->CallAsFunction(scopeObj, 1, argv);
			if (scopeStr->IsString())
			{
				v8::String::Utf8Value	strVal(scopeStr);
				DebugMsg("DescribeScopes scope(%d) = %s\n", idy, *strVal);
			}
			//displayScope(inFrameId, idy, scopeVal, 16);

			VString	scopeID;
			double	scopeIDReal = inFrameId;
			scopeIDReal += (idy + 1) / 1000.0;
			scopeID.AppendReal(scopeIDReal);
			//if (appScopeID.IsEmpty())
			{
				appScopeID = scopeID;
			}
			// global scope only
			if (scopeCount == 1)
			{
				AddGlobalScope(outScopes, inIsolate, scopeID, scopeObj);
			}
			else
			{
				// local+global
				if (scopeCount >= 2)
				{
					if (!nIndex)
					{
						AddLocalScope(outScopes, inIsolate, scopeID, scopeObj);
					}
					else
					{
						outScopes += ",";
						if (nIndex == scopeCount - 1)
						{
							AddGlobalScope(outScopes, inIsolate, scopeID, scopeObj);
						}
						else
						{
							AddClosureScope(outScopes, inIsolate, scopeID, scopeObj);
						}
					}
				}
			}

			nIndex++;

		}

		outScopes += "],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
		outScopes += appScopeID;// += ng::number(m_nCurrentRef - 1));
		outScopes += "}\",\"className\":\"Application\",\"description\":\"Application\"}";
		outScopes += "}";

	}

}



void V4DContext::VChromeDebugContext::valueToJSON(v8::Isolate*		inIsolate,
	Handle<Value>&		inValue,
	bool				inFull,
	VString&			outJSON)
{
	if (inFull)
	{
		if (inValue->IsObject())
		{
			//protectJSValue(inValue);
			goto treat_object;
		}
		outJSON += "\"result\":";
	}

	if (inValue->IsNumber())
	{
		double		nValue = inValue->NumberValue();
		VString		tmpStr;
		outJSON += "{\"value\":{\"type\":\"number\",\"value\":";
		//double		nMyNaN = nonInlineNaN ( );
		if (nValue != nValue)
		{
			tmpStr += "null,\"description\":\"NaN\"";
		}
		else if (std::numeric_limits<double>::infinity() == nValue)
		{
			tmpStr += "null,\"description\":\"Infinity\"";
		}
		else if (std::numeric_limits<double>::infinity() == -nValue)
		{
			tmpStr += "null,\"description\":\"-Infinity\"";
		}
		else
		{
			tmpStr.AppendReal(nValue);
			tmpStr += ",\"description\":\"";
			tmpStr.AppendReal(nValue);
			tmpStr += "\"";
		}
		outJSON += tmpStr;
		outJSON += "}";
	}
	else if (inValue->IsString())
	{
		v8::String::Value	tmpStrVal(inValue);
		VString		tmpStr(*tmpStrVal);
		VString		jsonUstr;
		if (tmpStr.GetJSONString(jsonUstr) == VE_OK)
		{
			outJSON += "{\"value\":{\"type\":\"string\",\"value\":\"";
			outJSON += jsonUstr;
			outJSON += "\"}";
		}
		else
		{
			outJSON += "{\"value\":{\"type\":\"string\",\"value\":\"";
			outJSON += "internal error during value computation";
			outJSON += "\"}";
		}
	}
	else if (inValue->IsUndefined())
	{
		outJSON += "{\"value\":{\"type\":\"undefined\"}";
	}
	else if (inValue->IsNull())
	{
		outJSON += "{\"value\":{\"type\":\"object\",\"value\":null}";
	}
	else if (inValue->IsBoolean())
	{
		bool		bValue = false;
		outJSON += "{\"value\":{\"type\":\"boolean\",\"value\":";
		bValue = inValue->ToBoolean()->Value();
		if (bValue)
		{
			outJSON += "true";
		}
		else
		{
			outJSON += "false";
		}
		outJSON += "}";
	}
	else if (inValue->IsObject())
	{
	treat_object:
		int a = 1;
#if 1
		Handle<Object>	jsObj = inValue->ToObject();
		Persistent<Object>*	newObj = new Persistent<Object>(inIsolate, jsObj);
		VString objectId;
		objectId.AppendReal((double)((sLONG8)newObj));
		AddPersistentObjectInMap(inIsolate, objectId, newObj);
		Local<String>	ustrClassNameStr = jsObj->GetConstructorName();
		v8::String::Value	stringValue(ustrClassNameStr);
		VString				ustrClassName(*stringValue);

		if (ustrClassName.Find("EntityRecord"))// == VString("EntityRecord"))
		{
			ustrClassName = VString("Entity");
		}
		/*if (ustrClassName == VString("Arguments"))
		{
		double			l_length=1;
		Handle<String>	lPropId = v8::String::New("length");
		Handle<Value>	lValue = jsObj->Get(lPropId);
		if (lValue->IsNumber())
		{
		l_length = lValue->ToNumber()->Value();
		}
		else
		{
		l_length = 0;
		}
		outJSON += "{\"value\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
		outJSON += objectId;
		outJSON += "}\",\"subtype\":\"array\",\"className\":\"Arguments\",\"description\":\"Arguments[";
		outJSON.AppendULong8(l_length);
		outJSON += "]\"}";
		return;
		}*/
		if (inValue->IsFunction())//ustrClassName == VString("Function"))
		{
			outJSON += "{\"value\":{\"type\":\"function\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":";
			outJSON += objectId;
			outJSON += "}\",\"className\":\"Object\",\"description\":\"function () {}";
			outJSON += "\"}";
			return;
		}

		if (inFull)
		{
			Handle<Array>	arrPropertyNames;

			if (ustrClassName == VString("Date"))
			{
				String::Utf8Value	strValue(inValue);
				/*unsigned int	nGMTIndex = strValue.find(UString("GMT"));
				size_t			nLastIndex = strValue.find(UString(" "), nGMTIndex);
				UString			strDate;
				if (nLastIndex != notFound)
				strDate = strValue.substringSharingImpl(0, nLastIndex);*/ //GH

				outJSON += VString("{\"value\":{\"type\":\"string\",\"value\":\"");
				outJSON += *strValue;// strDate;
				outJSON += VString("\"}");
			}
			else
			{
				outJSON += "{\"value\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":4,\\\"id\\\":";
				outJSON += objectId;
				outJSON += "}\",\"className\":\"Object\",\"description\":\"";
				outJSON += ustrClassName;
				outJSON += "\"}";

				int					nSentCount = 0;
				bool				bOutOfOrder = false;

				Handle<String>		idName = v8::String::NewFromUtf8(inIsolate,"name");
				bool				bMustSendName = jsObj->Has(idName);// || jsObj->hasOwnProperty(excState, idName);
				Handle<String>		idNAME = v8::String::NewFromUtf8(inIsolate, "NAME");
				bool				bMustSendNAME = false; //jsObj-> hasProperty ( excState, idNAME ) || jsObj-> hasOwnProperty ( excState, idNAME );
				Handle<String>		idId = v8::String::NewFromUtf8(inIsolate, "id");
				bool				bMustSendId = jsObj->Has(idId);// || jsObj->hasOwnProperty(excState, idId);
				Handle<String>		idID = v8::String::NewFromUtf8(inIsolate, "ID");
				bool				bMustSendID = false; //jsObj-> hasProperty ( excState, idID ) || jsObj-> hasOwnProperty ( excState, idID );
				Handle<String>		idLength = v8::String::NewFromUtf8(inIsolate, "length");
				bool				bMustSendLength = jsObj->Has(idLength);// || jsObj->hasOwnProperty(excState, idLength);

				// TODO: Need to merge results of two calls
				//jsObj-> getAllPropertyNames ( excState, arrPropertyNames );
				//GH arrPropertyNames.m_forceAll = true;
				arrPropertyNames = jsObj->GetPropertyNames();

				bool				bIsFunction = false;
				for (size_t i = 0; i < arrPropertyNames->Length() && nSentCount < 3; i++)
				{
					bIsFunction = false;
					bOutOfOrder = false;

					Handle<String>		prop = arrPropertyNames->Get((uint32_t)i)->ToString();

					if (bMustSendName)
					{
						i--;
						bMustSendName = false;
						bOutOfOrder = true;
						prop = idName;
					}
					else if (bMustSendNAME)
					{
						i--;
						bMustSendNAME = false;
						bOutOfOrder = true;
						prop = idNAME;
					}
					else if (bMustSendId)
					{
						i--;
						bMustSendId = false;
						bOutOfOrder = true;
						prop = idId;
					}
					else if (bMustSendID)
					{
						i--;
						bMustSendID = false;
						bOutOfOrder = true;
						prop = idID;
					}
					else if (bMustSendLength)
					{
						i--;
						bMustSendLength = false;
						bOutOfOrder = true;
						prop = idLength;
					}

					v8::String::Value	propValue(prop);
					VString				strPropName(*propValue);
					if (!bOutOfOrder &&
						(strPropName == VString("name") ||
						strPropName == VString("NAME") ||
						strPropName == VString("id") ||
						strPropName == VString("ID") ||
						strPropName == VString("length")))
						continue;

					nSentCount++;

					UniChar			chSecond = strPropName.GetUniChar(2);
					UniChar			chFifth = strPropName.GetUniChar(5);
					UniChar			chSpace = ' ';
					UniChar			chF = 'f';

					if (prop->Length() > 5 && chFifth == chSpace)
					{
						//GH strPropName = strPropName.substringSharingImpl(5);
						if (chSecond == chF)
						{
							bIsFunction = true;
						}

					}
					else
					{
					}
					if (bIsFunction)
					{
					}
					else
					{
						//Identifier		propReal(excState, strPropName);
						//JSValue			jsPropValue = jsObj->get(excState, propReal);

						//if (inExecState->hadException())
						//	inExecState->clearException();
					}

					if (i + 1 < arrPropertyNames->Length() || nSentCount < 3)
					{
					}

				}

			}

		}
		else
		{

		}
#endif
	}
	else
	{
		outJSON += "{\"value\":{\"type\":\"string\",\"value\":\"";
		outJSON += "undefined";
		outJSON += "\"}";
	}

}


int64_t V4DContext::VChromeDebugContext::GetIntegerFromObject(v8::Isolate* inIsolate, Handle<Object>& inObject, const VString& inName)
{
	Handle<String> locStr = v8::String::NewFromTwoByte(inIsolate, inName.GetCPointer(), v8::String::kNormalString, inName.GetLength());
	Local<Object>	intObj = inObject->Get(locStr)->ToObject();
	Local<Value>	intVal = intObj->CallAsFunction(inObject, 0, NULL);
	int64_t		intValue = intVal->ToInt32()->Value();
	return intValue;
}


END_TOOLBOX_NAMESPACE

#endif


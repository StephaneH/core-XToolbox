#if USE_V8_ENGINE


#ifndef V4D__CONTEXT__H__
#define V4D__CONTEXT__H__

#include <v8.h>
#include <JS4D.h>

BEGIN_TOOLBOX_NAMESPACE


class V4DContext: public XBOX::VObject
{
public:
								V4DContext(v8::Isolate* isolate,XBOX::JS4D::ClassRef inGlobalClassRef);
								~V4DContext();

		bool					EvaluateScript(const XBOX::VString& inScript, const VString& inUrl, VJSValue *outResult);
		bool					CheckScriptSyntax(const XBOX::VString& inScript, const VString& inUrl);
		bool					HasDebuggerAttached();
		void					AttachDebugger();

		void					GetSourceData(sLONG inSourceID, XBOX::VString& outUrl);
		bool					GetSourceID(const XBOX::VString& inUrl, int& outID);

		v8::Persistent<v8::FunctionTemplate>*	GetImplicitFunctionConstructor(const xbox::VString& inName);

static	void 					AddNativeFunction( const xbox::JS4D::StaticFunction& inStaticFunction );

static	void 					AddNativeClass( xbox::JS4D::ClassDefinition* inClassDefinition );

static	void 					AddNativeObjectTemplate(
							const xbox::VString& inName,
							v8::Handle<v8::ObjectTemplate>& inObjTemp );




		v8::Handle<v8::Object>		GetFunction(const char* inClassName);

		v8::Handle<v8::Function>	CreateFunctionTemplateForClass(	JS4D::ClassRef	inClassRef,
																	bool			inLockResources = true,
																	bool			inAddPersistentConstructor = false );

		v8::Handle<v8::Function>	CreateFunctionTemplateForClassConstructor(
											JS4D::ClassRef inClassRef,
											const JS4D::ObjectCallAsFunctionCallback inConstructorCallback,
											void* inData,
											bool inAcceptCallAsFunction = false,
											JS4D::StaticFunction inContructorFunctions[] = NULL);


		void*					GetGlobalObjectPrivateInstance();
		void					SetGlobalObjectPrivateInstance(void* inPrivateData);

		void*					GetObjectPrivateData(v8::Value* inObject);
		void					SetObjectPrivateData(v8::Value* inObject, void* inPrivateData);
	
static 	v8::Persistent< v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >*		GetPersistentContext(v8::Isolate* inIsolate);
static	void*					GetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject);
static	void					SetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject, void* inPrivateData);
static	void					DualCallFunctionHandler(const v8::FunctionCallbackInfo<v8::Value>& info);

		v8::Isolate*			GetIsolate() { return fIsolate; }

		v8::Handle<v8::Function>		AddCallback(JS4D::ObjectCallAsFunctionCallback inFunctionCallback);
		v8::Persistent<v8::Object>*		GetStringifyFunction() { return fStringify; }

		xbox::VString			GetCurrentScriptUrl();
private:
	class VChromeDebugContext;

	bool																		fIsOK;
	v8::Isolate* 																fIsolate;
	xbox::JS4D::ClassDefinition* 												fGlobalClassRef;
	v8::Persistent <v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >	fPersistentCtx;
	std::map< xbox::VString , v8::Persistent<v8::FunctionTemplate>* >			fPersImplicitFunctionConstructorMap;
	std::vector<v8::Persistent<v8::FunctionTemplate>*>							fCallbacksVector;
	v8::Persistent<v8::Object>*													fStringify;
	//std::stack<xbox::VString>													fUrlStack;
	xbox::JS4D::ClassDefinition*												fDualCallClass;
	VChromeDebugContext*														fDebugContext;
	std::map< int, v8::Persistent<v8::Script>* >								fScriptsMap;
	std::map< xbox::VString, int >												fScriptIDsMap;

		void						UpdateContext();
		void						AddNativeFunctionTemplate(	const xbox::VString& inName,
																v8::Handle<v8::FunctionTemplate>& inFuncTemp);

static	bool 						CheckIfNew( const XBOX::VString& inName );
static	void						Print(const v8::FunctionCallbackInfo<v8::Value>& args);
static	void						SetAccessors(v8::Handle<v8::ObjectTemplate>& inInstance, JS4D::ClassDefinition* inClassDef);
static	void						GetPropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
static	void						SetPropHandler(v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
static	void						EnumPropHandler(const v8::PropertyCallbackInfo<v8::Array>& info);
static	void						QueryPropHandler(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Integer>& inInfo);
static	void						IndexedGetPropHandler(uint32_t inIndex, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
static	void						IndexedSetPropHandler(uint32_t inIndex, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
static void							GetPropertyCallback(v8::Local<v8::String> inProperty, const v8::PropertyCallbackInfo<v8::Value>& inInfo);
static void							SetPropertyCallback(v8::Local<v8::String> inProperty, v8::Local<v8::Value> inValue, const v8::PropertyCallbackInfo<void>& inInfo);

		VChromeDebugContext*		GetDebugContext();

static	XBOX::VCriticalSection											sLock;
static	std::map< xbox::VString, xbox::JS4D::ClassDefinition >			sNativeClassesMap;


};
#endif

END_TOOLBOX_NAMESPACE

#endif

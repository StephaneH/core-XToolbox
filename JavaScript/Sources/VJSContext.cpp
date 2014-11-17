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
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "JS4D.h"
#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSJSON.h"
#include "VJSModule.h"

BEGIN_TOOLBOX_NAMESPACE

#if USE_V8_ENGINE
#if	V8_USE_MALLOC_IN_PLACE
VJSException::VJSException()
{
	//new (&fException) v8::Persistent<v8::Value>();
	fException = new PersistentValue();
}

VJSException::VJSException(const VJSException& inOther) : fException(inOther.fException)
{
	v8::Isolate*	curtIsolate = v8::Isolate::GetCurrent();
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(curtIsolate);
	HandleScope scope(curtIsolate);
	Handle<Context> ctx = Handle<Context>::New(curtIsolate, *v8PersContext);
	Context::Scope context_scope(ctx);

	//new (&fException) v8::Persistent<v8::Value>(curtIsolate, ToV8Persistent(inOther.fException));
	fException = inOther.fException;
	fException->Retain();
}

VJSException& VJSException::operator=(const VJSException& inOther)
{
	v8::Isolate*	curtIsolate = v8::Isolate::GetCurrent();
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(curtIsolate);
	HandleScope scope(curtIsolate);
	Handle<Context> ctx = Handle<Context>::New(curtIsolate, *v8PersContext);
	Context::Scope context_scope(ctx);

	//ToV8Persistent(fException).Reset(curtIsolate, ToV8Persistent(inOther.fException));
	ReleaseRefCountable(&fException);
	fException = inOther.fException;
	fException->Retain();

	return *this;
}

VJSException::~VJSException()
{
	//ToV8Persistent(fException).~Persistent();
	ReleaseRefCountable(&fException);
}

void VJSException::Clear()
{
	//ToV8Persistent(fException).Reset();
	ReleaseRefCountable(&fException);
}

bool VJSException::IsEmpty() const
{ 
	return ToV8Persistent(fException).IsEmpty() || (*ToV8Persistent(fException))->IsUndefined();
}

#else
VJSException::VJSException() : fException(NULL)	
{
}

VJSException::VJSException(const VJSException& inOther) : fException(inOther.fException)
{
}

VJSException& VJSException::operator=(const VJSException& inOther)
{ 
	fException = inOther.fException;
	return *this;
}

VJSException::~VJSException()
{
}


void VJSException::Clear()
{
	fException = NULL;
}

bool VJSException::IsEmpty() const
{
	return true;
}
#endif

void VJSException::SetValue(JS4D::ContextRef inContext,const v8::Handle<v8::Value>& inExceptionValue)
{
#if	V8_USE_MALLOC_IN_PLACE
	ToV8Persistent(fException).Reset(inContext,inExceptionValue);
#else
#endif
}
#if	V8_USE_MALLOC_IN_PLACE
JS4D::ValueRef VJSException::GetValue() const
{
	return fException;
}
#endif

#endif

static bool LoadScriptFile( VFile *inFile, VString& outScript)
{
	VFileStream stream( inFile);
	VError err = stream.OpenReading();
	if (err == VE_OK)
	{
		err = stream.GuessCharSetFromLeadingBytes( VTC_DefaultTextExport );		// sc 18/03/2011 instead of VTC_UTF_8
		stream.SetCarriageReturnMode( eCRM_NATIVE );
		if (err == VE_OK)
			err = stream.GetText( outScript);
	}
	stream.CloseReading();
	return err == VE_OK;
}



VJSContext::VJSContext( const VJSGlobalContext *inGlobalContext)
: fGlobalContext( RetainRefCountable( inGlobalContext))
, fContext( inGlobalContext->Use())
, fDebuggerAllowed(true)
{
}


VJSContext::~VJSContext()
{
	if (fGlobalContext != NULL)
	{
		fGlobalContext->Unuse();
		fGlobalContext->Release();
	}
}

VJSContext& VJSContext::operator=( const VJSContext& inOther)
{
	fContext = inOther.fContext;
	fGlobalContext = RetainRefCountable( inOther.fGlobalContext);
	if (fGlobalContext != NULL)
	{
		fGlobalContext->Use();
	}
	return *this;
}


bool VJSContext::EvaluateScript( VFile *inFile, VJSValue *outResult, VJSException* outException, VJSObject* inThisObject) const
{
	VString script;
	bool ok = LoadScriptFile(inFile, script);

	if (ok)
	{
		VURL url(inFile->GetPath());
		ok = EvaluateScript(script, &url, outResult, outException, inThisObject);
	}

	return ok;
}

bool VJSContext::EvaluateScript(const VString& inScript, const VURL *inSource, VJSValue *outResult, VJSException* outException, VJSObject* inThisObject, bool inCacheScript) const
{

	bool ok = false;
#if USE_V8_ENGINE
	VString	urlString = K_EMPTY_URL;
	if (inSource)
	{
		inSource->GetAbsoluteURL(urlString,true);
	}
	//DebugMsg("VJSContext::EvaluateScript called for <%S>\n",&urlString );
	Isolate*	isolate = Isolate::GetCurrent();
	xbox_assert( isolate == fContext);
	V4DContext* v8Ctx = (V4DContext*)isolate->GetData(0);

	bool	attachDebuggerOnContext = false;

	if (!v8Ctx->HasDebuggerAttached())
	{
		VJSGlobalContext::sWAKDebuggerServerLock.Lock();
		if (VJSGlobalContext::sWAKDebuggerServer)
		{
			if (VJSGlobalContext::sWAKDebuggerServer->GetType() == WEB_INSPECTOR_TYPE)
			{
				if (VJSGlobalContext::sWAKDebuggerServer->HasClients())
				{
					attachDebuggerOnContext = true;
				}
			}
		}
		VJSGlobalContext::sWAKDebuggerServerLock.Unlock();
	}

	if (attachDebuggerOnContext)
	{
		v8Ctx->AttachDebugger();
	}

	ok = v8Ctx->EvaluateScript(inScript, urlString, outResult, inCacheScript, outException);

#else
	VJSGlobalObject	*globalObject = GetGlobalObjectPrivateInstance();

	// globalObject might be NULL when used on a WebView context.

	JSStringRef jsUrl;
	if (inSource != NULL)
	{
		VString url;
		inSource->GetAbsoluteURL( url, true);
		/*if (url.Find("storage"))
		{
			int a = 1;
		}*/
		jsUrl = JS4D::VStringToString( url);
	}
	else
	{
		jsUrl = NULL;	
	}
	
	VJSException exception;
    JSStringRef jsScript = JS4D::VStringToString( inScript);
	int nStartingLineNumber = fDebuggerAllowed ? 0 : -1;
	JSValueRef result = JS4DEvaluateScript(fContext, jsScript, inThisObject == NULL ? NULL : inThisObject->GetObjectRef()/*thisObject*/, jsUrl, nStartingLineNumber, VJSException::GetExceptionRefIfNotNull(&exception));
    JSStringRelease( jsScript);
	
	if (jsUrl != NULL)
		JSStringRelease( jsUrl);
	
	ok = (result != NULL) && (exception.GetValue() == NULL);
	
	if (outResult != NULL)
	{
		*outResult = VJSValue(fContext,result);
	}

	if (outException != NULL)
	{
		*outException = exception;
	}
#endif
	return ok;
}



bool VJSContext::CheckScriptSyntax (const VString &inScript, const VURL *inSource, VJSException *outException) const
{
	bool		ok = false;
#if USE_V8_ENGINE
	Isolate*	isolate = Isolate::GetCurrent();
	xbox_assert( isolate == fContext);
	V4DContext* 	v8Ctx = (V4DContext*)isolate->GetData(0);
	VString	urlString = K_EMPTY_URL;
	if (inSource)
	{
		inSource->GetAbsoluteURL(urlString,true);
	}

	ok = v8Ctx->CheckScriptSyntax(inScript,urlString);

#else
	JSStringRef jsUrl;
	VString		url;

	jsUrl = NULL;
	if (inSource != NULL) {

		inSource->GetAbsoluteURL(url, true);
		jsUrl = JS4D::VStringToString(url);

	}
	
    JSStringRef jsScript; 

	jsScript = JS4D::VStringToString(inScript);
	ok = JSCheckScriptSyntax(fContext, jsScript, jsUrl, 0, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease(jsScript);

	if (jsUrl != NULL)

		JSStringRelease(jsUrl);

#endif
	return ok;
}


bool VJSContext::GetFunction(const VString& inFunctionRef, VJSObject& outFuncObj)
{
	bool found = false;
	VJSValue result(*this);

	EvaluateScript(inFunctionRef, nil, &result, nil, nil);

	if (result.IsObject())
	{
		VJSObject funcobj(result.GetObject());
		if (funcobj.IsFunction())
		{
			outFuncObj = funcobj;
			found = true;
		}
	}
	return found;
}



VJSObject VJSContext::MakeFunction( const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, VJSException *outException)
{
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
#else
	VJSContext	vjsContext(fContext);
	return VJSObject(
			vjsContext,
			JS4D::MakeFunction(fContext, inName, inParamNames, inBody, inUrl, inStartingLineNumber, VJSException::GetExceptionRefIfNotNull(outException)));
#endif
}

VJSGlobalObject *VJSContext::GetGlobalObjectPrivateInstance() const
{
#if USE_V8_ENGINE
	V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData(0);
	return static_cast<VJSGlobalObject*>(v8Ctx->GetGlobalObjectPrivateInstance());
#else
	return static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)));
#endif
}


VJSObject VJSContext::GetGlobalObject() const
{
#if USE_V8_ENGINE
	if (!testAssert(fContext == v8::Isolate::GetCurrent()))
	{
		DebugMsg("VJSContext::GetGlobalObject() bad ctx=%x / %x  taskID=%d\n",
			fContext,
			v8::Isolate::GetCurrent(),
			VTask::GetCurrent()->GetID());
	}
	/*else
	{
		DebugMsg("VJSContext::GetGlobalObject() OK ctx=%x / %x  taskID=%d\n",
			fContext,
			v8::Isolate::GetCurrent(),
			VTask::GetCurrent()->GetID());
	}*/
	HandleScope handle_scope(fContext);
	V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData(0);
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(fContext);
	Handle<Context> context = Handle<Context>::New(fContext,*v8PersContext);
	Handle<Object> global = context->Global();

	return VJSObject( fContext, global );
#else
	VJSContext	vjsContext(fContext);
	return VJSObject( vjsContext, JSContextGetGlobalObject( fContext));
#endif
}

#if USE_V8_ENGINE
VString VJSContext::GetCurrentScriptUrl() const
{
	V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData(0);
	return v8Ctx->GetCurrentScriptUrl();
}
#endif

void VJSContext::EndOfNativeCodeExecution() const
{
	VJSGlobalObject *privateInstance = GetGlobalObjectPrivateInstance();
	if (privateInstance != NULL)
		privateInstance->GarbageCollectIfNecessary();
}


void VJSContext::GarbageCollect() const
{
	GetGlobalObjectPrivateInstance()->GarbageCollect();
}


UniChar VJSContext::GetWildChar() const
{ 
	return fGlobalContext->GetWildChar(); 
}


//======================================================


VJSGlobalContext::~VJSGlobalContext()
{
	if (fContext != NULL) {
#if USE_V8_ENGINE
		bool isSameIsolate = (fContext == v8::Isolate::GetCurrent());
		if (!testAssert(isSameIsolate))
		{
			int a = 1;
		}
		DebugMsg(
			"VJSGlobalContext::~VJSGlobalContext exit from ctx=%x sameIsolate=%d taskID=%d\n",
			fContext,
			isSameIsolate,
			VTask::GetCurrent()->GetID());

		V4DContext* v8Ctx = (V4DContext*)fContext->GetData(0);
		if (testAssert(v8Ctx))
		{
			JS4D::ClassDefinition	classDef;
			VJSGlobalClass::GetDefinition(classDef);
			v8Ctx->DisposeGloBalObjectPrivateInstance(classDef);
		}
		/* seems useless in out case*/
		while (!V8::IdleNotification())
		{
			int a = 1;
		};

		if (v8Ctx)
		{
			delete v8Ctx;
		}
		fContext->Dispose();
		fContext = NULL;
#else
	// Pending fix. (replace with xbox_assert(fContext != NULL)).


		// require() function object is protected from garbage collection.
		// Unprotect it before the global context is released.

		XBOX::VJSContext	context(fContext);
		XBOX::VJSObject		globalObject(context.GetGlobalObject());
		XBOX::VJSObject		requireObject(context);
		
		requireObject = globalObject.GetPropertyAsObject("require");
		if (requireObject.IsOfClass(VJSRequireClass::Class())) {

			VJSModuleState	*moduleState;

			moduleState = requireObject.GetPrivateData<VJSRequireClass>();
			xbox_assert(moduleState);

			moduleState->GetRequireFunction().Unprotect();

		}
	
		JSGlobalContextRelease(fContext);
#endif

	}
}


/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate)
{
#if 0
DebugMsg("VJSGlobalContext::Create called\n");
#endif

	return VJSGlobalContext::Create( inRuntimeDelegate, VJSGlobalClass::Class());
}


VJSGlobalContext::VJSGlobalContext(JS4D::GlobalContextRef inGlobalContext)
: fContext(inGlobalContext)
{
}

#define V8_TRACE_MEM_ALLOCS 0
#if V8_TRACE_MEM_ALLOCS
static void MemAllocCallback(v8::ObjectSpace inSpace, v8::AllocationAction inAllocAction, int size)
{
	bool display = false;
	//static char	trace[256];
	static XBOX::VCriticalSection	mtx;
	static sLONG8	totalBytes = 0;
	static sLONG8	prevTotalBytes = 0;
	mtx.Lock();

	switch (inAllocAction)
	{
	case v8::kAllocationActionAllocate:
		totalBytes += size;
		break;
	case v8::kAllocationActionFree:
		totalBytes -= size;
		break;
	default:
		xbox_assert(false);
	}
	display = (totalBytes > (prevTotalBytes + 1000000));
	if (display)
	{
		//_snprintf(trace, 255, "MemAllocCallback size=%lld\n", totalBytes);
		DebugMsg("MemAllocCallback size=%lld\n", totalBytes);
		prevTotalBytes = totalBytes;
	}
	mtx.Unlock();

}
#endif
/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate, JS4D::ClassRef inGlobalClassRef)
{
#if USE_V8_ENGINE
	static v8::Isolate*		sDefaultIsolate = NULL;
	if (!sDefaultIsolate)
	{
#if V8_TRACE_MEM_ALLOCS
		//V8::AddMemoryAllocationCallback(MemAllocCallback, v8::kObjectSpaceAll, kAllocationActionAll);
#endif
		sDefaultIsolate = v8::Isolate::GetCurrent();
		DebugMsg("VJSGlobalContext::Create2 sDefaultIsolate=%x\n", sDefaultIsolate);
	}
	Isolate*	ref = Isolate::New();
	//v8::ResourceConstraints rc;
	//rc.ConfigureDefaults(16 * 1024 * 1024 * 1024, 8);
	//uint32_t*	stackLimit = new uint32_t;
	//*stackLimit = 8000000;
	//rc.set_stack_limit(stackLimit);
	//v8::SetResourceConstraints(ref, &rc);
	if (testAssert(ref))
	{

		DebugMsg("VJSGlobalContext::Create2 called newCtx=%x classRef='%s' taskID=%d\n",
			ref,
			((JS4D::ClassDefinition*)inGlobalClassRef)->className,
			VTask::GetCurrent()->GetID());

		V4DContext*			v8Ctx = new V4DContext(ref,inGlobalClassRef);
		VJSGlobalObject*	globalObject = new VJSGlobalObject( ref, NULL, inRuntimeDelegate);
		v8Ctx->SetGlobalObjectPrivateInstance(globalObject);

		HandleScope handle_scope(ref);
		Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(ref);
		Handle<Context> context = Handle<Context>::New(ref, *v8PersContext);
		Context::Scope context_scope(context);
		Local<Object> global = context->Global();

		VJSGlobalContext*	newCtx = new VJSGlobalContext(ref);
		VJSContext jsContext(newCtx);
		VJSObject	globObj(jsContext, global);
		((JS4D::ClassDefinition*)inGlobalClassRef)->initialize(ref, globObj);
		return newCtx;
#else
	JSGlobalContextRef ref = JSGlobalContextCreateInGroup( NULL, inGlobalClassRef);
	if (ref != NULL)
	{
		VJSGlobalObject *globalObject = new VJSGlobalObject( ref, NULL, inRuntimeDelegate);
		JSObjectSetPrivate( JSContextGetGlobalObject( ref), globalObject);

		return new VJSGlobalContext( ref);

#endif
	}
	else
		return NULL;
}
#if CHECK_JSCONTEXT_THREAD_COHERENCY
bool VJSContext::Check(bool inDisplay) const
{
	bool isOk = false;
#if USE_V8_ENGINE
	isOk = testAssert(fContext == v8::Isolate::GetCurrent());
#else
	VJSGlobalObject *globObj = static_cast<VJSGlobalObject*>(JSObjectGetPrivate(JSContextGetGlobalObject(fContext)));
	if (testAssert(globObj))
	{
		isOk = globObj->Check(inDisplay);
	}
#endif
	return isOk;
}
#endif

/*
	static
*/
void VJSGlobalContext::SetSourcesRoot ( const VFolder & inRootFolder )
{
	VFilePath		vfPathRoot;
	inRootFolder. GetPath ( vfPathRoot );
	
	VString			vstrRoot;
	vfPathRoot. GetPosixPath ( vstrRoot );
#if USE_V8_ENGINE
	//xbox_assert(false);

#else
    JSStringRef		jsstrRoot = JS4D::VStringToString ( vstrRoot );
	if ( jsstrRoot != 0 )
	{
		JSSetSourcesRoot ( jsstrRoot );
		JSStringRelease ( jsstrRoot );
	}
#endif
}

bool VJSGlobalContext::IsDebuggerPaused ( )
{
#if USE_V8_ENGINE
	xbox_assert(false);
	return false;
#else
	return JSIsDebuggerPaused ( );
#endif
}

bool VJSGlobalContext::IsDebuggerActive ( )
{
#if USE_V8_ENGINE
	//xbox_assert(false);
	return false;
#else
	return JSIsDebuggerActive ( );
#endif
}

void VJSGlobalContext::AbortAllDebug ( )
{
#if USE_V8_ENGINE
	int a = 1;
#else
	JSAbortAllDebug ( );
#endif
}

void VJSGlobalContext::AllowDebuggerLaunch ( )
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else
	JSAllowDebuggerLaunch ( );
#endif
}

void VJSGlobalContext::ForbidDebuggerLaunch ( )
{
#if USE_V8_ENGINE
	xbox_assert(false);
#else
	JSForbidDebuggerLaunch ( );
#endif
}

IRemoteDebuggerServer*		VJSGlobalContext::sWAKDebuggerServer = NULL;
#if USE_V8_ENGINE
IChromeDebuggerServer*		VJSGlobalContext::sChromeDebuggerServer = NULL;
#endif
XBOX::VCriticalSection		VJSGlobalContext::sWAKDebuggerServerLock;

void VJSGlobalContext::SetDebuggerServer(	IWAKDebuggerServer*		inDebuggerServer,
											IChromeDebuggerServer*	inChromeDebuggerServer,
											IRemoteDebuggerServer*	inNoDebugger)
{
#if USE_V8_ENGINE
	sWAKDebuggerServerLock.Lock();
	if (!testAssert(inDebuggerServer == NULL))
	{
		DebugMsg("VJSGlobalContext::SetDebuggerServer wakandaDebugger not allowed with V8\n");
	}
	else
	{
		/*if (sWAKDebuggerServer)
		{
			if (sDebuggerServer->GetType() == REGULAR_DBG_TYPE)
			{
				WAKDebugger::AbortAll();//sDebuggerServer->StopServer();
			}
		}*/
		if (inChromeDebuggerServer)
		{
			sWAKDebuggerServer = (IRemoteDebuggerServer*)inChromeDebuggerServer;// dynamic_cast<IRemoteDebuggerServer *>(inChromeDebuggerServer);
			sChromeDebuggerServer = inChromeDebuggerServer;
		}
		else
		{
			sWAKDebuggerServer = inNoDebugger;
			sChromeDebuggerServer = NULL;
		}
	}
	sWAKDebuggerServerLock.Unlock();
#else
	
	sWAKDebuggerServerLock.Lock();
	JS4DSetDebuggerServer( inDebuggerServer,inChromeDebuggerServer, inNoDebugger);
	sWAKDebuggerServer =
		(inDebuggerServer ?
			(IRemoteDebuggerServer*)inDebuggerServer :
			(inChromeDebuggerServer ? (IRemoteDebuggerServer*)inChromeDebuggerServer : inNoDebugger ) );
	sWAKDebuggerServerLock.Unlock();
#endif
}

IRemoteDebuggerServer* VJSGlobalContext::GetDebuggerServer( )
{
	IRemoteDebuggerServer*	result = NULL;
#if USE_V8_ENGINE

#else
#endif		

	sWAKDebuggerServerLock.Lock();
	result = sWAKDebuggerServer;
	sWAKDebuggerServerLock.Unlock();

	return result;
}


JS4D::ContextRef VJSGlobalContext::Use() const
{
#if USE_V8_ENGINE
	V4DContext* v8Ctx = (V4DContext*)(fContext->GetData(0));
	VJSGlobalObject*	gblObj = (VJSGlobalObject*)v8Ctx->GetGlobalObjectPrivateInstance();
	gblObj->UseContext();
#else
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UseContext();
#endif
	return fContext;
}


void VJSGlobalContext::Unuse() const
{
#if USE_V8_ENGINE
	V4DContext* v8Ctx = (V4DContext*)(fContext->GetData(0));
	VJSGlobalObject*	gblObj = (VJSGlobalObject*)v8Ctx->GetGlobalObjectPrivateInstance();
	gblObj->UnuseContext();

	int a=1;
#else
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UnuseContext();
#endif
}
void VJSGlobalContext::Enter()
{
#if USE_V8_ENGINE
	V4DContext* v8Ctx = (V4DContext*)(fContext->GetData(0));
	v8Ctx->Enter();
	//VJSGlobalObject*	gblObj = (VJSGlobalObject*)v8Ctx->GetGlobalObjectPrivateInstance();
#endif
	int a = 1;
}
void VJSGlobalContext::Exit()
{
#if USE_V8_ENGINE
	V4DContext* v8Ctx = (V4DContext*)(fContext->GetData(0));
	//VJSGlobalObject*	gblObj = (VJSGlobalObject*)v8Ctx->GetGlobalObjectPrivateInstance();
	v8Ctx->Exit();
#endif
	int a = 1;
}

UniChar VJSGlobalContext::GetWildChar() const
{
	// right now only use the VIntlMgr set in the task
	VIntlMgr* intl = VTask::GetCurrentIntlManager();
	if (intl == nil)
		return '*';
	else
		return intl->GetCollator()->GetWildChar();
}



bool VJSGlobalContext::EvaluateScript( VFile *inFile, VValueSingle **outResult, bool inJSONresult ) const
{
	VString script;
	bool ok = LoadScriptFile( inFile, script);
	if (ok)
	{
		VURL url( inFile->GetPath());
		ok = EvaluateScript( script, &url, outResult, inJSONresult );
	}

	return ok;
}


bool VJSGlobalContext::EvaluateScript(const VString& inScript, const VURL *inSource, VValueSingle **outResult, bool inJSONresult, bool inCacheScript) const
{
	VJSContext context( fContext);

	Use();

	VJSValue result(context);
	VJSException exception;

	bool ok = context.EvaluateScript(inScript, inSource, &result, &exception, NULL, inCacheScript);
	/*if (!ok)
	{
		int a = 1;
	}*/
	// first throw an error for javascript if necessary
	if (exception.ThrowVErrorForException(fContext))
	{
		xbox_assert( !ok);
		// then rethrow an error to explain wat we were doing
		VTask::GetCurrent()->GetDebugContext().DisableUI();
		{
			VString url;
			if (inSource != NULL)
				inSource->GetAbsoluteURL( url, false);
			if (!url.IsEmpty())
			{
				StThrowError<>	error( MAKE_VERROR( 'JS4D', 2));
				error->SetString( "url", url);
				error->SetString( "error_description", CVSTR( "Error evaluating javascript {url}"));
			}
			else
			{
				StThrowError<>	error( MAKE_VERROR( 'JS4D', 3));
				error->SetString( "error_description", CVSTR( "Error evaluating javascript"));
			}
		}
		VTask::GetCurrent()->GetDebugContext().EnableUI();
		if (outResult != NULL)
			*outResult = NULL;
	}
	else
	{
		xbox_assert( ok);
		if (outResult != NULL)
		{
			if ( inJSONresult )
			{
				VJSJSON stringifier( context );
				VString* ptStr = new VString;
				// The "result" value's fValue is initialized to 0 when a script is aborted using the JS debugger. In this case js core crashes while stringifying, thus the if-else
				// The if-else can be moved directly into ::Stringify if "null" is acceptable as a generic return value
				if ( result. IsNull ( ) )
					ptStr-> AppendCString ( "null" );
				else
					stringifier.Stringify( result, *ptStr );

				*outResult = ptStr;
			}
			else
				*outResult = result.CreateVValue(false);
		}
	}

	Unuse();
	
	return ok;
}

END_TOOLBOX_NAMESPACE

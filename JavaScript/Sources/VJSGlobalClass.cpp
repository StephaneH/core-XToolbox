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

#if  USE_V8_ENGINE
#include "V4DContext.h"

#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSRuntime_file.h"
#include "VJSRuntime_stream.h"
#include "VJSRuntime_progressIndicator.h"
#include "VJSRuntime_Image.h"
#include "VJSRuntime_Atomic.h"
#include "VJSRuntime_blob.h"
#include "VJSGlobalClass.h"
#if WITH_NATIVE_HTTP_BACKEND
#include "VXMLHttpRequest.h"
#endif
#include "VcURLXMLHttpRequest.h"
#include "VJSEndPoint.h"
#include "VJSSystemWorker.h"
#include "VJSEvent.h"
#include "VJSWorker.h"
#include "VJSWorkerProxy.h"
#include "VJSWebStorage.h"
#include "VJSProcess.h"

#include "VJSMysqlBuffer.h"
#include "VJSTLS.h"
#include "VJSW3CFileSystem.h"
#include "VJSBuffer.h"
#include "VJSNet.h"
#include "VJSCrypto.h"
#include "VJSModule.h"
#include "VJSEventEmitter.h"
#include "VJSWebSocket.h"
#include "VJSMIME.h"

#include "VJSNetSocket.h"
#include "VJSNetServer.h"

USING_TOOLBOX_NAMESPACE

const VString		K_REFRESH_STR = CVSTR("refresh");
const VString		K_AUTO_STR = CVSTR("auto");
const VString		K_SYSTEM_STR = CVSTR("system");
const VString		K_USER_STR = CVSTR("user");
const VString		K_RELATIVE_STR = CVSTR("relative");
const VString		K_CAN_INTERRUPT_STR = CVSTR("Can Interrupt");
const VString		K_CANNOT_INTERRUPT_STR = CVSTR("Cannot Interrupt");


VJSSpecifics::~VJSSpecifics()
{
	for( MapOfValues::iterator i = fMap.begin() ; i != fMap.end() ; ++i)
	{
		if (i->second.first != NULL && i->second.second != NULL)
			i->second.second( i->second.first);
	}
}


void *VJSSpecifics::Get( key_type inKey) const
{
	MapOfValues::const_iterator i = fMap.find( inKey);
	return (i != fMap.end()) ? i->second.first : NULL;
}


bool VJSSpecifics::Set( key_type inKey, void *inValue, value_destructor_type inDestructor)
{
	bool ok = true;
	MapOfValues::iterator i = fMap.find( inKey);
	if (i != fMap.end())
	{
		if (i->second.first != inValue)
			i->second.second( i->second.first);
		i->second.first = inValue;
		i->second.second = inDestructor;
	}
	else
	{
		MapOfValues::mapped_type value( inValue, inDestructor);
		fMap.insert( MapOfValues::value_type( inKey, value));
	}
	return ok;
}


//======================================================


XBOX::VFileSystemNamespace* IJSRuntimeDelegate::RetainRuntimeFileSystemNamespace()
{
	return RetainRefCountable( VProcess::Get()->GetFileSystemNamespace());
}

/*XBOX::VSystemWorkerNamespace* IJSRuntimeDelegate::RetainRuntimeSystemWorkerNamespace()
{
	xbox_assert ( false );

	return NULL;
}*/

bool IJSRuntimeDelegate::CanAccessAllFiles()
{
	return true;
}


void IJSRuntimeDelegate::SetRuntimeSpecific( const XBOX::VString& inSignature, XBOX::IRefCountable *inObject)
{
	VTaskLock lock( &fMutex);
	fSpecifics[inSignature] = inObject;
}


XBOX::IRefCountable *IJSRuntimeDelegate::RetainRuntimeSpecific( const XBOX::VString& inSignature) const
{
	VTaskLock lock( &fMutex);
	MapOfSpecifics::const_iterator i = fSpecifics.find( inSignature);
	return (i == fSpecifics.end()) ? NULL : i->second.Retain();
}

XBOX::VError IJSRuntimeDelegate::AddWebSocketHandler (XBOX::VJSContext &inContext, const XBOX::VString &inPath, WebSocketHandler *inHandler, void *inUserData)
{
	xbox_assert(!inPath.IsEmpty() && inHandler != NULL && inUserData != NULL);

	xbox_assert(false);				// Only to be implemented by VRIAServerProjectJSRuntimeDelegate.
	return XBOX::VE_UNIMPLEMENTED;	
}

XBOX::VError IJSRuntimeDelegate::RemoveWebSocketHandler (XBOX::VJSContext &inContext, const XBOX::VString &inPath)
{
	xbox_assert(!inPath.IsEmpty());

	xbox_assert(false);				// Only to be implemented by VRIAServerProjectJSRuntimeDelegate.
	return XBOX::VE_UNIMPLEMENTED;	
}
	

//======================================================


VJSGlobalObject::VJSGlobalObject( JS4D::GlobalContextRef inContext, VJSContextGroup *inContextGroup, IJSRuntimeDelegate *inRuntimeDelegate)
: fContext( inContext)
, fContextGroup( inContextGroup)
, fRuntimeDelegate( inRuntimeDelegate)
, fIncludedFiles( false)	// path matching without diacritics
, fGarbageCollectionPending( 0)
, fLastGarbageCollection( 0)
, fContextUseTaskID( NULL_TASK_ID)
, fContextUseCount( 0)
{
#if CHECK_JSCONTEXT_THREAD_COHERENCY
	Enter();
#endif
}


VJSGlobalObject::~VJSGlobalObject()
{
#if CHECK_JSCONTEXT_THREAD_COHERENCY
	Exit();
#endif
}


VJSGlobalObject::operator VJSObject() const
{
	VJSContext	vjsContext(fContext);
#if USE_V8_ENGINE
	xbox_assert(false);
	return VJSObject( vjsContext );
#else
	return VJSObject( vjsContext, JSContextGetGlobalObject( fContext));
#endif
}


bool VJSGlobalObject::RegisterIncludedFile( VFile *inFile)
{
	bool newlyRegistered;
	
	if (inFile != NULL)
	{
		VString path;
		inFile->GetPath( path, FPS_POSIX);

		std::pair<MapOfIncludedFile::const_iterator, bool> result = fIncludedFiles.insert( MapOfIncludedFile::value_type( path, inFile));
		if (result.second)
		{
			newlyRegistered = true;

			VTime modificationTime;
			inFile->GetTimeAttributes( &modificationTime);
			fIncludedFilesModificationTime.push_back( VectorOfFileModificationTime::value_type( inFile, modificationTime));
		}
		else
		{
			newlyRegistered = false;
		}
	}
	else
	{
		newlyRegistered = false;
	}
	return newlyRegistered;
}


bool VJSGlobalObject::IsAnIncludedFile( const VFile *inFile) const
{
	bool registered = false;
	
	if (inFile != NULL)
	{
		VString path;
		inFile->GetPath( path, FPS_POSIX);
	#if 0	// build fix MAC
		registered = (fIncludedFiles.find( path) != fIncludedFiles.end());
	#endif
	}
	return registered;
}


bool VJSGlobalObject::IsIncludedFilesHaveBeenChanged() const
{
	bool haveChanges = false;

	for (VectorOfFileModificationTime::const_iterator iter = fIncludedFilesModificationTime.begin() ; (iter != fIncludedFilesModificationTime.end()) && !haveChanges ; ++iter)
	{
		if (iter->first != NULL)
		{
			VTime modificationTime;
			iter->first->GetTimeAttributes( &modificationTime);
			haveChanges = (modificationTime.CompareTo( iter->second) != CR_EQUAL);
		}
	}
	return haveChanges;
}


void VJSGlobalObject::UseContext()
{
	VTaskLock lock( &fUseMutex);

	if (fContextUseTaskID == NULL_TASK_ID)
	{
		xbox_assert( fContextUseCount == 0);
		fContextUseTaskID = VTask::GetCurrentID();
		fContextUseCount = 1;

#if CHECK_JSCONTEXT_THREAD_COHERENCY
		Enter();
#endif
	}
	else if (testAssert( fContextUseTaskID == VTask::GetCurrentID()))	// can't be in use by another task
	{
		++fContextUseCount;
	}
}


void VJSGlobalObject::UnuseContext()
{
	GarbageCollectIfNecessary();

	{
		VTaskLock lock( &fUseMutex);

		xbox_assert( fContextUseTaskID == VTask::GetCurrentID());
		xbox_assert( fContextUseCount > 0);

		if (--fContextUseCount == 0)
		{
			fContextUseTaskID = NULL_TASK_ID;

#if CHECK_JSCONTEXT_THREAD_COHERENCY
			Exit();
#endif
		}
	}
}

bool VJSGlobalObject::TryToUseContext ()
{
	VTaskLock lock(&fUseMutex);

	if (fContextUseTaskID == NULL_TASK_ID) {
		
		xbox_assert(!fContextUseCount);
		fContextUseTaskID = VTask::GetCurrentID();
		fContextUseCount = 1;

		return true;

	} else 

		return false;
}

void VJSGlobalObject::GarbageCollect()
{
#if USE_V8_ENGINE
	//xbox_assert(false);
	V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData(0);
	fUseMutex.Lock();
	if (fContextUseTaskID == NULL_TASK_ID)
	{
		// no one is using the context.
		// one must keep the use mutex until the end of garbage collection.
		
		// do not garbageCollect when COntext not associated to thread
		//v8Ctx->CollectGarbage();
		//fLastGarbageCollection = fGarbageCollectionPending;
		fUseMutex.Unlock();
	}
	else if (VTask::GetCurrentID() == fContextUseTaskID)
	{
		// the context is being used by current thread.
		// we can safely garbage collect now.
		// release the mutex early so that the cache manager can call us without latency.
		fUseMutex.Unlock();
		fLastGarbageCollection = fGarbageCollectionPending;
		v8Ctx->CollectGarbage();
	}
	else
	{
		// deferred garbage collection until unuse or next c++ call from javascript
		AskDeferredGarbageCollection();
		fUseMutex.Unlock();
	}
#else
	fUseMutex.Lock();
	if (fContextUseTaskID == NULL_TASK_ID)
	{
		// no one is using the context.
		// one must keep the use mutex until the end of garbage collection.
		JSGarbageCollect( fContext);
		fLastGarbageCollection = fGarbageCollectionPending;
		fUseMutex.Unlock();
	}
	else if (VTask::GetCurrentID() == fContextUseTaskID)
	{
		// the context is being used by current thread.
		// we can safely garbage collect now.
		// release the mutex early so that the cache manager can call us without latency.
		fUseMutex.Unlock();
		fLastGarbageCollection = fGarbageCollectionPending;
		JSGarbageCollect( fContext);
	}
	else
	{
		// deferred garbage collection until unuse or next c++ call from javascript
		AskDeferredGarbageCollection();
		fUseMutex.Unlock();
	}
#endif
}


void VJSGlobalObject::GarbageCollectIfNecessary()
{
	if (testAssert(VTask::GetCurrentID() == fContextUseTaskID))
	{
		if (fLastGarbageCollection != fGarbageCollectionPending)
		{
			fLastGarbageCollection = fGarbageCollectionPending;
#if USE_V8_ENGINE
			V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData(0);
			v8Ctx->CollectGarbage();

#else
			JSGarbageCollect(fContext);
#endif
		}
	}
	else
	{
		/*static int count = 0;
		count++;
		int a = 1;*/
	}
}

//======================================================


VJSGlobalClass::VectorOfStaticFunction		VJSGlobalClass::sStaticFunctions;
VJSGlobalClass::VectorOfStaticValue			VJSGlobalClass::sStaticValues;
VJSGlobalClass::MapOfModuleConstructor		VJSGlobalClass::sModuleConstructors;

bool VJSGlobalClass::sGetDefinitionHasBeenCalled = false;

void VJSGlobalClass::Initialize( const VJSParms_initialize& inParms, VJSGlobalObject* inGlobalObject)
{
#if USE_V8_ENGINE
DebugMsg("VJSGlobalClass::Initialize called\n");
#else
	xbox_assert( inGlobalObject == NULL);	// cause there's one and only one global class instance, created after the context
#endif
	// VJSGlobalClass::CreateGlobalClasses() is not called before by Wakanda Studio.

	// Add Buffer, SystemWorker, Folder, File, and TextStream object constructors.

	XBOX::VJSContext	context(inParms.GetContext());
	XBOX::VJSObject		globalObject(inParms.GetObject());

	globalObject.SetProperty( CVSTR( "Blob"), JS4DMakeConstructor( context, VJSBlob::Class(), VJSBlob::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	globalObject.SetProperty(CVSTR("XMLHttpRequest"), VJSXMLHttpRequest::MakeConstructor(context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

#if WITH_NATIVE_HTTP_BACKEND
	//globalObject.SetProperty(CVSTR("cURLXMLHttpRequest"), VJScURLXMLHttpRequest::MakeConstructor(context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
#endif

	globalObject.SetProperty( CVSTR( "EndPoint"), JS4DMakeConstructor( context, VJSEndPoint::Class(), VJSEndPoint::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty( CVSTR( "MysqlBuffer"), JS4DMakeConstructor( context, VJSMysqlBufferClass::Class(), VJSMysqlBufferClass::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	globalObject.SetProperty( CVSTR( "Buffer"), VJSBufferClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 
	globalObject.SetProperty( CVSTR( "SystemWorker"), VJSSystemWorkerClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 	
	globalObject.SetProperty( CVSTR( "Folder"), VJSFolderIterator::MakeConstructor(context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 
	globalObject.SetProperty( CVSTR( "File"), VJSFileIterator::MakeConstructor( context), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 
	globalObject.SetProperty( CVSTR( "TextStream"), JS4DMakeConstructor( context, VJSTextStream::Class(), VJSTextStream::Construct,true ), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 

	globalObject.SetProperty( CVSTR( "MIMEWriter"), JS4DMakeConstructor( context, VJSMIMEWriter::Class(), VJSMIMEWriter::_Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly); 

	JS4DMakeConstructor(context, VJSEventEmitterClass::Class(), NULL);
	VJSEventEmitter	*eventEmitter = new VJSEventEmitter();
	if (eventEmitter != NULL)
	{
		JS4DMakeConstructor(context, VJSProcess::Class(), NULL);
		VJSObject processObject = VJSProcess::CreateInstance(context, eventEmitter);
		if (processObject.IsObject() && processObject.IsOfClass(VJSProcess::Class()))
		{
			globalObject.SetProperty(CVSTR("process"), processObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
		}
		else
		{
			int a = 1;
		}
	}
	ReleaseRefCountable( &eventEmitter);
	
	JS4DMakeConstructor(context, VJSOS::Class(), NULL);
	globalObject.SetProperty("os", VJSOS::CreateInstance(context, NULL), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	globalObject.SetProperty("Worker", VJSDedicatedWorkerClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete);
	globalObject.SetProperty("SharedWorker", VJSSharedWorkerClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete);

	globalObject.SetProperty("WebSocket", VJSWebSocketClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete);
	globalObject.SetProperty("WebSocketSync", VJSWebSocketSyncClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete);
	globalObject.SetProperty("WebSocketServer", VJSWebSocketServerClass::MakeConstructor(context), JS4D::PropertyAttributeDontDelete);

	VJSRequireClass::Class();
	// VJSRequireClass::MakeObject may return null object
	VJSObject requireObject( VJSRequireClass::MakeObject(context));
	if (requireObject.IsOfClass(VJSRequireClass::Class()))
	{

		globalObject.SetProperty("require", requireObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

		// WAK0082711:
		//
		// We are in a "direct" execution, not inside a require(), so we must have require.main === module. 
		// Create a dummy module object and set it as require.main.

		XBOX::VJSObject	moduleObject(context);

		moduleObject.MakeEmpty();
		globalObject.SetProperty("module", moduleObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
		requireObject.SetProperty("main", moduleObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	}

	// Add LocalFileSystem and LocalFileSystemSync interfaces support	
	VJSLocalFileSystem::AddInterfacesSupport(inParms.GetContext());

	// Register native C++ module.
	globalObject.SetProperty(CVSTR("Server"), JS4DMakeConstructor(inParms.GetContext(), VJSNetServerClass::Class(), VJSNetServerClass::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty(CVSTR("ServerSync"), JS4DMakeConstructor(inParms.GetContext(), VJSNetServerSyncClass::Class(), VJSNetServerSyncClass::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty(CVSTR("Socket"), JS4DMakeConstructor(inParms.GetContext(), VJSNetSocketClass::Class(), VJSNetSocketClass::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty(CVSTR("SocketSync"), JS4DMakeConstructor(inParms.GetContext(), VJSNetSocketSyncClass::Class(), VJSNetSocketSyncClass::Construct), JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	VJSNetClass::RegisterModule();				// "net"
	VJSTLSClass::RegisterModule();				// "tls"
	VJSCryptoClass::RegisterModule();			// "crypto"
	VJSEventEmitterClass::RegisterModule();		// "events"
	VJSMIMEReader::RegisterModule();			// "waf-mail-private-MIMEReader"
}

void VJSGlobalClass::Finalize( const VJSParms_finalize& inParms, VJSGlobalObject* inGlobalObject)
{
	inGlobalObject->Release();
}
	
	
void VJSGlobalClass::AddStaticFunction( const char *inName, JS4D::ObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes)
{
	xbox_assert( !sGetDefinitionHasBeenCalled);	// The global class has aready been created, its too late to add static values, 
	_CheckInitStaticFunctions();
	xbox_assert( !sStaticFunctions.empty());
	JS4D::StaticFunction fn = { inName, inCallBack, inAttributes};
	#if WITH_ASSERT
	// check the name is not already registered.
	for( VectorOfStaticFunction::const_iterator i = sStaticFunctions.begin() ; i != sStaticFunctions.end() ; ++i)
		xbox_assert( (i->name == NULL) || (strcmp( i->name, inName) != 0) );
	#endif
	sStaticFunctions.insert( sStaticFunctions.end() - 1, fn);	// leave trailing {0} marker at the end
}


void VJSGlobalClass::AddStaticValue( const char *inName, JS4D::ObjectGetPropertyCallback inGetCallback, JS4D::ObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes)
{
	xbox_assert( !sGetDefinitionHasBeenCalled);	// The global class has aready been created, its too late to add static values, 
	_CheckInitStaticValues();
	xbox_assert( !sStaticValues.empty());
	JS4D::StaticValue value = { inName, inGetCallback, inSetCallback, inAttributes};
	#if WITH_ASSERT
	// check the name is not already registered.
	for( VectorOfStaticValue::const_iterator i = sStaticValues.begin() ; i != sStaticValues.end() ; ++i)
		xbox_assert( (i->name == NULL) || (strcmp( i->name, inName) != 0) );
	#endif
	sStaticValues.insert( sStaticValues.end() - 1, value);	// leave trailing {0} marker at the end
}

 
/*
	static
*/
void VJSGlobalClass::RegisterModuleConstructor( const VString& inModuleName, ModuleConstructor inConstructor)
{
	try
	{
		sModuleConstructors.insert( MapOfModuleConstructor::value_type( inModuleName, inConstructor));
	}
	catch(...)
	{
	}
}

	
void VJSGlobalClass::_PushBackStaticFunction( const char *inName, JS4D::ObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes)
{
	JS4D::StaticFunction fn = { inName, inCallBack, inAttributes};
	sStaticFunctions.push_back( fn);
}
	
	
void VJSGlobalClass::_PushBackStaticValue( const char *inName, JS4D::ObjectGetPropertyCallback inGetCallback, JS4D::ObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes)
{
	JS4D::StaticValue value = { inName, inGetCallback, inSetCallback, inAttributes};
	sStaticValues.push_back( value);
}

		
void VJSGlobalClass::GetDefinition( ClassDefinition& outDefinition)
{
	_CheckInitStaticFunctions();
	_CheckInitStaticValues();
	outDefinition.className = "Component";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = &sStaticFunctions.at(0);
	outDefinition.staticValues = &sStaticValues.at(0);
	outDefinition.attributes = JS4D::ClassAttributeNoAutomaticPrototype;	// important because that's the global object class. Else JS will screw up in JSObjectHasProperty in infinite loop.
	sGetDefinitionHasBeenCalled = true;
}


void VJSGlobalClass::_CheckInitStaticFunctions()
{
	if (sStaticFunctions.empty())
	{
		//** TODO: Garder ? Mozilla propose dump() avec même comportement.
		_PushBackStaticFunction( "trace", js_callStaticFunction<do_trace>, JS4D::PropertyAttributeNone);

		//_PushBackStaticFunction( "testMe", js_callStaticFunction<do_testMe>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction( "include", js_callStaticFunction<do_include>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction( "ProgressIndicator", js_callStaticFunction<do_ProgressIndicator>, JS4D::PropertyAttributeNone); // ProgressIndicator( { number : maxvalue, string : sessiontext, bool : caninterrupt = false, string : windowtitle, string : progressInfo })
		_PushBackStaticFunction( "getProgressIndicator", js_callStaticFunction<do_GetProgressIndicator>, JS4D::PropertyAttributeNone); // getProgressIndicator( string : progressInfo })
		_PushBackStaticFunction( "BinaryStream", js_callStaticFunction<do_BinaryStream>, JS4D::PropertyAttributeNone); // BinaryStream : BinaryStream( File | stringURL, bool | "Write" "Read");
//		_PushBackStaticFunction( "TextStream", js_callStaticFunction<do_TextStream>, JS4D::PropertyAttributeNone); // TextStream : TextStream( File | stringURL, bool | "Write" "Read", {number:charset});
		_PushBackStaticFunction( "displayNotification", js_callStaticFunction<do_displayNotification>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction( "XmlToJSON", js_callStaticFunction<do_XmlToJSON>, JS4D::PropertyAttributeNone); // string : XmlToJSON( string : XML text, "simpleJson" "fullJson");
		_PushBackStaticFunction( "JSONToXml", js_callStaticFunction<do_JSONToXml>, JS4D::PropertyAttributeNone); // string : JSONToXml( string : JSON text, "simpleJson" "fullJson");
		_PushBackStaticFunction( "loadImage", js_callStaticFunction<do_loadImage>, JS4D::PropertyAttributeNone); // Picture : LoadImage(File)
		_PushBackStaticFunction( "garbageCollect", js_callStaticFunction<do_garbageCollect>, JS4D::PropertyAttributeNone); 
		_PushBackStaticFunction( "isoToDate", js_callStaticFunction<do_isoToDate>, JS4D::PropertyAttributeNone); 
		_PushBackStaticFunction( "dateToIso", js_callStaticFunction<do_dateToIso>, JS4D::PropertyAttributeNone); 
		_PushBackStaticFunction( "Mutex", js_callStaticFunction<do_AtomicSection>, JS4D::PropertyAttributeNone); 
		_PushBackStaticFunction( "SyncEvent", js_callStaticFunction<do_SyncEvent>, JS4D::PropertyAttributeNone); // object : System ( string : commandLine );
		_PushBackStaticFunction( "loadText", js_callStaticFunction<do_LoadText>, JS4D::PropertyAttributeNone); // string : loadText(File, {number:charset})
		_PushBackStaticFunction( "saveText", js_callStaticFunction<do_SaveText>, JS4D::PropertyAttributeNone); // saveText(text: string, File, {number:charset})
		_PushBackStaticFunction( "getURLPath", js_callStaticFunction<do_GetURLPath>, JS4D::PropertyAttributeNone); // array of string : getURLPath(string:url)
		_PushBackStaticFunction( "getURLQuery", js_callStaticFunction<do_GetURLQuery>, JS4D::PropertyAttributeNone); // object : getURLQuery(string:url)
		
		_PushBackStaticFunction( "generateUUID", js_callStaticFunction<do_GenerateUUID>, JS4D::PropertyAttributeNone); // string : generateUUID();
		
		_PushBackStaticFunction("setTimeout", js_callStaticFunction<VJSTimer::SetTimeout>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction("clearTimeout", js_callStaticFunction<VJSTimer::ClearTimeout>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction("setInterval", js_callStaticFunction<VJSTimer::SetInterval>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction("clearInterval", js_callStaticFunction<VJSTimer::ClearInterval>, JS4D::PropertyAttributeNone);

		_PushBackStaticFunction("close", js_callStaticFunction<VJSWorker::Close>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction("wait", js_callStaticFunction<VJSWorker::Wait>, JS4D::PropertyAttributeNone);
		_PushBackStaticFunction("exitWait", js_callStaticFunction<VJSWorker::ExitWait>, JS4D::PropertyAttributeNone);

		_PushBackStaticFunction("importScripts", js_callStaticFunction<VJSGlobalClass::do_ImportScripts>, JS4D::PropertyAttributeNone);

		_PushBackStaticFunction("requireNative", js_callStaticFunction<VJSGlobalClass::do_RequireNative>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete);			// object : require(string:className);
		_PushBackStaticFunction("requireFile", js_callStaticFunction<VJSGlobalClass::do_RequireFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete);

		_PushBackStaticFunction("getFileSystems", js_callStaticFunction<VJSGlobalClass::do_GetFileSystems>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete);

		_PushBackStaticFunction("getThreads", js_callStaticFunction<VJSGlobalClass::do_GetThreads>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete);

		//_PushBackStaticFunction("SetCurrentUser", js_callStaticFunction<do_SetCurrentUser>, JS4D::PropertyAttributeNone);

		_PushBackStaticFunction( 0, 0, 0);
	}
}


void VJSGlobalClass::_CheckInitStaticValues()
{
	if (sStaticValues.empty())
	{
		_PushBackStaticValue( 0, 0, 0, 0);
	}
}


void VJSGlobalClass::CreateGlobalClasses()
{
	VJSTextStream::Class();
	VJSStream::Class();
	VJSProgressIndicator::Class();
	VJSFolderIterator::Class();
	VJSFileIterator::Class();

	VJSBlob::Class();
	
#if WITH_NATIVE_HTTP_BACKEND
	VJSXMLHttpRequest::Class();
#else
	VJScURLXMLHttpRequest::Class();
#endif

	VJSEndPoint::Class();

	VJSDedicatedWorkerClass::Class();
	VJSSharedWorkerClass::Class();

	VJSMysqlBufferClass::Class();

	VJSStorageClass::Class();
	VJSBufferClass::Class();

	VJSFileErrorClass::Class();

	AddStaticValue("FileError", VJSFileErrorConstructorClass::js_getProperty<VJSFileErrorConstructorClass::_getConstructorObject>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeDontEnum);

}



void VJSGlobalClass::do_SetCurrentUser( VJSParms_callStaticFunction& inParms, VJSGlobalObject*)
{
	VString username;
	inParms.GetStringParam(1, username);
	if (!username.IsEmpty())
	{
		// a faire ici ou dans VJSSolution ?
	}
}


void VJSGlobalClass::do_trace( VJSParms_callStaticFunction& inParms, VJSGlobalObject*)
{
	size_t count = inParms.CountParams();
	for( size_t i = 1 ; i <= count ; ++i)	
	{
		VString msg;
		bool ok = inParms.GetStringParam( i, msg);
		if (!ok)
			break;
		VDebugMgr::Get()->DebugMsg( msg);
	}
}


void VJSGlobalClass::do_displayNotification( VJSParms_callStaticFunction& inParms, VJSGlobalObject*)
{
	VString message;
	inParms.GetStringParam( 1, message);
	
	VString title( "Notification");
	inParms.GetStringParam( 2, title);
	
	bool critical = false;
	inParms.GetBoolParam( 3, &critical);

	VSystem::DisplayNotification( title, message, critical ? EDN_StyleCritical : 0);
}


void VJSGlobalClass::do_include( VJSParms_callStaticFunction& inParms, VJSGlobalObject *inGlobalObject)
{
	// EvaluateScript() uses NULL (this is default) as inThisObject argument. 
	// This works fine currently, but would it be better (if any problem) to pass global (application) object instead? 
	// See WAK0074064.

	// If the first argument is a file, then the second optional argument is a boolean or string ("refresh"/"auto") 
	// controlling evaluation. Otherwise, if the first argument is a string (path), then the second optional argument 
	// can be a string telling how to interpret the path. Followed by a third optional argument controlling the 
	// evaluation.

	VFile	*file	= inParms.RetainFileParam(1, false);

	if (file != NULL)
	{
		bool newlyRegistered = inGlobalObject->RegisterIncludedFile( file);		// sc 15/06/2010 the file must be registered
		if (inParms.GetBoolParam(2,K_REFRESH_STR,K_AUTO_STR) || newlyRegistered)
		{
			VJSException	tmpExc;
			inParms.GetContext().EvaluateScript(file, NULL, &tmpExc);
			inParms.SetException(tmpExc);
		}
		file->Release();
	}
	else
	{
		VString pathname;

		if (!inParms.IsStringParam(1) || !inParms.GetStringParam(1, pathname))

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");

		else {

			bool	isOk;
			sLONG	pathType;			// 0: "user", 1: "system", 2: "relative".
			bool	refresh;

			// Read optional path interpretation (default is "user") and evaluation control (default is "auto").

			isOk = true;
			pathType = 0;
			if (inParms.CountParams() < 2)
				
				refresh = false;

			else if (inParms.IsBooleanParam(2) || !inParms.IsStringParam(2))

				inParms.GetBoolParam(2, &refresh);

			 else {

				XBOX::VString	string;

				inParms.GetStringParam(2, string);
				if (string.EqualToString(K_REFRESH_STR)) 

					refresh = true;
					
				else if (string.EqualToString(K_AUTO_STR))

					refresh = false;

				else {

					if (string.EqualToString(K_USER_STR)) 

						pathType = 0;

					else if (string.EqualToString(K_SYSTEM_STR)) 

						pathType = 1;

					else if (string.EqualToString(K_RELATIVE_STR))

						pathType = 2;

					else {

						XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER, "2");
						isOk = false;

					}

					if (isOk && inParms.CountParams() >= 3)

						refresh = inParms.GetBoolParam(3, K_REFRESH_STR, K_AUTO_STR);							

					else

						refresh = false;

				}

			} 

			// Find out folder.

			VFolder	*folder = NULL;

			if (isOk) {

				if (pathType == 0)
				{
					folder = inGlobalObject->GetRuntimeDelegate()->RetainScriptsFolder();
					isOk = folder != NULL;
				}
				else if (pathType == 1) 
				{
					VFolder* rootfolder = VProcess::Get()->RetainFolder('walb');
					if (rootfolder != NULL)
					{
						VFilePath path;
						rootfolder->GetPath(path);
						path.ToSubFolder("WAF");
						path.ToSubFolder("Core");
						path.ToSubFolder("Runtime");
						folder = new VFolder(path);
						QuickReleaseRefCountable(rootfolder);
					}
					isOk = folder != NULL;
				} 
				else if (pathType == 2) 
				{
					XBOX::VFilePath	path, parentPath;
					
					isOk = false;
#if  USE_V8_ENGINE
					VString urlString = inParms.GetContext().GetCurrentScriptUrl();
					XBOX::VURL	url(urlString, true);
					if (!url.GetFilePath(path))
					{
						path.Clear();	// Failed to parse URL, make path empty.
					}
					else
					{
						isOk = true;
					}
#else
					JS4D::StringRef	sourceURL;
					if ((sourceURL = JS4DGetExecutionURL(inParms.GetContext())) != NULL) {

						XBOX::VString	urlString;

						if (JS4D::StringToVString(sourceURL, urlString)) {

							XBOX::VURL	url(urlString, true);
		
							if (!url.GetFilePath(path))

								path.Clear();	// Failed to parse URL, make path empty.

							else

								isOk = true;

						}
						JSStringRelease(sourceURL);
					}
					
#endif

					if (isOk)

						isOk = path.GetParent(parentPath);

					if (!isOk) {

						// Failed to get a valid path of parent folder.

						XBOX::vThrowError(XBOX::VE_UNKNOWN_ERROR);
					
					} 
				
					if (isOk && (folder = new VFolder(parentPath)) == NULL) {

						XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
						isOk = false;

					}

				}
				else 
				{
					xbox_assert(false);	// Invalid pathType value.
				}

			}

			// Open file and interpret it.

			if (isOk) {

				file = new VFile( *folder, pathname, FPS_POSIX);
					
				bool newlyRegistered = inGlobalObject->RegisterIncludedFile( file);		// sc 15/06/2010 the file must be registered
				if (refresh || newlyRegistered)
				{
					VJSException	tmpExc;
					inParms.GetContext().EvaluateScript(file, NULL, &tmpExc);
					inParms.SetException(tmpExc);
				}
				ReleaseRefCountable( &file);
				ReleaseRefCountable( &folder);

			}

		}
	}
}


void VJSGlobalClass::do_testMe( VJSParms_callStaticFunction& inParms, VJSGlobalObject*)
{
	VJSObject globalObject( inParms.GetContext().GetGlobalObject());
	
	VString functionName;
	inParms.GetStringParam( 1, functionName);
	
	std::vector<VJSValue> values;
	size_t count = inParms.CountParams();
	for( size_t i = 2 ; i <= count ; ++i)
		values.push_back( inParms.GetParamValue( i));

	VJSValue result( inParms.GetContext());
	VJSException	tmpExc;
	bool ok = globalObject.CallMemberFunction( functionName, &values, &result, &tmpExc);
	inParms.SetException(tmpExc);
	inParms.ReturnValue( result);
}

void VJSGlobalClass::do_GetProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext)
{
	if (ioParms.IsStringParam(1))
	{
		VString userinfo;
		ioParms.GetStringParam(1, userinfo);
		VProgressIndicator* progress = VProgressManager::RetainProgressIndicator(userinfo);
		if (progress != nil)
			ioParms.ReturnValue(VJSProgressIndicator::CreateInstance(ioParms.GetContext(), progress));
		QuickReleaseRefCountable(progress);
	}
}

	
void VJSGlobalClass::do_ProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext)
{
	VString sessiontile;
	VString windowtile, userinfo;
	bool caninterrupt = false;
	Real maxvalue = -1;
	VError err = VE_OK;
	
	if (ioParms.IsNumberParam(1))
		ioParms.GetRealParam(1, &maxvalue);
	else
		err = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");

	if (err == VE_OK )
	{
		if (ioParms.IsStringParam(2))
			ioParms.GetStringParam(2, sessiontile);
		else
			err = vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
	}

	caninterrupt = ioParms.GetBoolParam( 3, K_CAN_INTERRUPT_STR, K_CANNOT_INTERRUPT_STR);

		
	//VProgressIndicator* progress = inContext->GetRuntimeDelegate()->CreateProgressIndicator( windowtile);

	if (err == VE_OK)
	{
		VProgressIndicator* progress = new VProgressIndicator();
		if (progress != NULL)
		{
			if (ioParms.CountParams() >= 4)
			{
				if (ioParms.IsStringParam(4))
				{
					ioParms.GetStringParam(4, windowtile);
					progress->SetTitle(windowtile);
				}
				else
					vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "4");
			}
			if (ioParms.CountParams() >= 5)
			{
				if (ioParms.IsStringParam(5))
				{
					ioParms.GetStringParam(5, userinfo);
					progress->SetUserInfo(userinfo);
				}
				else
					vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "5");
			}
			progress->BeginSession((sLONG8)maxvalue, sessiontile, caninterrupt);
		}
		ioParms.ReturnValue(VJSProgressIndicator::CreateInstance(ioParms.GetContext(), progress));
		
		ReleaseRefCountable( &progress);
	}
	else
		ioParms.ReturnNullValue();	
}	


void VJSGlobalClass::do_BinaryStream(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VJSStream::do_BinaryStream( ioParms);
}

void VJSGlobalClass::do_XmlToJSON(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString xml;
	VString json, root;
	bool simpleJSON = false;

	ioParms.GetStringParam( 1, xml);
	if ( ioParms.CountParams() >= 2)
	{
		VString s;
		if (ioParms.IsStringParam(2))
		{
			ioParms.GetStringParam(2, s);
			if(s== "json-bag")
			{
				simpleJSON = true;
				root="bag";
				if(ioParms.CountParams() >= 3)
				{
					if (ioParms.IsStringParam(3))
						ioParms.GetStringParam(3, root);
					else
						vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "3");
				}
			}
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
	}

	if (simpleJSON)
	{
		XBOX::VValueBag *bag = new XBOX::VValueBag;

		VError err = LoadBagFromXML( xml, root, *bag);
		if(err == VE_OK)
			bag->GetJSONString(json);
		ReleaseRefCountable( &bag); 			
	}
	else
	{
		VString errorMessage; sLONG lineNumber;
		VError err = VXMLJsonUtility::XMLToJson(xml, json, errorMessage, lineNumber);
	}

	ioParms.ReturnString(json);
}

void VJSGlobalClass::do_JSONToXml(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString xml;
	VString json, root;
	bool simpleJSON = false;
	bool isxhtml = false;

	ioParms.GetStringParam( 1, json);
	if ( ioParms.CountParams() >= 2)
	{
		VString s;
		if (ioParms.IsStringParam(2))
		{
			ioParms.GetStringParam(2, s);
			if(s== "json-bag")
			{
				simpleJSON = true;
				root="bag";
				if(ioParms.CountParams() >= 3)
				{
					if (ioParms.IsStringParam(3))
						ioParms.GetStringParam(3, root);
					else
						vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "3");
				}
			}
			else if(s== "xhtml")
			{
				isxhtml = true;
			}
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
	}

	if (simpleJSON)
	{
		XBOX::VValueBag *bag = new XBOX::VValueBag;
		bag->FromJSONString(json);
		
		xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
		bag->DumpXML( xml, root, false);
		ReleaseRefCountable( &bag); 			
	}
	else
	{
		if(isxhtml)
			VError err = VXMLJsonUtility::JsonToXHTML(json, xml);
		else
			VError err = VXMLJsonUtility::JsonToXML(json, xml);
	}

	ioParms.ReturnString(xml);
}


void VJSGlobalClass::do_loadImage(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	bool okloaded = false;

	VFile* file = ioParms.RetainFileParam(1);

	if (file != nil)
	{
		if (file->Exists())
		{
			XBOX::VPicture* pict = new XBOX::VPicture(*file, false);
			/*
			VJSPictureContainer* pictContains = new VJSPictureContainer(pict, true, ioParms.GetContext());
			ioParms.ReturnValue(VJSImage::CreateInstance(ioParms.GetContext(), pictContains));
			pictContains->Release();
			*/
			if (pict != NULL)
			{
				okloaded = true;
				ioParms.ReturnVValue(*pict, false);
				delete pict;
			}
		}
		file->Release();
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");

	if (!okloaded)
		ioParms.ReturnNullValue();
}


void VJSGlobalClass::do_garbageCollect(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	ioParms.GetContext().GarbageCollect();
}


void VJSGlobalClass::do_isoToDate(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VTime dd;
	VString s;
	ioParms.GetStringParam(1, s);
	dd.FromJSONString(s);
	ioParms.ReturnTime(dd);
}

void VJSGlobalClass::do_dateToIso(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	VJSValue jsval(ioParms.GetContext());
	if (ioParms.CountParams() > 0)
	{
		VTime dd;
		jsval = ioParms.GetParamValue(1);
		jsval.GetTime(dd);
		dd.GetJSONString(s);
	}
	ioParms.ReturnString(s);
}



void VJSGlobalClass::do_AtomicSection(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	bool okresult = false;
	if (ioParms.IsStringParam(1))
	{
		ioParms.GetStringParam(1, s);
		if (!s.IsEmpty())
		{
			jsAtomicSection* atomsec = jsAtomicSection::RetainAtomicSection(s);
			if (atomsec != nil)
			{
				okresult = true;
				ioParms.ReturnValue(VJSAtomicSection::CreateInstance(ioParms.GetContext(), atomsec));
				atomsec->Release();
			}
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	if (!okresult)
		ioParms.ReturnNullValue();
}


void VJSGlobalClass::do_SyncEvent(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	bool okresult = false;
	if (ioParms.IsStringParam(1))
	{	ioParms.GetStringParam(1, s);
		if (!s.IsEmpty())
		{
			jsSyncEvent* sync = jsSyncEvent::RetainSyncEvent(s);
			if (sync != nil)
			{
				okresult = true;
				ioParms.ReturnValue(VJSSyncEvent::CreateInstance(ioParms.GetContext(), sync));
				sync->Release();
			}
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	if (!okresult)
		ioParms.ReturnNullValue();
}

void VJSGlobalClass::do_LoadText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	bool okloaded = false;

	VFile* file = ioParms.RetainFileParam(1);

	if (file != nil)
	{
		if (file->Exists())
		{
			VFileStream inp(file);
			VError err = inp.OpenReading();
			if (err == VE_OK)
			{
				CharSet defaultCharSet = VTC_UTF_8;
				sLONG xcharset = 0;
				if (ioParms.GetLongParam(2, &xcharset) && xcharset != 0)
					defaultCharSet = (CharSet)xcharset;
				inp.GuessCharSetFromLeadingBytes(defaultCharSet );
				inp.SetCarriageReturnMode( eCRM_NATIVE );
				VString s;
				err = inp.GetText(s);
				if (err == VE_OK)
				{
					ioParms.ReturnString(s);
					okloaded = true;
				}
				inp.CloseReading();
			}
			
			
		}
		file->Release();
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");

	if (!okloaded)
		ioParms.ReturnNullValue();

}


void VJSGlobalClass::do_SaveText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	if (ioParms.IsStringParam(1))
	{
		if (ioParms.GetStringParam(1, s))
		{
			VFile* file = ioParms.RetainFileParam(2);
			if (file != nil)
			{
				VError err = VE_OK;
				if (file->Exists())
					err = file->Delete();
				if (err == VE_OK)
					err = file->Create();
				if (err == VE_OK)
				{
					CharSet defaultCharSet = VTC_UTF_8;
					sLONG xcharset = 0;
					if (ioParms.GetLongParam(3, &xcharset) && xcharset != 0)
						defaultCharSet = (CharSet)xcharset;
					VFileStream inp(file);
					err = inp.OpenWriting();
					if (err == VE_OK)
					{
						inp.SetCharSet(defaultCharSet);
						inp.PutText(s);
						inp.CloseWriting();
					}
				}
			}
			else
				vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "2");
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
}


void VJSGlobalClass::do_GetURLPath(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	if (ioParms.IsStringParam(1))
	{
		if (ioParms.GetStringParam(1, s))
		{
			VFullURL url(s);
			VJSArray result(ioParms.GetContext());
			const VString* next = url.NextPart();
			while (next != NULL)
			{
				VJSValue elem(ioParms.GetContext());
				elem.SetString(*next);
				result.PushValue(elem);
				next = url.NextPart();
			}
			ioParms.ReturnValue(result);
		}
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
}


void VJSGlobalClass::do_GetURLQuery(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VString s;
	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, s))
	{
		VFullURL url(s);
		VJSObject result(ioParms.GetContext());
		result.MakeEmpty();
		const VValueBag& values = url.GetValues();
		VIndex nbvalues = values.GetAttributesCount();
		for (VIndex i = 1; i <= nbvalues; i++)
		{
			VString attName;
			const VValueSingle* cv = values.GetNthAttribute(i, &attName);
			if (cv != nil)
			{
				result.SetProperty(attName, cv);
			}
		}
		ioParms.ReturnValue(result);
	}
	else
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
}

void VJSGlobalClass::do_GenerateUUID(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*)
{
	VUUID		theUUID(true);
	VString		uuidStr;
	
	theUUID.GetString(uuidStr);
	ioParms.ReturnString(uuidStr);
}

// See web workers specification (http://www.w3.org/TR/workers/), section 5.1.

void VJSGlobalClass::do_ImportScripts (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{	
	if (!ioParms.CountParams())

		return;

	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VFolder		*folder;
	XBOX::VString		rootPath;

	folder = context.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	xbox_assert(folder != NULL);
	folder->GetPath(rootPath, XBOX::FPS_POSIX);
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

	std::list<XBOX::VString>	urls;

	for (sLONG i = 1; i <= ioParms.CountParams(); i++) {

		XBOX::VString	url;

		if (!ioParms.GetStringParam(i, url)) {
			
			XBOX::VString	indexString;

			indexString.FromLong(i);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, indexString);
			return;

		}

#if VERSIONWIN

		if (url.GetLength() >= 2
		&& VIntlMgr::GetDefaultMgr()->IsAlpha(url.GetUniChar(1)) 
		&& url.GetUniChar(2) == ':' 
		&& url.GetUniChar(3) == '/') {

#else

		if (url.GetUniChar(1) == '/') {

#endif

			urls.push_back(url);

		} else {

			urls.push_back(rootPath + url);

		}

	}

	XBOX::VJSValue	result(context);
	bool			isOk;

	result.SetNull();
	isOk = true;
	do {

		XBOX::VURL		url(urls.front(), eURL_POSIX_STYLE, CVSTR ("file://"));
		XBOX::VFilePath	path;
		XBOX::VString	script;
				
		if (url.GetFilePath(path)) {

			XBOX::VFile			file(path);			
			XBOX::VFileStream	stream(&file);
			XBOX::VError		error;
		
			if ((error = stream.OpenReading()) == XBOX::VE_OK
			&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

				stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
				error = stream.GetText(script);

			}
			stream.CloseReading();

			isOk = error == XBOX::VE_OK;

		} 

		if (!isOk) {

			// Failed to read script.

			XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, urls.front());
		
		} else if (!context.CheckScriptSyntax(script, &url, NULL)) {

			// Syntax error.

			XBOX::vThrowError(XBOX::VE_JVSC_SYNTAX_ERROR, urls.front());
			
		} else {

			XBOX::VFile	*file;
			
			if ((file = new XBOX::VFile(path)) == NULL) {

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				break;

			} else {

				XBOX::VJSObject				thisObject	= ioParms.GetThis();
				XBOX::VJSException			exception;
			
				context.GetGlobalObjectPrivateInstance()->RegisterIncludedFile(file);
			
				// ImportScripts() will return evaluation result of last argument (script).

				if (!context.EvaluateScript(script, &url, &result, &exception, &thisObject)) {
						
					ioParms.SetException(exception);

/*
					XBOX::VString	exceptionExplanation;

					JS4D::ValueToString(context, exception, exceptionExplanation);
					XBOX::DebugMsg(exceptionExplanation);
*/

				}

				XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

			}

		}
		urls.pop_front();

	} while (isOk && !urls.empty());

	ioParms.ReturnValue(result);
}

void VJSGlobalClass::do_RequireNative (VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject)
{
	xbox_assert(inGlobalObject != NULL);

	VString	name;

	if (ioParms.CountParams() != 1 || !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, name))
	{
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	}
	else
	{
		MapOfModuleConstructor::const_iterator i = sModuleConstructors.find( name);
		if (i != sModuleConstructors.end())
			ioParms.ReturnValue( (*i->second)( ioParms.GetContext(), name));
		else
			ioParms.ReturnNullValue();
	}
}

void VJSGlobalClass::do_RequireFile (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *)
{	
	if (ioParms.CountParams() != 2)

		return;

	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VFolder		*folder;
	XBOX::VString		rootPath;

	folder = context.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	xbox_assert(folder != NULL);
	folder->GetPath(rootPath, XBOX::FPS_POSIX);
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

	XBOX::VString	urlString;

	if (!ioParms.GetStringParam(1,  urlString)) {
			
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	XBOX::VString	moduleInitString;

	if (!ioParms.GetStringParam(2, moduleInitString)) {
			
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
		return;

	}

#if VERSIONWIN

	if (urlString.GetLength() >= 2
	&& VIntlMgr::GetDefaultMgr()->IsAlpha(urlString.GetUniChar(1)) 
	&& urlString.GetUniChar(2) == ':' 
	&& urlString.GetUniChar(3) == '/') {

#else

	if (urlString.GetUniChar(1) == '/') {

#endif

	} else {

		urlString = rootPath + urlString;

	}

	XBOX::VJSValue	result(context);
	bool			isOk;

	result.SetNull();
	isOk = true;

	XBOX::VURL		url(urlString, eURL_POSIX_STYLE, CVSTR ("file://"));
	XBOX::VFilePath	path;
	XBOX::VString	script;
				
	if (url.GetFilePath(path)) {

		XBOX::VFile			file(path);			
		XBOX::VFileStream	stream(&file);
		XBOX::VError		error;
		
		if ((error = stream.OpenReading()) == XBOX::VE_OK
		&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

			stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
			error = stream.GetText(script);

		}
		stream.CloseReading();

		isOk = error == XBOX::VE_OK;

	} 

	XBOX::VURL					emptyURL;
	XBOX::VJSObject				thisObject	= ioParms.GetThis();
	XBOX::VJSException			exception;

	if (!isOk) {

		// Failed to read script.

		XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, urlString);
				
	} else if (!context.CheckScriptSyntax(script, &url, NULL)) {

		// Syntax error.

		XBOX::vThrowError(XBOX::VE_JVSC_SYNTAX_ERROR, urlString);
			
	} else {

		XBOX::VFile	*file;
			
		if ((file = new XBOX::VFile(path)) == NULL) {

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			
		} else {

			context.GetGlobalObjectPrivateInstance()->RegisterIncludedFile(file);

			XBOX::VString	closuredScript;
			
			// Doesn't check that moduleInitString has a correct syntax.

			closuredScript.AppendString("(function (){var exports = {}; var module = {}; ");
			closuredScript.AppendString(moduleInitString);
			closuredScript.AppendString("; ");
			closuredScript.AppendString(script);
			closuredScript.AppendString("; return exports; }).call();");
			
			// Will return exports object.

			if (!context.EvaluateScript(closuredScript, &url, &result, &exception, &thisObject)) {
						
				ioParms.SetException(exception);
/*
				XBOX::VString	exceptionExplanation;

				JS4D::ValueToString(context, exception, exceptionExplanation);
				XBOX::DebugMsg(exceptionExplanation);
*/
			}

			XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

		}
	
	}

	ioParms.ReturnValue(result);
}


void VJSGlobalClass::do_GetFileSystems (VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject)
{
	IJSRuntimeDelegate *runtimeDelegate = (inGlobalObject == NULL) ? NULL : inGlobalObject->GetRuntimeDelegate();
	if (runtimeDelegate != NULL)
	{
		VFileSystemNamespace *fsNameSpace = runtimeDelegate->RetainRuntimeFileSystemNamespace();
		if (fsNameSpace != NULL)
		{
			VFileSystemNamespace::OrderedMapOfFileSystem fsmap;
			fsNameSpace->RetainMapOfFileSystems(fsmap);
			VJSArray arr(ioParms.GetContext());
			for (VFileSystemNamespace::OrderedMapOfFileSystem::iterator cur = fsmap.begin(), end = fsmap.end(); cur != end; ++cur)
			{
				arr.PushValue(VJSLocalFileSystem::GetInstance(ioParms.GetContext(), cur->second, true));
			}
			ioParms.ReturnValue(arr);
			fsNameSpace->Release();
		}
	}
}


void VJSGlobalClass::do_GetThreads(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject)
{
	std::vector<VRefPtr<VTask> > tasks;
	VTaskMgr::Get()->GetTasks(tasks);

	VJSArray arr(ioParms.GetContext());
	for (std::vector<VRefPtr<VTask> >::iterator cur = tasks.begin(), end = tasks.end(); cur != end; ++cur)
	{
		VJSObject taskobj(arr, -1);
		VTask* task = cur->Get();
		VString taskname;
		task->GetName(taskname);
		taskobj.SetProperty("name", taskname);
		VString taskstate;
		switch (task->GetState())
		{
			case 	TS_CREATING:
				taskstate = "creating";
				break;

			case TS_RUN_PENDING:
				taskstate = "pending";
				break;

			case TS_RUNNING:
				taskstate = "running";
				break;

			case TS_DYING:
				taskstate = "dying";
				break;

			case TS_DEAD:
				taskstate = "dead";
				break;

			default:
				break;
		}
		taskobj.SetProperty("state", taskstate);
		Real systime, usertime;
		task->GetCPUTimes(systime, usertime);
		taskobj.SetProperty("systemTime", systime);
		taskobj.SetProperty("userTime", usertime);
		Real cpuusage;
		task->GetCPUUsage(cpuusage);
		taskobj.SetProperty("cpuUsage", cpuusage);
	}
	ioParms.ReturnValue(arr);
}



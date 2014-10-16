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
#ifndef __VJSContext__
#define __VJSContext__

#include "JS4D.h"

#define WKA_PROVIDES_UNIFIED_DBG

class IRemoteDebuggerServer;
class IWAKDebuggerServer;
class IChromeDebuggerServer;

BEGIN_TOOLBOX_NAMESPACE

class VJSGlobalObject;
class VJSGlobalContext;
class IJSRuntimeDelegate;

//======================================================

class XTOOLBOX_API VJSException : public XBOX::VObject
{

public:
	explicit					VJSException() : fException(NULL)	{}
	explicit					VJSException(const VJSException& inOther) : fException(inOther.fException)	{}
	explicit					VJSException(JS4D::ExceptionRef inExceptionRef) : fException(inExceptionRef)	{}

	bool						ThrowVErrorForException(JS4D::ContextRef inContext) const {	return JS4D::ThrowVErrorForException(inContext,fException); }

	bool						IsEmpty() const { return fException == NULL; }
	JS4D::ExceptionRef*			GetPtr() const { return &fException; }
	JS4D::ExceptionRef			GetValue() const { return fException; }
	void						SetValue(JS4D::ExceptionRef inExceptionRef) { fException = inExceptionRef; }

static inline XBOX::JS4D::ValueRef* GetExceptionRefIfNotNull(XBOX::VJSException* inException)
{
	return (XBOX::JS4D::ValueRef*)(inException ? inException->GetPtr() : NULL);
}
private:
mutable JS4D::ExceptionRef		fException;

};


/*
	not thread safe.
	
*/
class XTOOLBOX_API VJSContextGroup : public XBOX::VObject, public XBOX::IRefCountable
{
public:
											VJSContextGroup();
											~VJSContextGroup();
	
private:
											VJSContextGroup( const VJSContextGroup&);	// forbidden
											VJSContextGroup& operator=( const VJSContextGroup&);	// forbidden

};

//======================================================
#if USE_V8_ENGINE
class V4DContext;
#endif

/*
	not thread safe.

	It's a smarter pointer & wrapper class.
	
	The context is not retained to avoid cyclic redundancy (children don't retain their parent)
*/
class XTOOLBOX_API VJSContext : public XBOX::VObject
{
friend class VJSParms_withContext;
public:
	/*class VjsScope
	{
	public:
		VjsScope(const VJSContext&	inContext);
	private:
		v8::HandleScope				fHS;
		JS4D::v8ContextRef			fContext;
	};*/

#if CHECK_JSCONTEXT_THREAD_COHERENCY
			bool							Check(bool inDisplay=false) const;
#endif
	explicit								VJSContext( JS4D::ContextRef inContext):fContext( inContext),fGlobalContext(NULL),fDebuggerAllowed(true)		{}
											VJSContext( const VJSContext& inOther):fContext( inOther.fContext),fGlobalContext(NULL),fDebuggerAllowed(true)	{}
											VJSContext( const VJSGlobalContext *inGlobalContext);
											~VJSContext();

			// evaluates some script an optionnally returns the result (outResult may be NULL) and the exception (outException may be NULL).
			// return true if no exception occured.
	
			// At each EvaluateScript(), the path of the currently executing script is stored as a "specific" in the global object. After evaluation is over, 
			// previous path (if any) is restored, hence recursive calling of EvaluateScript() is supported.

			bool							EvaluateScript( VFile *inFile, VJSValue *outResult, VJSException& outException, VJSObject* inThisObject = NULL) const;
			bool							EvaluateScript( VFile *inFile, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject = NULL) const;
			bool							EvaluateScript( const VString& inScript, const VURL *inSource, VJSValue *outResult, VJSException& outException, VJSObject* inThisObject = NULL) const;
			bool							EvaluateScript( const VString& inScript, const VURL *inSource, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject = NULL) const;
			
			// Check syntax of the script. Return false if erroneous, in which case outException contains details. 

			bool							CheckScriptSyntax (const VString &inScript, const VURL *inSource, XBOX::VJSException* outException = NULL) const;

			VJSObject						MakeFunction( const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, JS4D::ExceptionRef *outException);
			bool							GetFunction(const VString& inFunctionRef, VJSObject& outFuncObj);
	
			VJSObject						GetGlobalObject() const;
#if USE_V8_ENGINE
			xbox::VString					GetCurrentScriptUrl();
			void							GetGlobalObject(VJSObject& ioObject);
#endif
			VJSGlobalObject*				GetGlobalObjectPrivateInstance() const;
			const VJSGlobalContext*			GetGlobalContext() const {return fGlobalContext;}

			void							GarbageCollect() const;

			void							EnableDebugger()									{ fDebuggerAllowed = true; }
			void							DisableDebugger()									{ fDebuggerAllowed = false; }

											operator JS4D::ContextRef() const					{ return fContext;}
											VJSContext& operator=( const VJSContext& inOther);

			UniChar							GetWildChar() const;

private:
friend class VJSValue;
#if USE_V8_ENGINE
friend class VJSClassImpl;
friend class VJSConstructor;
friend class VJSObject;
friend class VJSArray;
friend class VJSPropertyIterator;
friend class VJSJSON;
#endif
			void							EndOfNativeCodeExecution() const;

			JS4D::ContextRef				fContext;
			const VJSGlobalContext*			fGlobalContext;
			bool							fDebuggerAllowed;
};


//======================================================
#if USE_V8_ENGINE
class VJSObject;
#endif

class XTOOLBOX_API VJSGlobalContext : public XBOX::VObject, public XBOX::IRefCountable
{
public:
	static	VJSGlobalContext*				Create( IJSRuntimeDelegate *inRuntimeDelegate);

			// evaluates some script in global context and throw an xbox VError if a javascript exception occured.
			// you should not uses these if being called from inside some javascript code (prefer VJSContext variants instead)
			// return true if no exception occured.
			// you'll have to delete any result returned.
			bool							EvaluateScript( VFile *inFile, VValueSingle **outResult, bool inJSONresult = false ) const;
			bool							EvaluateScript( const VString& inScript, const VURL *inSource, VValueSingle **outResult, bool inJSONresult = false ) const;

			// Defines the root of the JavaScript source files for the current JS runtime. At the moment, this is essentially the 
			// root of the current solution. Source root is used by the debugger to sync file paths between the debug server and
			// debug client - they both use JS file paths relative to a given source root.
	static	void							SetSourcesRoot ( const VFolder & inRootFolder );

	static	void							SetDebuggerServer(	IWAKDebuggerServer*		inDebuggerServer,
																IChromeDebuggerServer*	inChromeDebuggerServer,
																IRemoteDebuggerServer*	inNoDebugger = NULL);

	static	IRemoteDebuggerServer*			GetDebuggerServer( );

			// Returns true if debugger is currently paused on either breakpoint or an exception, false otherwise.
	static	bool							IsDebuggerPaused ( );

			// Returns true if there is at least one debug client connected, false otherwise.
	static	bool							IsDebuggerActive ( );

			// Aborts all JS contexts execution blocked in debug on a JS statement because of a break point or whatever other reason.
	static	void							AbortAllDebug ( );

			// Allows or forbids debugging server launch. Typically, debug server is not launched in the studio but is launched on the server.
	static	void							AllowDebuggerLaunch ( );
	static	void							ForbidDebuggerLaunch ( );

			// mark the context as being in use for execution.
			// Note that EvaluateScript and VJSContext( const VJSGlobalContext) call Use/Unuse, so you shouldn't have to call these yourselves.
			JS4D::ContextRef				Use() const;
			void							Unuse() const;

			UniChar							GetWildChar() const;
#if USE_V8_ENGINE
	static	IChromeDebuggerServer*			sChromeDebuggerServer;
#endif
private:
#if USE_V8_ENGINE
											VJSGlobalContext(JS4D::GlobalContextRef inGlobalContext,VJSObject* inObjRef);
#else
											VJSGlobalContext(JS4D::GlobalContextRef inGlobalContext);
#endif
											VJSGlobalContext(); // forbidden
											~VJSGlobalContext();

											VJSGlobalContext( const VJSGlobalContext&);	// forbidden
											VJSGlobalContext& operator=( const VJSGlobalContext&);	// forbidden

	static	VJSGlobalContext*				Create( IJSRuntimeDelegate *inRuntimeDelegate, JS4D::ClassRef inGlobalClassRef);

	JS4D::GlobalContextRef					fContext;
#if USE_V8_ENGINE
	VJSObject*								fGlobalObject;
friend class VJSContext;
#endif
	static	IRemoteDebuggerServer*			sWAKDebuggerServer;//also stored here, not to modify JSC API
	static XBOX::VCriticalSection			sWAKDebuggerServerLock;

};


END_TOOLBOX_NAMESPACE

#endif

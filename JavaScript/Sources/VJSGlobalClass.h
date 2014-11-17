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
#ifndef __VJSGlobalClass__
#define __VJSGlobalClass__

#include "JS4D.h"
#include "ServerNet/VServerNet.h"

namespace xbox
{
	class VSystemWorkerNamespace;
};


BEGIN_TOOLBOX_NAMESPACE

//======================================================


class XTOOLBOX_API VJSSpecifics : public XBOX::VObject
{
public:
								VJSSpecifics()									{}
								~VJSSpecifics();
								
	typedef OsType	key_type;
	typedef void (*value_destructor_type)( void*);
	
	// common value destructors
	static	void				DestructorVoid( void *inValue)					{}
	static	void				DestructorVObject( void *inValue)				{ delete static_cast<XBOX::VObject*>( inValue);}
	static	void				DestructorReleaseVObject( void *inValue)		{ XBOX::IRefCountable *o = dynamic_cast<XBOX::IRefCountable*>( static_cast<XBOX::VObject*>( inValue)); if (o) o->Release(); }
	static	void				DestructorReleaseIRefCountable( void *inValue)	{ XBOX::IRefCountable *o = static_cast<XBOX::IRefCountable*>( inValue); if (o) o->Release(); }
	static	void				DestructorReleaseCComponent( void *inValue)		{ XBOX::CComponent *o = static_cast<XBOX::CComponent*>( inValue); if (o) o->Release(); }

			void*				Get( key_type inKey) const;
			bool				Set( key_type inKey, void *inValue, value_destructor_type inDestructor);

private:
	typedef std::map<key_type, std::pair<void*,value_destructor_type> >	MapOfValues;
			MapOfValues			fMap;
};


//======================================================

class XTOOLBOX_API IJSRuntimeDelegate
{
public:

	virtual	XBOX::VFolder*					RetainScriptsFolder() = 0;

	virtual XBOX::VProgressIndicator*		CreateProgressIndicator( const XBOX::VString& inTitle) = 0;

			// Returns the namespace for file systems available with this runtime.
			// The runtime might be a wakanda project or solution, or a 4D application.
			// May return NULL
	virtual	XBOX::VFileSystemNamespace*		RetainRuntimeFileSystemNamespace();

	virtual	XBOX::VSystemWorkerNamespace*	RetainRuntimeSystemWorkerNamespace() = 0;

			// tells if File() and Folder() JS constructors should allow access to any files.
			// default is true for converted solutions, false to new solutions.
	virtual	bool							CanAccessAllFiles();
	
			// SetRuntimeSpecific() let you associate some custom data with the host runtime.
			// The signature must be unique in the whole application. It could be something like "SharedWorkers" or "SharedSQLConnections".
			// The custom data object is something of your own. It could be a map of named shared workers or a map of shared sql connections for example.
			// 
			// Warning: the runtime delegate actual implementation can be various things like a wakanda project, a wakanda solution or a 4D database.
			// Sometimes it's a short-live context for evaluating some js expression.
			//
			// Your custom data objects will be released when the runtime delegate is destroyed when a project, solution or 4D application is closed.
			// 			
			void							SetRuntimeSpecific( const XBOX::VString& inSignature, XBOX::IRefCountable *inObject);
			XBOX::IRefCountable*			RetainRuntimeSpecific( const XBOX::VString& inSignature) const;

	// Facilities to allow setting WebSocket connection handlers on current project HTTP server.
	// Handler is called with newly created XBOX::VWebSocket instance and a pointer to inUserData given at setup.
	// Only VRIAServerProjectJSRuntimeDelegate shoud override these methods, because it has access to HTTP server.

	typedef XBOX::VError					WebSocketHandler (XBOX::VJSContext &inContext, XBOX::VWebSocket *inWebSocket, void *inUserData);

	virtual XBOX::VError					AddWebSocketHandler (XBOX::VJSContext &inContext, const XBOX::VString &inPath, WebSocketHandler *inHandler, void *inUserData);
	virtual XBOX::VError					RemoveWebSocketHandler (XBOX::VJSContext &inContext, const XBOX::VString &inPath);

private:
	typedef	unordered_map_VString<VRefPtr<IRefCountable> >				MapOfSpecifics;
	mutable	VCriticalSection				fMutex;
			MapOfSpecifics					fSpecifics;

};


//======================================================

/*
	VJSGlobalObject is the private data class for one context global object.
	not thread safe except specifically claimed as such.
*/
class XTOOLBOX_API VJSGlobalObject : public XBOX::VObject, public XBOX::IRefCountable
{
friend class VJSContext;
public:
											VJSGlobalObject( JS4D::GlobalContextRef inContext, VJSContextGroup *inContextGroup, IJSRuntimeDelegate *inRuntimeDelegate);
	
											/*
												'DB4D'	->	CDB4DBaseContext*
											*/
			void*							GetSpecific( VJSSpecifics::key_type inKey) const															{ return fSpecifics.Get( inKey);}
			bool							SetSpecific( VJSSpecifics::key_type inKey, void *inValue, VJSSpecifics::value_destructor_type inDestructor)	{ return fSpecifics.Set( inKey, inValue, inDestructor);}

			IJSRuntimeDelegate*				GetRuntimeDelegate() const		{ return fRuntimeDelegate;}
			VJSContextGroup*				GetContextGroup() const			{ return fContextGroup;}
			JS4D::GlobalContextRef			GetContext() const				{ return fContext; }

											operator VJSObject() const;
											
											// register a file for the 'include' function.
											// A file can't be registered twice.
											// returns true if the file has been successfully registered.
			bool							RegisterIncludedFile( VFile *inFile);

			bool							IsAnIncludedFile( const VFile *inFile) const;

											// Returns true if some files have been changed since they have been included
			bool							IsIncludedFilesHaveBeenChanged() const;

											// Ask a garbage collection on associated context.
											// If the context is used by another thread, the garbage collection will be deferred until the next call to some c++ code from javascript or when the context is marked unused.
											// thread safe
			void							GarbageCollect();

											// GarbageCollectIfNecessary will collect garbage if a previous call to GarbageCollect couldn't be executed because the context was in use by another thread.
											// Not thread safe! Must be called from the thread using the context like any other method of this class except GarbageCollect().
			void							GarbageCollectIfNecessary();

											// the context is being marked as 'in use' by VJSGlobalContext::EvaluateScript.
											// this is to track the use of a context by a thread and synchronize the calls to GarbageCollect.
			void							UseContext();
			void							UnuseContext();

											// Try to use context, which must not be used (zero use count), return true if successful. Thread safe.
				
			bool							TryToUseContext ();

			const unordered_map_VString<VRefPtr<VFile> >& GetIncludedFiles() const
			{
				return fIncludedFiles;
			}
#if CHECK_JSCONTEXT_THREAD_COHERENCY
			bool									Check(bool inDisplay=false) const {
				bool isOk = false;
				VString	tmp("Check backtrace=\n");
				fStackLock.Lock();
				std::stack<XBOX::VTask*>	tmpStack = fStack;
				isOk = testAssert(fStack.top() == XBOX::VTask::GetCurrent());
				fStackLock.Unlock();
				if (inDisplay)
				{
					while (!tmpStack.empty())
					{
						XBOX::VTask*	tmpTsk = (XBOX::VTask*)tmpStack.top();
						tmp += " ..";
						tmpStack.pop();
						tmp.AppendLong8(tmpTsk->GetID());
						tmp += "\n";
					}
					if (inDisplay)
					{
						DebugMsg("Check VJSGlobalObject %S\n", &tmp);
					}
				}
				return isOk;
			}

			mutable XBOX::VCriticalSection			fStackLock;
			mutable std::stack<XBOX::VTask*>		fStack;
			void									Enter() const {
				XBOX::VTask*	curtTsk = XBOX::VTask::GetCurrent();
				if (curtTsk != XBOX::VTask::GetMain())
				{
					int a = 1;
				}
				fStackLock.Lock();
				if (fStack.size())
				{
					if (curtTsk != (XBOX::VTask*)fStack.top())
					{
						std::stack<XBOX::VTask*>	tmpStack = fStack;
						VString	tmp("Enter backtrace  newtsk=");
						tmp.AppendLong8(curtTsk->GetID());
						tmp += " \n";
						while (!tmpStack.empty())
						{
							XBOX::VTask*	tmpTsk = (XBOX::VTask*)tmpStack.top();
							tmp += " ..";
							tmpStack.pop();
							tmp.AppendLong8(tmpTsk->GetID());
							tmp += "\n";
						}
						DebugMsg("Enter VJSGlobalObject %S\n", &tmp);
					}
				}
				fStack.push(curtTsk);
				fStackLock.Unlock();
			}
			void									Exit() const {
				fStackLock.Lock();
				fStack.pop();
				fStackLock.Unlock();
			}
#endif
private:
											~VJSGlobalObject();
											VJSGlobalObject( const VJSGlobalObject&);	// forbidden
											VJSGlobalObject& operator=( const VJSGlobalObject&);	// forbidden

			void							AskDeferredGarbageCollection()	{ XBOX::VInterlocked::Increment( &fGarbageCollectionPending);}

	typedef	unordered_map_VString<VRefPtr<VFile> >				MapOfIncludedFile;
	typedef	std::vector< std::pair< VRefPtr<VFile>, VTime> >	VectorOfFileModificationTime;

			JS4D::GlobalContextRef			fContext;
			VJSContextGroup*				fContextGroup;	// not retained (this is our parent)
			VJSSpecifics					fSpecifics;
			IJSRuntimeDelegate*				fRuntimeDelegate;
			MapOfIncludedFile				fIncludedFiles;
			VectorOfFileModificationTime	fIncludedFilesModificationTime;

			VCriticalSection				fUseMutex;				// mutex to guard access to fContextUseTaskID
			sLONG							fContextUseCount;		// incremented each time the associated context has been marked in use by VJSGlobalContext::Use
			VTaskID							fContextUseTaskID;		// task id of the task that marked the context in use
			sLONG							fGarbageCollectionPending;	// incremented each time a deferred garbage collection is being asked
			sLONG							fLastGarbageCollection;	// the deferred garbage collection stamp read during last garbage collect
		
			class CCompare {

			public:
		
				bool operator() (const XBOX::VString &inStringA, const XBOX::VString &inStringB) const
				{ 
					return inStringA.CompareToString(inStringB, true) == XBOX::CR_SMALLER;				
				}

			};

};


//======================================================


class XTOOLBOX_API VJSGlobalClass : public VJSClass<VJSGlobalClass, VJSGlobalObject >
{
public:
	typedef VJSClass<VJSGlobalClass, void >	inherited;

	/*
		You can add static functions and static values to this class definition before it's get registered.
		
		Beware that as soon as VJSGlobalClass::Class() is being called, calling AddStaticFunction or AddStaticValue won't have any effect.
		(an assert will remind it to you)
	*/
	static	void	AddStaticFunction( const char *inName, JS4D::ObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes);
	static	void	AddStaticValue( const char *inName, JS4D::ObjectGetPropertyCallback inGetCallback, JS4D::ObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes);

	typedef VJSObject (*ModuleConstructor) ( const VJSContext& inContext, const VString& inModuleName);
	static	void	RegisterModuleConstructor( const VString& inModuleName, ModuleConstructor inCallback);

	static	void	GetDefinition( ClassDefinition& outDefinition);
	static	void	CreateGlobalClasses();

	static	void	Initialize( const VJSParms_initialize& inParms, VJSGlobalObject* inGlobalObject);
	static	void	Finalize( const VJSParms_finalize& inParms, VJSGlobalObject* inGlobalObject);

	static	void	do_LoadText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_SaveText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

private:
	static	void	_CheckInitStaticFunctions();
	static	void	_CheckInitStaticValues();
	static	void	_PushBackStaticFunction( const char *inName, JS4D::ObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes);
	static	void	_PushBackStaticValue( const char *inName, JS4D::ObjectGetPropertyCallback inGetCallback, JS4D::ObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes);

	typedef		std::vector<JS4D::StaticFunction>	VectorOfStaticFunction;
	static VectorOfStaticFunction	sStaticFunctions;

	typedef		std::vector<JS4D::StaticValue>	VectorOfStaticValue;
	static VectorOfStaticValue		sStaticValues;

	typedef		unordered_map_VString<ModuleConstructor>	MapOfModuleConstructor;
	static	MapOfModuleConstructor	sModuleConstructors;
	
	static	bool	sGetDefinitionHasBeenCalled;	// spy to know if one can still call AddStaticFunction & AddStaticValue

	static	void	do_trace( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);
	static	void	do_displayNotification( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);
	static	void	do_include( VJSParms_callStaticFunction& inParms, VJSGlobalObject* inContext);
	static	void	do_testMe( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);

	static	void	do_GetProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext);
	static	void	do_ProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext);
	static	void	do_BinaryStream(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_XmlToJSON(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_JSONToXml(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_loadImage(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_garbageCollect(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_isoToDate(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_dateToIso(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_AtomicSection(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_SyncEvent(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_GetURLPath(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_GetURLQuery(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_GenerateUUID(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_SetCurrentUser(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static void		do_GetFileSystems (VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject);

	static void		do_GetThreads(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject);

	static void		do_RequireNative (VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inGlobalObject);
	static void		do_RequireFile (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *);
	static void		do_ImportScripts (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);	
};

END_TOOLBOX_NAMESPACE

#endif

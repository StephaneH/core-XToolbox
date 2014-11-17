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
#ifndef __VJS_WORKER_PROXY__
#define __VJS_WORKER_PROXY__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSWorker;
class VJSMessagePort;
class VJSWebSocketObject;

// WorkerLocation objects are never to be created directly (global object attribute).

class XTOOLBOX_API VJSWorkerLocationObject : public XBOX::VObject
{
public:

	XBOX::VString	fHRef;
	XBOX::VString	fProtocol;
	XBOX::VString	fHost;
	XBOX::VString	fHostName;
	XBOX::VString	fPort;
	XBOX::VString	fPathName;
	XBOX::VString	fSearch;
	XBOX::VString	fHash;	

	VJSWorkerLocationObject (const XBOX::VString &inURL);
};

class XTOOLBOX_API VJSWorkerLocationClass : public XBOX::VJSClass<VJSWorkerLocationClass, VJSWorkerLocationObject>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);

private:

	typedef XBOX::VJSClass<VJSWorkerLocationClass, VJSWorkerLocationObject> inherited;

	static void	_Initialize (const XBOX::VJSParms_initialize &inParms, VJSWorkerLocationObject *inWorkerLocation);
};

// Web worker proxy object.

class XTOOLBOX_API VJSWebWorkerObject : public XBOX::VObject
{
public:

	VJSWorker					*GetWorker ()	{	return fWorker;	}

private:

friend class VJSDedicatedWorkerClass;
friend class VJSSharedWorkerClass;

								VJSWebWorkerObject (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort, VJSWorker *inWorker);
	virtual						~VJSWebWorkerObject ();

	// Create a web worker. inWebSocket is only considered for dedicated worker and if not null.

	static VJSWebWorkerObject	*_CreateWorker (XBOX::VJSContext &inParentContext, VJSWorker *inOutsideWorker, 
												bool inReUseContext, VJSWebSocketObject *inWebSocket,
												const XBOX::VString &inUrl, bool inIsDedicated, const XBOX::VString &inName = "");

	VJSMessagePort				*fOnMessagePort;	// Message port for "onmessage" and postMessage().
	VJSMessagePort				*fOnErrorPort;		// Unidirectional port for "onerror".
	VJSWorker					*fWorker;			// This is the proxy for this (dedicated or shared) worker.
};

// Dedicated web worker. 

class XTOOLBOX_API VJSDedicatedWorkerClass : public XBOX::VJSClass<VJSDedicatedWorkerClass, VJSWebWorkerObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	MakeConstructor (XBOX::VJSContext inContext);

private:
	static	JS4D::StaticFunction sConstrFunctions[];
	typedef XBOX::VJSClass<VJSDedicatedWorkerClass, VJSWebWorkerObject>	inherited;

	static void	_Construct (VJSParms_construct &ioParms);
	static void _Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebWorkerObject *inDedicatedWorker);

	static void	_terminate (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebWorkerObject *inDedicatedWorker);
	static void	_postMessage (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebWorkerObject *inDedicatedWorker);

	static void	_GetNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *);
};

// Shared web worker. 

class XTOOLBOX_API VJSSharedWorkerClass : public XBOX::VJSClass<VJSSharedWorkerClass, VJSWebWorkerObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	MakeConstructor (XBOX::VJSContext inContext);
	
private:
	static	JS4D::StaticFunction sConstrFunctions[];
	typedef XBOX::VJSClass<VJSSharedWorkerClass, VJSWebWorkerObject>	inherited;
	
	static void	_Construct (VJSParms_construct &ioParmss);
	static void _Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebWorkerObject *inSharedWorker);    

	static void	_IsRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void	_GetNumberRunning (XBOX::VJSParms_callStaticFunction &ioParms, void *);
};

END_TOOLBOX_NAMESPACE

#endif
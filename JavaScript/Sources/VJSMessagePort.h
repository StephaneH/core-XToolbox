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
#ifndef __VJS_MESSAGE_PORT__
#define __VJS_MESSAGE_PORT__

#include <list>
#include <vector>

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSWorker;
class VJSStructuredClone;

// Message ports are used to exchange data between two javascript workers (bidirectional), or to implement callbacks (unidirectional). 
// They are automatically added to appropriate message port list(s) when created, and removed when closed. An unidirectional port can 
// only receive, not send. 

class XTOOLBOX_API VJSMessagePort : public XBOX::VObject, public XBOX::IRefCountable
{
public:

	// The VJSMessagePort will free itself (release) when both workers have closed.

	// The "outside" worker is the one which created the message port, the "inside" being the invoked worker.
	// This follow the web workers specification naming, where the "outside" is the invoker of the dedicated 
	// or shared worker, and the "inside" the actual invoked/created worker.
	
	static VJSMessagePort	*Create (VJSWorker *inOutsideWorker, VJSWorker *inInsideWorker, const VJSContext& inContext);

	// Create an unidirectional message port, can only receive messages. To be used internally.

	static VJSMessagePort	*Create (VJSWorker *inWorker, XBOX::VJSObject &inObject, const XBOX::VString &inCallbackName);

	// Close port. It is then forbidden to use a closed port (refcountable released).
	// If both workers are closed, the VJSMessagePort will free itself.

	void					Close (VJSWorker *inWorker);

	// Return true if inWorker is the "inside" worker of this message port.
	
	bool					IsInside (VJSWorker *inWorker);

	// Get a pointer to the other worker of message port or check if it has closed.
	// For unidirectional port, the other worker is always the receiving worker.
	// Note that it is not a retain.
		
	VJSWorker				*GetOther (VJSWorker *inWorker);
	VJSWorker				*GetOther ();
	bool					IsOtherClosed (VJSWorker *inWorker);
	bool					IsOtherClosed ();	

	// Set object containing the callback (as an attribute). For dedicated workers, it is the global object. 
	// For shared workers, a MessagePort. And for dedicated workers parents, it is the Worker proxy object.
	
	void					SetObject (VJSWorker *inWorker, XBOX::VJSObject &inObject);

 	// Set callback name. 

	void					SetCallbackName (VJSWorker *inWorker, const XBOX::VString &inCallbackName);

	// Retrieve callback object (from javascript attribute fObject.fCallbackName).

	XBOX::VJSObject			GetCallback (VJSWorker *inWorker, XBOX::VJSContext &inContext);

	// Post a message to a worker. If the target end's port is closed, the message is silently dropped.
	// inMessage memory is freed by inTargetWorker's event handler or PostMessage() if dropped. Message
	// is transfered as a "structured clone".

	void					PostMessage (VJSWorker *inTargetWorker, VJSStructuredClone *inMessage);

	// Post an error event to a worker, works like PostMessage().

	void					PostError (VJSWorker *inTargetWorker, 
										const XBOX::VString &inMessage, 
										const XBOX::VString &inFileName, 
										sLONG inLineNumber);

	// Implementation of the postMessage() method (web worker proxies, message port objects, or global for 
	// dedicated workers).
	
	static void				PostMessageMethod (XBOX::VJSParms_callStaticFunction &ioParms, VJSMessagePort *inMessagePort);

private: 

friend class VJSMessagePortClass;

	VJSWorker				*fOutsideWorker;			// Set to NULL if unidirectional worker.
	bool					fOutsideClosingFlag;		// Set to true if unidirectional worker.
	XBOX::VJSObject			fOutsideObject;
	XBOX::VString			fOutsideCallbackName;
	
	VJSWorker				*fInsideWorker;				
	bool					fInsideClosingFlag;	
	XBOX::VJSObject			fInsideObject;
	XBOX::VString			fInsideCallbackName;

							VJSMessagePort (const VJSContext& inContext);
	virtual					~VJSMessagePort ();
};

// Implement the MessagePort class.

class XTOOLBOX_API VJSMessagePortClass : public XBOX::VJSClass<VJSMessagePortClass, VJSMessagePort>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);

private:

	typedef XBOX::VJSClass<VJSMessagePort, VJSMessagePort>	inherited;
	
	static void	_postMessage (XBOX::VJSParms_callStaticFunction &ioParms, VJSMessagePort *inMessagePort);
};

END_TOOLBOX_NAMESPACE

#endif
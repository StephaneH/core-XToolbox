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
#ifndef __VJS_NET_SOCKET__
#define __VJS_NET_SOCKET__

#include <list>

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSWorker.h"
#include "VJSEventEmitter.h"
#include "VJSBuffer.h"

#include "ServerNet/VServerNet.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSNetSocketObject : public VJSEventEmitter
{
friend class VJSNetSocketClass;

public:

	enum {
	
		eTYPE_TCP4,
		eTYPE_TCP6
		
	};

	struct SPacket {

		uBYTE	*fBuffer;
		sLONG	fLength;

	};

	static const uLONG				kReadBufferSize	= 4096;

									VJSNetSocketObject (bool inIsSynchronous, sLONG inType, bool inAllowHalfOpen,const XBOX::VJSContext& inContext);
	virtual							~VJSNetSocketObject ();

	static XBOX::VTCPSelectIOPool	*GetSelectIOPool ();

	static XBOX::VCriticalSection	sMutex;
	static XBOX::VTCPSelectIOPool	*sSelectIOPool;			// Do not read directly, always use GetSelectIOPool().

	XBOX::VCriticalSection			fMutex;

	sLONG							fType;
	XBOX::VTCPEndPoint				*fEndPoint;
	bool							fIsSynchronous;
	bool							fAllowHalfOpen;
	uLONG							fTimeOut;
	CharSet							fEncoding;		// Default is XBOX::VTC_UNKNOWN: do not encode, consider data as binary.
	
	uLONG8							fBytesRead;
	uLONG8							fBytesWritten;

	XBOX::VJSObject					fObject;
	VJSWorker						*fWorker;

	// Doesn't do a retain, it is up to caller to do that.

	XBOX::VTCPEndPoint				*GetEndPoint ()	{	return fEndPoint;	}

	// Asynchronous sockets allow data reading to be paused. Implementation is to read and save available data, but not queue "data" events.
	// When resume() is called, this will queue "data" events for all buffered data.

	bool							fIsPaused;
	std::list<SPacket>				fBufferedData;

	// If not closed yet, force closing of the socket. Can be called repeatedly.

	void							ForceClose ();

	// Return encoding to use, default is UTF-8.

	CharSet							GetEncoding ()	{	return fEncoding;	}

	// Callback for select I/O.

	static bool						_ReadCallback (Socket inRawSocket, VEndPoint* inEndPoint, void *inData, sLONG inErrorCode);	
	
	// Socket reading function used by _ReadCallback().

	bool							_ReadSocket ();

	// To not lose data, it is buffered.

	void							_FlushBufferedData();
};

class XTOOLBOX_API VJSNetSocketClass : public XBOX::VJSClass<VJSNetSocketClass, VJSNetSocketObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
	static void		Construct (VJSParms_construct &ioParms);

	// Internal use to implement connection functions.

	static void		Connect (XBOX::VJSParms_withArguments &ioParms, XBOX::VJSObject	&inSocket, bool inIsSSL);	

	// Create a new Socket object from a given endpoint in an asynchronous event.

	static void		NewInstance (XBOX::VJSContext inContext, 
								VJSEventEmitter *inEventEmitter, 
								XBOX::VTCPEndPoint *inEndPoint, 
								XBOX::VJSValue *outValue);

private:

	typedef XBOX::VJSClass<VJSNetSocketClass, VJSNetSocketObject>	inherited;

	static void		_Initialize (const XBOX::VJSParms_initialize &inParms, VJSNetSocketObject *inSocket);
	static void		_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetSocketObject *inSocket);
	static void		_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSNetSocketObject *inSocket);

	static void		_connect (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setEncoding (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setSecure (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_end (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_destroy (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_pause (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_resume (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setTimeout (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setNoDelay (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setKeepAlive (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
};

class XTOOLBOX_API VJSNetSocketSyncClass : public XBOX::VJSClass<VJSNetSocketSyncClass, VJSNetSocketObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
	static void		Construct (VJSParms_construct &ioParms);

	// Internal use to implement connection functions.

	static void		Connect (XBOX::VJSParms_withArguments &ioParms, XBOX::VJSObject &inSocket, bool inIsSSL);	

private:

	typedef XBOX::VJSClass<VJSNetSocketSyncClass, VJSNetSocketObject>	inherited;

	static void		_Initialize (const XBOX::VJSParms_initialize &inParms, VJSNetSocketObject *inSocket);
	static void		_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetSocketObject *inSocket);
	static void		_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSNetSocketObject *inSocket);

	static void		_connect (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);	
	static void		_setEncoding (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setSecure (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_read (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_write (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_end (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_destroy (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setTimeout (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setNoDelay (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_setKeepAlive (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
	static void		_address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetSocketObject *inSocket);
};

END_TOOLBOX_NAMESPACE

#endif
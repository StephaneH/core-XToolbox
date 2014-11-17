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
#ifndef __VJS_NET_SERVER__
#define __VJS_NET_SERVER__

#include "VJSClass.h"
#include "VJSValue.h"

#include "ServerNet/VServerNet.h"
#include "VJSEventEmitter.h"

BEGIN_TOOLBOX_NAMESPACE

// connections and maxConnections attributes are not supported.

class XTOOLBOX_API VJSNetServerObject : public VJSEventEmitter
{
friend class VJSNetServerClass;
friend class VJSNetServerSyncClass;	

public:

									VJSNetServerObject (bool inIsSynchronous, bool inAllowHalfOpen = true);
	virtual							~VJSNetServerObject ();
	
	bool							fIsSynchronous;	
	bool							fAllowHalfOpen;
	bool							fIsSSL;
	
	// Currently listened address and port.

	XBOX::VString					fAddress;		// Correctly formatted in either IPv4 or IPv6 format.
	sLONG							fPort;

	// Key and certificate.

	XBOX::VString					fKey;
	XBOX::VString					fCertificate;

	XBOX::VTCPConnectionListener	*fConnectionListener;	// Asynchronous only.
	XBOX::VSockListener				*fSockListener;			// Synchronous only.

	// Set SSL key and certificate, this is the way to turn SSL on.

	void							SetKeyAndCertificate (const XBOX::VString &inKey, const XBOX::VString &inCertificate);
	
	// If listening, stop doing so. Otherwise, do nothing.

	void							Close ();

	// close() and address() functions, shared by both net.Server and net.ServerSync objects.

	static void						Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);
	static void						Address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);
};

class XTOOLBOX_API VJSConnectionHandler : public VConnectionHandler
{
friend class VJSConnectionHandlerFactory;

						VJSConnectionHandler (VJSWorker *inWorker, VJSNetServerObject *inServerObject, bool inIsSSL);
	virtual				~VJSConnectionHandler ();

	// Connection handler doesn't do anything, except to queue an event when SetEndPoint() is called.

	XBOX::VError		SetEndPoint (XBOX::VEndPoint *inEndPoint);

	bool				CanShareWorker ()			{	return true;	}

	enum E_WORK_STATUS	Handle (VError &outError)	{	return eWS_DONE;	}

	int					GetType ()					{	return 'JSNT';	}

	XBOX::VError		Stop ()						{	return XBOX::VE_OK;	}

	VJSWorker			*fWorker;
	VJSNetServerObject	*fServerObject;
	bool				fIsSSL;
};

class XTOOLBOX_API VJSConnectionHandlerFactory : public VTCPConnectionHandlerFactory
{
public:

								VJSConnectionHandlerFactory (VJSWorker *inWorker, VJSNetServerObject *inServerObject, bool inIsSSL);

	XBOX::VConnectionHandler	*CreateConnectionHandler (XBOX::VError &outError);

	int							GetType ()	{	return 'JSNT';	}

	XBOX::VError				AddNewPort (PortNumber iPort);
	XBOX::VError				GetPorts (std::vector<PortNumber> &oPorts);

private:

	virtual						~VJSConnectionHandlerFactory ();

	XBOX::VCriticalSection		fMutex;

	sLONG						fPort;

	VJSWorker					*fWorker;
	VJSNetServerObject			*fServerObject;
	bool						fIsSSL;

	VJSConnectionHandler		*fConnectionHandler;
};

class XTOOLBOX_API VJSNetServerClass : public XBOX::VJSClass<VJSNetServerClass, VJSNetServerObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
	static void		Construct( XBOX::VJSParms_construct &ioParms);
	static void		Initialize(const XBOX::VJSParms_initialize &inParms, VJSNetServerObject *inServer);

private:

	typedef XBOX::VJSClass<VJSNetServerClass, VJSNetServerObject>	inherited;

	static void		_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetServerObject *inServer);

	static void		_listen (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);		
	static void		_pause (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);	
	static void		_close (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);	
};

class XTOOLBOX_API VJSNetServerSyncClass : public XBOX::VJSClass<VJSNetServerSyncClass, VJSNetServerObject>
{
public:

	static void		GetDefinition (ClassDefinition &outDefinition);
	static void		Construct (XBOX::VJSParms_construct &ioParms);
	static void		Initialize(const XBOX::VJSParms_initialize &inParms, VJSNetServerObject *inServer) { ; }

private:

	typedef XBOX::VJSClass<VJSNetServerSyncClass, VJSNetServerObject>	inherited;

	static void		_Finalize (const XBOX::VJSParms_finalize &inParms, VJSNetServerObject *inServer);

	static void		_listen (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);		
	static void		_accept (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);	
	static void		_close (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);
	static void		_address (XBOX::VJSParms_callStaticFunction &ioParms, VJSNetServerObject *inServer);
};

END_TOOLBOX_NAMESPACE

#endif
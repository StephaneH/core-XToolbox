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
#include "ServerNetTypes.h"

#include "IConnectionListener.h"
#include "VSockListener.h"

#include <vector>


#ifndef __SNET_SERVER_BASE__
#define __SNET_SERVER_BASE__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VServer : public VObject, public IRefCountable
{
	public :
	
	VServer ( );
	virtual ~VServer ( );
	
	virtual VError AddConnectionListener ( IConnectionListener* inListener );
	virtual VError GetPublishedPorts ( std::vector<PortNumber>& outPorts );
	
	virtual VError Start ( );
	virtual bool IsRunning ( );
	virtual VError Stop ( );
	
	protected :
	
	VCriticalSection								m_vmtxListenerProtector;
	std::vector<IConnectionListener*>				m_vctrListeners;
};


class VWorkerPool;

class XTOOLBOX_API VTCPConnectionListener : public IConnectionListener, public VTask
{
	public :
	
	VTCPConnectionListener ( IRequestLogger* inRequestLogger = 0 );
	virtual ~VTCPConnectionListener ( );
	
	virtual VError StartListening ( );
	virtual bool IsListening ( );
	virtual VError StopListening ( );
	
	virtual VError AddConnectionHandlerFactory ( VConnectionHandlerFactory* inFactory );
	virtual VError GetPorts ( std::vector<PortNumber>& outPorts );
	virtual VError AddWorkerPool ( VWorkerPool* inPool );
	virtual VError AddSelectIOPool ( VTCPSelectIOPool* inPool );

	// SSL key and certificate can be set using two methods: Specify paths of both or specify them directly.
	
	virtual void SetSSLCertificatePaths (const VFilePath& inCertificatePath, const VFilePath& inKeyPath);
	virtual void SetSSLKeyAndCertificate ( VString const & inCertificate, VString const &inKey);
	
	protected :
	
	virtual Boolean DoRun ( );
	
	virtual void DeInit ( );
	
	IRequestLogger*										fRequestLogger;
	std::vector<VTCPConnectionHandlerFactory*>			fFactories;
	VSockListener*										fSockListener;
	VWorkerPool*										fWorkerPool;
	VTCPSelectIOPool*									fSelectIOPool;
	VFilePath											fCertificatePath;
	VFilePath											fKeyPath;
	VString												fCertificate;
	VString												fKey;
};


END_TOOLBOX_NAMESPACE

#endif

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

#include <queue>

#ifndef __SNET_CONNECTION_HANDLER_FACTORY__
#define __SNET_CONNECTION_HANDLER_FACTORY__


BEGIN_TOOLBOX_NAMESPACE


/*	IMPORTANT
 User of this interface must be aware that when they use the workerpool for their tasks,
 the TaskKindData is handled by VExclusiveWorker, which just calls SetTaskKindData(this).
 => Do not change the TaskKindData()
 */
class XTOOLBOX_API VConnectionHandler : public VObject, public IRefCountable
{
public :
	
	virtual VError SetEndPoint ( VEndPoint* inEndPoint ) = 0;
	
	/* Should return true if this VConnectionHandler can share one VWorker
	 with other VConnectionHandler-s. Should return false if this VConnectionHandler
	 requires exclusive VWorker. */
	virtual bool CanShareWorker ( ) = 0;
	
	enum E_WORK_STATUS
	{
		eWS_DONE,
		eWS_NOT_DONE
		/* eWS_IO_WAITING */
	};
	
	virtual enum E_WORK_STATUS Handle ( VError& outError ) = 0;
	
	uWORD _GetLoadRedistributionCount ( ) { return m_nLoadRedistributionCount; }
	void _IncrementLoadRedistributionCount ( ) { m_nLoadRedistributionCount++; }
	void _ResetRedistributionCount ( ) { m_nLoadRedistributionCount = 0; }
	
	/* Returns the type of a handler. Must be different for different kinds of handlers. */
	virtual int GetType ( ) = 0;
	virtual VError Stop ( ) = 0;
	
	
private:
	
	short							m_nLoadRedistributionCount;
};


class XTOOLBOX_API VConnectionHandlerFactory : public VObject, public IRefCountable
{
public :	
	
	virtual VConnectionHandler* CreateConnectionHandler ( VError& outError ) = 0;
	
	/* Returns the type of a handler that this factory can produce. */
	virtual int GetType ( ) = 0;
};


/* A synchronized queue of connection handlers. */
class XTOOLBOX_API VConnectionHandlerQueue : public VObject
{
public :
	
	VConnectionHandlerQueue ( );
	~VConnectionHandlerQueue ( );
	
	VError Push ( VConnectionHandler* inConnectionHandler );
	VConnectionHandler* Pop ( VError* ioError );
	void ReleaseAll ( );

	
private :
	
	VCriticalSection*						m_vcsQueueProtector;
	std::queue<VConnectionHandler*>			m_qConnectionHandlers;
};


class XTOOLBOX_API VTCPConnectionHandlerFactory : public VConnectionHandlerFactory
{
public :

	VTCPConnectionHandlerFactory ( ) { fIsSSL = false; }

	virtual VError AddNewPort ( PortNumber iPort ) = 0; /* AddPort is an already defined symbol */
	virtual VError GetPorts ( std::vector<PortNumber>& oPorts ) = 0;

	virtual void SetIP (const VString& inIP ) { fIP = inIP; }
	virtual VString GetIP ( ) { return fIP; }
	
	virtual void SetIsSSL ( bool bIsSSL ) { fIsSSL = bIsSSL; }
	virtual bool IsSSL ( ) { return fIsSSL; }
	
	protected :

	VString											fIP;
	
	bool											fIsSSL;
};	


END_TOOLBOX_NAMESPACE


#endif

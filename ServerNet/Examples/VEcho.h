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
#ifndef __VECHO__
#define __VECHO__

#include "VTCPServer.h"


BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VEchoSharedConnectionHandler : public VConnectionHandler
{
	public :

		enum	{ Handler_Type = 'ECHO' };

		VEchoSharedConnectionHandler ( );
		~VEchoSharedConnectionHandler ( );

		virtual VError SetEndPoint ( VEndPoint* inEndPoint );
		virtual bool CanShareWorker ( ) { return true; }

		virtual enum E_WORK_STATUS Handle ( VError& outError );

		virtual int GetType ( ) { return Handler_Type; }
		virtual VError Stop ( ) { return VE_OK; /* Do nothing for the sake of simplicity of this example. */ };

	protected :

		VEndPoint*						m_EndPoint;

		char*							m_szchBuffer;
		short							m_nMaxBufferSize;
		short							m_nCurrentBufferSize;

		virtual void DumpDebugInfo ( );
};


class XTOOLBOX_API VEchoSharedConnectionHandlerFactory : public VTCPConnectionHandlerFactory
{
	public :

		VEchoSharedConnectionHandlerFactory ( );
		~VEchoSharedConnectionHandlerFactory ( );

		virtual VConnectionHandler* CreateConnectionHandler ( VError& outError );

		virtual VError AddNewPort ( short iPort );
		virtual VError GetPorts ( std::vector<short>& oPorts );

		virtual int GetType ( ) { return VEchoSharedConnectionHandler::Handler_Type; }
};



class XTOOLBOX_API VEchoExclusiveConnectionHandler : public VConnectionHandler
{
	public :

		enum	{ Handler_Type = 'ECHO' };

		VEchoExclusiveConnectionHandler ( );
		~VEchoExclusiveConnectionHandler ( );

		virtual VError SetEndPoint ( VEndPoint* inEndPoint );
		virtual bool CanShareWorker ( ) { return false; }

		virtual enum E_WORK_STATUS Handle ( VError& outError );

		virtual int GetType ( ) { return Handler_Type; }
		virtual VError Stop ( ) { return VE_OK; /* Do nothing for the sake of simplicity of this example. */ };

	protected :

		VEndPoint*						m_EndPoint;

		virtual void DumpDebugInfo ( );
};

class XTOOLBOX_API VEchoExclusiveConnectionHandlerFactory : public VTCPConnectionHandlerFactory
{
	public :

		VEchoExclusiveConnectionHandlerFactory ( );
		~VEchoExclusiveConnectionHandlerFactory ( );

		virtual VConnectionHandler* CreateConnectionHandler ( VError& outError );

		virtual VError AddNewPort ( short iPort );
		virtual VError GetPorts ( std::vector<short>& oPorts );

		virtual int GetType ( ) { return VEchoExclusiveConnectionHandler::Handler_Type; }
};


END_TOOLBOX_NAMESPACE

#endif


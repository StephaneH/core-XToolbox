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
#include "VServerNetPrecompiled.h"

#include "VEcho.h"


BEGIN_TOOLBOX_NAMESPACE

VEchoSharedConnectionHandler::VEchoSharedConnectionHandler ( )
{
	m_EndPoint = NULL;

	m_nMaxBufferSize = 4;
	m_nCurrentBufferSize = 0;
	m_szchBuffer = new char [ m_nMaxBufferSize ];
}

VEchoSharedConnectionHandler::~VEchoSharedConnectionHandler ( )
{
	if ( m_EndPoint )
		delete m_EndPoint;
	if ( m_szchBuffer )
		delete [ ] m_szchBuffer;
}

VError VEchoSharedConnectionHandler::SetEndPoint ( VEndPoint* inEndPoint )
{
	m_EndPoint = inEndPoint;

	return VE_OK;
}

enum VConnectionHandler::E_WORK_STATUS VEchoSharedConnectionHandler::Handle ( VError& outError )
{
	outError = VE_OK;

	uLONG				nRead = m_nMaxBufferSize - m_nCurrentBufferSize;
	m_EndPoint-> Read ( m_szchBuffer + m_nCurrentBufferSize, &nRead );
	m_nCurrentBufferSize += nRead;
	if ( m_nCurrentBufferSize != m_nMaxBufferSize )
		return VConnectionHandler::eWS_NOT_DONE;

	m_szchBuffer [ 2 ] = '\0';
	int					nDelay = atoi ( m_szchBuffer );
	if ( nDelay == 99 )
		return VConnectionHandler::eWS_DONE;

	VTask::GetCurrent ( )-> Sleep ( nDelay * 1000 );

	DumpDebugInfo ( );
	m_nCurrentBufferSize = 0;

	return VConnectionHandler::eWS_NOT_DONE;
}

void VEchoSharedConnectionHandler::DumpDebugInfo ( )
{
	uLONG				nSize = 23;
	m_EndPoint-> Write ( ( void* ) "Delayed ECHO called by ", &nSize );
	VString				vstrTaskName;
	VTask::GetCurrent ( )-> GetName ( vstrTaskName );
	nSize = vstrTaskName. GetLength ( ) + 1;
	char*				szchTaskName = new char [ nSize ];
	vstrTaskName. ToCString ( szchTaskName, nSize );
	m_EndPoint-> Write ( ( void* ) szchTaskName, &nSize );
	nSize = 2;
	m_EndPoint-> Write ( ( void* ) "\r\n", &nSize );
}



VEchoSharedConnectionHandlerFactory::VEchoSharedConnectionHandlerFactory ( )
{
	;
}

VEchoSharedConnectionHandlerFactory::~VEchoSharedConnectionHandlerFactory ( )
{
	;
}

VConnectionHandler* VEchoSharedConnectionHandlerFactory::CreateConnectionHandler ( VError& outError )
{
	outError = VE_OK;

	return new VEchoSharedConnectionHandler ( );
}

VError VEchoSharedConnectionHandlerFactory::AddNewPort ( short iPort )
{
	VError			vError = VE_UNIMPLEMENTED;
	iPort = iPort * 2;

	return vError;
}

VError VEchoSharedConnectionHandlerFactory::GetPorts ( std::vector<short>& oPorts )
{
	oPorts. push_back ( 2525 );

	return VE_OK;
}



VEchoExclusiveConnectionHandler::VEchoExclusiveConnectionHandler ( )
{
	m_EndPoint = NULL;
}

VEchoExclusiveConnectionHandler::~VEchoExclusiveConnectionHandler ( )
{
	if ( m_EndPoint )
		delete m_EndPoint;
}

VError VEchoExclusiveConnectionHandler::SetEndPoint ( VEndPoint* inEndPoint )
{
	m_EndPoint = inEndPoint;

	return VE_OK;
}

enum VConnectionHandler::E_WORK_STATUS VEchoExclusiveConnectionHandler::Handle ( VError& outError )
{
	outError = VE_OK;

	char				szchBuffer [ 4 ];
	m_EndPoint-> ReadExactly ( szchBuffer, 4 );
	szchBuffer [ 2 ] = '\0';

	int					nDelay = atoi ( szchBuffer );
	if ( nDelay == 99 )
		return VConnectionHandler::eWS_DONE;

	VTask::GetCurrent ( )-> Sleep ( nDelay * 1000 );

	DumpDebugInfo ( );

	return VConnectionHandler::eWS_DONE;
}

void VEchoExclusiveConnectionHandler::DumpDebugInfo ( )
{
	uLONG				nSize = 23;
	m_EndPoint-> Write ( ( void* ) "Delayed ECHO called by ", &nSize );
	VString				vstrTaskName;
	VTask::GetCurrent ( )-> GetName ( vstrTaskName );
	nSize = vstrTaskName. GetLength ( ) + 1;
	char*				szchTaskName = new char [ nSize ];
	vstrTaskName. ToCString ( szchTaskName, nSize );
	m_EndPoint-> Write ( ( void* ) szchTaskName, &nSize );
	nSize = 2;
	m_EndPoint-> Write ( ( void* ) "\r\n", &nSize );
}


VEchoExclusiveConnectionHandlerFactory::VEchoExclusiveConnectionHandlerFactory ( )
{
	;
}

VEchoExclusiveConnectionHandlerFactory::~VEchoExclusiveConnectionHandlerFactory ( )
{
	;
}

VConnectionHandler* VEchoExclusiveConnectionHandlerFactory::CreateConnectionHandler ( VError& outError )
{
	outError = VE_OK;

	return new VEchoExclusiveConnectionHandler ( );
}

VError VEchoExclusiveConnectionHandlerFactory::AddNewPort ( short iPort )
{
	VError			vError = VE_UNIMPLEMENTED;
	iPort = iPort * 2;

	return vError;
}

VError VEchoExclusiveConnectionHandlerFactory::GetPorts ( std::vector<short>& oPorts )
{
	oPorts. push_back ( 5252 );

	return VE_OK;
}

END_TOOLBOX_NAMESPACE


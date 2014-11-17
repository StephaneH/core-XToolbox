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


#include "VConnectionHandlerFactory.h"


BEGIN_TOOLBOX_NAMESPACE


VConnectionHandlerQueue::VConnectionHandlerQueue ( ) :
m_qConnectionHandlers ( )
{
	m_vcsQueueProtector = new VCriticalSection ( );
}

VConnectionHandlerQueue::~VConnectionHandlerQueue ( )
{
	if ( m_vcsQueueProtector )
		delete m_vcsQueueProtector;
}

VError VConnectionHandlerQueue::Push ( VConnectionHandler* inConnectionHandler )
{
	if ( !m_vcsQueueProtector-> Lock ( ) )
		return VE_SRVR_FAILED_TO_SYNC_LOCK;
	
	m_qConnectionHandlers. push ( inConnectionHandler );
	
	m_vcsQueueProtector-> Unlock ( );
	
	return VE_OK;
}

VConnectionHandler* VConnectionHandlerQueue::Pop ( VError* ioError )
{
	if ( !m_vcsQueueProtector-> Lock ( ) )
	{
		*ioError = VE_SRVR_FAILED_TO_SYNC_LOCK;
		
		return NULL;
	}
	
	VConnectionHandler*			vcHandler = NULL;
	if ( m_qConnectionHandlers. size ( ) > 0 )
	{
		vcHandler = m_qConnectionHandlers. front ( );
		m_qConnectionHandlers. pop ( );
	}
	
	*ioError = VE_OK;
	
	m_vcsQueueProtector-> Unlock ( );
	
	return vcHandler;
}

void VConnectionHandlerQueue::ReleaseAll ( )
{
	m_vcsQueueProtector-> Lock ( );
	
	VConnectionHandler*			vcHandler = NULL;
	while ( m_qConnectionHandlers. size ( ) > 0 )
	{
		vcHandler = m_qConnectionHandlers. front ( );
		m_qConnectionHandlers. pop ( );
		vcHandler-> Release ( );
	}
	
	m_vcsQueueProtector-> Unlock ( );
}


END_TOOLBOX_NAMESPACE

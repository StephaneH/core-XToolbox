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

#include "VNoRemoteDebugger.h"


USING_TOOLBOX_NAMESPACE



VNoRemoteDebugger*					VNoRemoteDebugger::sDebugger=NULL;




VNoRemoteDebugger::VNoRemoteDebugger()
{
	if (sDebugger)
	{
		xbox_assert(false);
	}
}



VNoRemoteDebugger::~VNoRemoteDebugger()
{
	xbox_assert(false);

}





bool VNoRemoteDebugger::HasClients()
{
	return false;

}




WAKDebuggerType_t VNoRemoteDebugger::GetType()
{
	return NO_DEBUGGER_TYPE;
}

int	VNoRemoteDebugger::StartServer()
{
	return 0;
}
int	VNoRemoteDebugger::StopServer()
{
	return 0;
}
short VNoRemoteDebugger::GetServerPort()
{
	xbox_assert(false);
	return -1;
}


bool VNoRemoteDebugger::IsSecuredConnection()
{
	xbox_assert(false);
	return false;
}


void VNoRemoteDebugger::SetInfo( IWAKDebuggerInfo* inInfo )
{
	xbox_assert(false);
}


void VNoRemoteDebugger::GetStatus(
			bool&			outStarted,
			bool&			outConnected,
			long long&		outDebuggingEventsTimeStamp,
			bool&			outPendingContexts)
{
	outStarted = true;
	outPendingContexts = false;
	outConnected = false;
	outDebuggingEventsTimeStamp = -1;
}


void VNoRemoteDebugger::SetSettings( IWAKDebuggerSettings* inSettings )
{
}

void VNoRemoteDebugger::Trace(	OpaqueDebuggerContext	inContext,
								const void*				inString,
								int						inSize,
								WAKDebuggerTraceLevel_t inTraceLevel )
{

}



VNoRemoteDebugger* VNoRemoteDebugger::Get()
{
	if ( sDebugger == 0 )
	{
		sDebugger = new VNoRemoteDebugger();
	}
	return sDebugger;
}



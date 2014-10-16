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
#ifndef __VNOREMOTE_DEBUGGER_H__
#define __VNOREMOTE_DEBUGGER_H__


#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"


#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif





class JSDEBUGGER_API VNoRemoteDebugger :
						public IRemoteDebuggerServer,
						public XBOX::VObject
{
public:

	static VNoRemoteDebugger*			Get();

private:
										VNoRemoteDebugger();
	virtual								~VNoRemoteDebugger();


	// from IChromeDebuggerServer
	virtual WAKDebuggerType_t			GetType();
	virtual int							StartServer();
	virtual int							StopServer();
	virtual short						GetServerPort();
	virtual	bool						IsSecuredConnection();
	virtual void						SetSettings( IWAKDebuggerSettings* inSettings );
	virtual void						SetInfo( IWAKDebuggerInfo* inInfo );
	virtual bool						HasClients();

	virtual void						GetStatus(
											bool&			outStarted,
											bool&			outConnected,
											long long&		outDebuggingEventsTimeStamp,
											bool&			outPendingContexts);

	virtual void						Trace(	OpaqueDebuggerContext	inContext,
												const void*				inString,
												int						inSize,
												WAKDebuggerTraceLevel_t inTraceLevel = WAKDBG_ERROR_LEVEL );

	static								VNoRemoteDebugger*				sDebugger;


};

#endif


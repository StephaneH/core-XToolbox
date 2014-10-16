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
#ifndef __JSW_DEBUGGER_ERRORS__
#define __JSW_DEBUGGER_ERRORS__


namespace CrossfireKeys
{
	CREATE_BAGKEY ( seq );
	CREATE_BAGKEY ( request_seq );
	CREATE_BAGKEY ( id );
	CREATE_BAGKEY ( type );
	CREATE_BAGKEY ( event );
	CREATE_BAGKEY ( context_id );
	CREATE_BAGKEY ( data );
	CREATE_BAGKEY ( url );
	CREATE_BAGKEY ( function );
	CREATE_BAGKEY ( line );
	CREATE_BAGKEY ( href );
	CREATE_BAGKEY ( body );
	CREATE_BAGKEY ( frame );
	CREATE_BAGKEY ( frames );
	CREATE_BAGKEY ( preview );
	CREATE_BAGKEY ( command );
	CREATE_BAGKEY ( handle );
	CREATE_BAGKEY ( arguments );
	CREATE_BAGKEY ( success );
	CREATE_BAGKEY ( includeSource );
	CREATE_BAGKEY ( script );
	CREATE_BAGKEY ( source );
	CREATE_BAGKEY ( result );
	CREATE_BAGKEY ( expression );
	CREATE_BAGKEY ( target );
	CREATE_BAGKEY ( fromFrame );
	CREATE_BAGKEY ( toFrame );
	CREATE_BAGKEY ( includeScopes );
	CREATE_BAGKEY ( number );
	CREATE_BAGKEY ( frameNumber );
	CREATE_BAGKEY ( relativePath );
	CREATE_BAGKEY ( fullPath );
	CREATE_BAGKEY ( message );
	CREATE_BAGKEY ( name );
	CREATE_BAGKEY ( beginOffset );
	CREATE_BAGKEY ( endOffset );
	CREATE_BAGKEY ( state );
	CREATE_BAGKEY ( user );
	CREATE_BAGKEY ( ha1 );
	CREATE_BAGKEY ( error );
};


#define		JSW_DEBUGGER_ID 'JSWD'

const XBOX::VError	VE_JSW_DEBUGGER_ALREADY_DEBUGGING = MAKE_VERROR(JSW_DEBUGGER_ID, 1001); /* ALREADY DEBUGGING */
const XBOX::VError	VE_JSW_DEBUGGER_UNKNOWN_REMOTE_EVENT = MAKE_VERROR(JSW_DEBUGGER_ID, 1002); /* UNKNOWN REMOTE EVENT */
const XBOX::VError	VE_JSW_DEBUGGER_CLIENT_LISTENER_NOT_FOUND = MAKE_VERROR(JSW_DEBUGGER_ID, 1003); /* CLIENT LISTENER NOT FOUND */
const XBOX::VError	VE_JSW_DEBUGGER_REMOTE_EVENT_IS_MESSAGE = MAKE_VERROR(JSW_DEBUGGER_ID, 1004); /* REMOTE EVENT IS MESSAGET */
const XBOX::VError	VE_JSW_DEBUGGER_UNKNOWN_VALUE_TYPE = MAKE_VERROR(JSW_DEBUGGER_ID, 1005); /* UNKNOWN VALUE TYPE */
const XBOX::VError	VE_JSW_DEBUGGER_NOT_CONNECTED = MAKE_VERROR(JSW_DEBUGGER_ID, 1006); /* DEBUGGER NOT CONNECTED */
const XBOX::VError	VE_JSW_DEBUGGER_AUTHENTICATION_REQUIRED = MAKE_VERROR(JSW_DEBUGGER_ID, 1007); /* DEBUGGER AUTHENTICATION REQUIRED */
const XBOX::VError	VE_JSW_DEBUGGER_AUTHENTICATION_FAILED = MAKE_VERROR(JSW_DEBUGGER_ID, 1008); /* DEBUGGER AUTHENTICATION FAILED */
const XBOX::VError	VE_JSW_DEBUGGER_NO_DEBUGGER_USERS = MAKE_VERROR(JSW_DEBUGGER_ID, 1009); /* NO DEBUGGER USERS */

#endif

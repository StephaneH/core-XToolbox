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
#include "VKernelPrecompiled.h"
#include "VValueBag.h"
#include "VString.h"
#include "ILogger.h"


BEGIN_TOOLBOX_NAMESPACE


namespace ILoggerBagKeys
{
	CREATE_BAGKEY_NO_DEFAULT( message, VString);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( level, VLong, EMessageLevel, EML_Information);
	CREATE_BAGKEY_NO_DEFAULT( source, VString);
	CREATE_BAGKEY_NO_DEFAULT( task_name, VString);
	CREATE_BAGKEY_NO_DEFAULT( file_name, VString);
	CREATE_BAGKEY_NO_DEFAULT( stack_crawl, VString);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( line_number, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( component_signature, VLong, OsType);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( elapsed_milliseconds, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_sent, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_received, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT( base_context_uuid, VUUID);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( error_code, VLong8, VError);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_asked, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( socket, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( is_blocking, VBoolean, bool);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( is_ssl, VBoolean, bool);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( is_select_io, VBoolean, bool);
	CREATE_BAGKEY_NO_DEFAULT( payload, const char*);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( ms_timeout, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( ms_spent, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( dump_offset, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT( dump_buffer, VString);
	CREATE_BAGKEY_NO_DEFAULT( local_addr, VString);
	CREATE_BAGKEY_NO_DEFAULT( peer_addr, VString);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( local_port, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( peer_port, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( task_id, VLong, sLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( exchange_id, VBoolean, bool);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( debug_context, VLong8, sLONG8);
	CREATE_BAGKEY_NO_DEFAULT( jobId, VString );
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( jobState, VLong, sLONG);
};



END_TOOLBOX_NAMESPACE

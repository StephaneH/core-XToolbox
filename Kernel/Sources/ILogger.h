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
#ifndef __ILogger__
#define __ILogger__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VValueBag.h"
#include "Kernel/Sources/IRefCountable.h"

BEGIN_TOOLBOX_NAMESPACE

class VString;
class VValueBag;
class VUUID;
class VLong;

namespace ILoggerBagKeys
{
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( message, VString);							// the message
	XTOOLBOX_API EXTERN_BAGKEY_WITH_DEFAULT_SCALAR( level, VLong, EMessageLevel);		// message level. Default is EML_Information
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( source, VString);							// identifies the source of the message. Use reverse dns notation.
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( task_name, VString);							// task name
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( file_name, VString);							// file path of source code where message occured
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( stack_crawl, VString);						// stack crawl. Lines separated with \n 
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( line_number, VLong, sLONG);			// line number in file of source code where message occured
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( component_signature, VLong, OsType);	// identifies the source of the message. Used only if 'source' is not available.
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( elapsed_milliseconds, VLong, sLONG);	// duration of associated action.
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_sent, VLong, sLONG);		// number of bytes sent
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_received, VLong, sLONG);	// number of bytes received
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( base_context_uuid, VUUID);					// uuid of db4d base context
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( error_code, VLong8, VError);			// error code from a VErrorBase (without component signature)
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( count_bytes_asked, VLong, sLONG);		// number of bytes we were asked to read or write
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( socket, VLong, sLONG);				// raw socket descriptor
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( is_blocking, VBoolean, bool);			// socket state
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( is_ssl, VBoolean, bool);				// socket state
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( is_select_io, VBoolean, bool);		// socket state
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( payload, const char*);						// input/ouptput buffers
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( ms_timeout, VLong, sLONG);			// ms timeout for io calls
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( ms_spent, VLong, sLONG);				// time spent in io calls
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( dump_offset, VLong, sLONG);			// Offset for Read/Write IO buffers
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( dump_buffer, VString);						// A printable slice of Read/Write IO buffers
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( local_addr, VString);						// Local v6 or v4 IP
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( peer_addr, VString);							// Peer v6 or v4 IP
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( local_port, VLong, sLONG);			// Local TCP port
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( peer_port, VLong, sLONG);				// Peer TCP port
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( task_id, VLong, sLONG);				// Toolbox task id
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( exchange_id, VBoolean, bool);			// exchange endpoint ID ? (debug only)
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( debug_context, VLong8, sLONG8);	// number of bytes received
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT( jobId, VString);				// job id
	XTOOLBOX_API EXTERN_BAGKEY_NO_DEFAULT_SCALAR( jobState, VLong, sLONG);				// job state
}

class XTOOLBOX_API ILogListener : public IRefCountable
{
public:
	virtual	void Put( std::vector< const XBOX::VValueBag* >& inValuesVector ) = 0;
};

/*
	@brief pure interface for logging purpose.
*/
class XTOOLBOX_API ILogger : public IRefCountable
{
public:
			/** @brief	Start message logging using the highest log number. Call Start() to continue the logging using last used log file. */
	virtual	void					Start() = 0;
	virtual	void					Stop() = 0;
			
			// fast way to know if the log is opened.
			// but remember that the log can be opened or closed from another thread while you are calling IsStarted().
	virtual	bool					IsStarted() const = 0;
	
			// tells if logger will actually log messages with specified level
	virtual	bool					ShouldLog( EMessageLevel inMessageLevel) const = 0;
	
			// the message bag might be retained by the logger.
			// Don't modify it once given to the logger.
	virtual	void					LogBag( const VValueBag *inMessage) = 0;

	virtual	void					LogMessage( EMessageLevel inLevel, const VString& inMessage, const VString& inSourceIdentifier) = 0;

			//Synchronously pushes every feeds listeners with every pending log messages
	virtual void					Flush() = 0;
	
			//turns on / turns off logging of trace tag, returns old value
	virtual bool					WithTrace	(bool inWithTag) = 0;
	virtual bool					WithDump	(bool inWithTag) = 0;
	virtual bool					WithDebug	(bool inWithTag) = 0;
	virtual bool					WithWarning	(bool inWithTag) = 0;
	virtual bool					WithError	(bool inWithTag) = 0;
	virtual bool					WithFatal	(bool inWithTag) = 0;
	virtual bool					WithAssert	(bool inWithTag) = 0;
	virtual bool					WithInfo	(bool inWithTag) = 0;

	virtual	bool					AddLogListener(ILogListener* /*inLogListener*/) { return false;}
	virtual	bool					RemoveLogListener(ILogListener* /*inLogListener*/) { return false;}

};


END_TOOLBOX_NAMESPACE


#endif

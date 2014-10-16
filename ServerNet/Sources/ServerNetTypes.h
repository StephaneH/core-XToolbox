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
#include "ServerNetArchTypes.h"
#include "ServerNetSystemTypes.h"


#ifndef __SNET_TYPES__
#define __SNET_TYPES__


BEGIN_TOOLBOX_NAMESPACE


class VEndPoint;
class VTCPEndPoint;
class VTCPClientSession;
class VTCPServerSession;

class VConnectionHandler;
class VConnectionHandlerQueue;
class VConnectionHandlerFactory;
class VTCPConnectionHandlerFactory;
class VWorkerPool;
class VTCPSelectIOPool;


typedef enum {IpAuto=0, IpForceV4, IpAnyIsV6, IpPreferV6, IpForceV6} IpPolicy;
typedef enum
{
	HTTP_UNKNOWN = -1,
	HTTP_GET = 0,
	HTTP_HEAD,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_TRACE,
	HTTP_OPTIONS
}
HTTP_Method;


const sLONG kAUTO_YIELD_TIMEOUT=50; //WaitFor will call Yield every 50 ms

#define WITH_SHARED_WORKERS 0


enum
{
	kServerNetListenerTaskKind = 'SNET',	// this task kind may be overriden by ServerNet users (db4d, http, sql)
	kServerNetSessionManagerTaskKind = 'SNSM'	// this task kind is reserved and must not be overriden
};


enum
{
	kWorkerPool_InUseTaskKind = 'WKPL',
	kWorkerPool_SpareTaskKind = 'WKPS',
};

// classic functor for quick deletion of containers objects (m.c)
template<class T> struct del_fun_t 
{
	del_fun_t& operator()(T* p)
	{ 
		delete p;
		return *this;
	}
};

template<class T> del_fun_t<T> del_fun() 
{ 
	return del_fun_t<T>(); 
};


END_TOOLBOX_NAMESPACE

class IRequestLogger;

#endif

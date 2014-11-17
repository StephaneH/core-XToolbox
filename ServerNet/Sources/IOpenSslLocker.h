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
#ifndef __SNET_I_OPEN_SSL_LOCKER__
#define __SNET_I_OPEN_SSL_LOCKER__


//This file is needed by sli/openssl ; IOpenSslLocker interface is used by sli to create synchronisation
//callbacks for OpenSSL. Also, keep in mind that Sli has no knowledge of xbox types (VError, sLONG, etc)


class IOpenSslLocker
{
	public :
	
	virtual void	Init(int inCount)=0;
	virtual void	Lock(int inIndex)=0;
	virtual void	Unlock(int inIndex)=0;
	virtual unsigned long   GetTaskID()=0;
};


#endif
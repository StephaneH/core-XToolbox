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
#include "ServerNetTypes.h"


#ifndef __SNET_I_KEY_CERT_CHAIN_SOURCE__
#define __SNET_I_KEY_CERT_CHAIN_SOURCE__


BEGIN_TOOLBOX_NAMESPACE


class VKeyCertChain;

class XTOOLBOX_API IKeyCertChainSource
{
public :
	virtual VKeyCertChain* RetainKeyCertChain()=0;
};

/*

 You will need those functions from SslFramework namespace to implement this interface :

 VKeyCertChain* RetainKeyCertificateChain(const VMemoryBuffer<>& inKeyBuffer, const VMemoryBuffer<>& inCertBuffer);
 VKeyCertChain* RetainKeyCertificateChain(VKeyCertChain* inKeyCertChain);
 void ReleaseKeyCertificateChain(VKeyCertChain** inKeyCertChain);

 VKeyCertChain is an opaque type from ServerNet. You can create it, retain it or release it
 because it may live outside of ServerNet, but you can not manipulate it.
 
*/

END_TOOLBOX_NAMESPACE

#endif

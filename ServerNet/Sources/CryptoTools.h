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


#ifndef __SNET_CRYPTO_TOOLS__
#define __SNET_CRYPTO_TOOLS__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VPrivateKey : public VObject
{
public :

	VPrivateKey();
	~VPrivateKey();

	VError Init(const VString& inPemKey);
	VError InitFromCertificate(const VString& inPemCert);

	//Inner type to hide implementation of private key
	class XKey;
	
	XKey* GetXKey() const;

private :

	VPrivateKey(const VPrivateKey&);	//No copy !

	XKey* fKey;
};


//VVerifier : A class to compute message digest

class XTOOLBOX_API VVerifier : public VObject
{
public :

	VVerifier();
	~VVerifier();

	VError Init(const VString& inAlgo);

	VError Update(const void* inData, sLONG inLen);

	//returns true if signature is OK, and false if it's not OR if there is an error (and vthrow something)
	bool Verify(const void* inSig, sLONG inSigLen, VPrivateKey& inKey);


private :

	VVerifier(const VVerifier&);	//No copy !

	//Inner type to hide implementation of message digest context
	class XDigestContext;

	XDigestContext* fDigestContext;
};


END_TOOLBOX_NAMESPACE


#endif

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

#include "CryptoTools.h"

#include "SslStub.h"

BEGIN_TOOLBOX_NAMESPACE

//jmo - FIXME : Wooops Duplicated code !
//jmo - TODO : Rewrite it with a (registered) localizer !

static VError vThrowThreadErrorStackRec()
{
	unsigned long err=SSLSTUB::ERR_get_error();
	
	if(err==0)
		return VE_OK;
	
	//We need to recurse to throw the older errors first ! 
	vThrowThreadErrorStackRec();
	
	char buf[200];
	memset(buf, 0, sizeof(buf));

	SSLSTUB::ERR_error_string_n(err, buf, sizeof(buf));
	
	DebugMsg ("[%d] OpenSSL stacked error : %s\n", VTask::GetCurrentID(), buf);
	
	return vThrowError(VE_SSL_STACKED_ERROR, buf);
}


static VError vThrowThreadErrorStack(VError inImplErr)
{
	if(inImplErr==VE_OK)
		return VE_OK;

	vThrowThreadErrorStackRec();
	
	SSLSTUB::ERR_clear_error();
	
	return vThrowError(inImplErr);
}


enum EHashAlgorithm {kMD5, kSHA1};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VPrivateKey::XKey (an implementation wrapper to hide OpenSSL EVP_PKEY)
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VPrivateKey::XKey
{
public :

	XKey():fEvpKey(NULL) { }

	~XKey() { if(fEvpKey!=NULL) { SSLSTUB::EVP_PKEY_free(fEvpKey); } }

	VError Init(const VString& inPemKey);
	VError InitFromCertificate(const VString& inPemCert);


	EVP_PKEY* GetEvpKey() const { return fEvpKey; }


private :

	XKey(const XKey&);	//No copy !

	EVP_PKEY* fEvpKey;
};


VError VPrivateKey::XKey::Init(const VString& inPemKey)
{
	VError verr=VE_OK;

	XBOX::VStringConvertBuffer convBuf(inPemKey, XBOX::VTC_UTF_8);

	const unsigned char* ptr=reinterpret_cast<const unsigned char*>(convBuf.GetCPointer());
	int len=convBuf.GetSize();

	BIO* bioBuf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());

	if(bioBuf==NULL)
		verr=VE_MEMORY_FULL;

	int res=0;

	if(verr==VE_OK)
		res=SSLSTUB::BIO_write(bioBuf, ptr, len);

	if(res!=len)
		verr=VE_SSL_FAIL_TO_WRITE_BIO;

	fEvpKey=SSLSTUB::PEM_read_bio_PrivateKey(bioBuf, NULL, NULL, NULL);

	if(fEvpKey==NULL)
		verr=VE_SSL_FAIL_TO_READ_PEM;

	if(bioBuf!=NULL)
		SSLSTUB::BIO_free(bioBuf);

	xbox_assert(fEvpKey!=NULL);

	return vThrowError(verr);
}


// jmo - FIXME - Oh /purée/ (OpenSSL) c'est /affreux/ !
VError VPrivateKey::XKey::InitFromCertificate(const VString& inPemCert)
{
	VError verr=VE_OK;

	XBOX::VStringConvertBuffer convBuf(inPemCert, XBOX::VTC_UTF_8);

	const unsigned char* ptr=reinterpret_cast<const unsigned char*>(convBuf.GetCPointer());
	int len=convBuf.GetSize();

	BIO* bioBuf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());

	if(bioBuf==NULL)
		verr=VE_MEMORY_FULL;

	int res=0;

	if(verr==VE_OK)
		res=SSLSTUB::BIO_write(bioBuf, ptr, len);

	if(res!=len)
		verr=VE_SSL_FAIL_TO_WRITE_BIO;

	X509* cert=NULL;

	if(verr==VE_OK)
		cert=SSLSTUB::PEM_read_bio_X509(bioBuf, NULL, NULL, NULL);

	if(cert==NULL)
		verr=VE_SSL_FAIL_TO_READ_PEM;

	if(verr==VE_OK)
		fEvpKey=SSLSTUB::X509_get_pubkey(cert);

	if(fEvpKey==NULL)
		//jmo - FIXME : FAIL_TO_READ_X09 ?
		verr=VE_SSL_FAIL_TO_READ_PEM;

	if(bioBuf!=NULL)
		SSLSTUB::BIO_free(bioBuf);

	xbox_assert(fEvpKey!=NULL);

	return vThrowError(verr);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VPrivateKey (a specialized wrapper for OpenSSL EVP_PKEY, a high level type to handle keys (priv/pub, rsa/dsa/etc)
//
// http://www.openssl.org/docs/crypto/pem.html
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

VPrivateKey::VPrivateKey()
{
	fKey=new VPrivateKey::XKey;
}


VPrivateKey::~VPrivateKey()
{
	delete fKey;
}


VError VPrivateKey::Init(const VString& inPemKey)
{
	VError verr=VE_OK;

	if(fKey==NULL)
		verr=VE_SSL_FAIL_TO_INIT_KEY;
	else
		verr=fKey->Init(inPemKey);

	if(verr!=VE_OK)
		return vThrowError(VE_SSL_FAIL_TO_INIT_KEY);

	return VE_OK;
}

VError VPrivateKey::InitFromCertificate(const VString& inPemCert)
{
	VError verr=VE_OK;

	if(fKey==NULL)
		verr=VE_SSL_FAIL_TO_INIT_KEY;
	else
		verr=fKey->InitFromCertificate(inPemCert);

	if(verr!=VE_OK)
		return vThrowError(VE_SSL_FAIL_TO_INIT_KEY);

	return VE_OK;
}


VPrivateKey::XKey* VPrivateKey::GetXKey() const
{
	return fKey;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VVerifier::XDigestContext (an implementation wrapper to hide OpenSSL EVP_MD_CTX)
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VVerifier::XDigestContext
{
public :

	XDigestContext():fCtx(NULL) { }

	~XDigestContext() { if(fCtx!=NULL) { SSLSTUB::EVP_MD_CTX_cleanup(fCtx); SSLSTUB::EVP_MD_CTX_destroy(fCtx); } }

	VError Init(EHashAlgorithm inAlgo);

	EVP_MD_CTX* GetEvpMdCtx() { return fCtx; }


private :

	XDigestContext(const XDigestContext&);	//No copy !

	EVP_MD_CTX* fCtx;
};


VError VVerifier::XDigestContext::Init(EHashAlgorithm inAlgo)
{
	fCtx=SSLSTUB::EVP_MD_CTX_create();
	xbox_assert(fCtx!=NULL);

	if(fCtx==NULL)
		return vThrowError(VE_MEMORY_FULL);

	VError verr=VE_OK;

	switch(inAlgo)
	{
		case kMD5:
			SSLSTUB::EVP_DigestInit_ex(fCtx, SSLSTUB::EVP_md5(), NULL);
			break;

		case kSHA1:
			SSLSTUB::EVP_DigestInit_ex(fCtx, SSLSTUB::EVP_sha1(), NULL);
			break;

		default:
			verr=VE_UNIMPLEMENTED;
			xbox_assert(false);
	}

	return verr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VVerifier (a wrapper for OpenSSL EVP_VerifyXxx routines, a high level interface for digital signatures)
//
// http://www.openssl.org/docs/crypto/EVP_VerifyInit.html
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	


VVerifier::VVerifier()
{
	fDigestContext=new XDigestContext;
}


VVerifier::~VVerifier()
{
	delete fDigestContext;
	fDigestContext=NULL;
}


VError VVerifier::Init(const VString& inAlgo)
{
	VError verr=VE_OK;

	if(fDigestContext==NULL)
		verr=VE_MEMORY_FULL;

	EHashAlgorithm algo=kSHA1;

	if(verr==VE_OK)
	{
		if(inAlgo.EqualToString(CVSTR("sha1"), false /*case insensitive*/))
			algo=kSHA1;
		else if(inAlgo.EqualToString(CVSTR("md5"), false /*case insensitive*/))
			algo=kMD5;
		else if(!inAlgo.IsEmpty())
			verr=VE_UNIMPLEMENTED;
	}

	if(verr==VE_OK)
	{
		verr=fDigestContext->Init(algo);
		xbox_assert(verr==VE_OK);
	}

	if(verr!=VE_OK)
	{
		delete fDigestContext;
		fDigestContext=NULL;

		verr=VE_SSL_FAIL_TO_INIT_VERIFIER;
	}

	return vThrowError(verr);
}


VError VVerifier::Update(const void* inData, sLONG inLen)
{
	xbox_assert(fDigestContext!=NULL);
	
	if(fDigestContext==NULL)
		return VE_SSL_FAIL_TO_INIT_VERIFIER;	//Already thrown

	unsigned int ulen=(unsigned int)inLen;

	EVP_MD_CTX* ctx=fDigestContext->GetEvpMdCtx();

	if((sLONG)ulen!=inLen || ctx==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	int res=SSLSTUB::EVP_VerifyUpdate(ctx, inData, ulen);

	if(res!=1)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_UPDATE_DIGEST);

	return VE_OK;
}


bool VVerifier::Verify(const void* inSig, sLONG inSigLen, VPrivateKey& inKey)
{
	xbox_assert(fDigestContext!=NULL);

	if(fDigestContext==NULL)
		return false;

	const unsigned char* sig=reinterpret_cast<const unsigned char*>(inSig);

	unsigned int ulen=(unsigned int)inSigLen;

	VPrivateKey::XKey* xkey=inKey.GetXKey();
	EVP_PKEY* pkey=(xkey!=NULL) ? xkey->GetEvpKey() : NULL;	

	EVP_MD_CTX* ctx=fDigestContext->GetEvpMdCtx();

	if(sig==NULL || (VIndex)ulen!=inSigLen || pkey==NULL)
	{
		vThrowError(VE_INVALID_PARAMETER);
		return false;
	}

	int res=SSLSTUB::EVP_VerifyFinal(ctx, sig, ulen, pkey);

	if(res==-1)
	{
		vThrowThreadErrorStack(VE_SSL_FAIL_TO_VERIFY_DIGEST);
		return false;
	}

	if(res==0)
		return false;

	return true;
}


END_TOOLBOX_NAMESPACE

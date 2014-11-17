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

#include "VSockListener.h"

#include "Tools.h"
#include "VSslDelegate.h"
#include "VNetAddr.h"

BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


class XTOOLBOX_API XSBind
{
public:

	XSBind(const VNetAddress& inAddr, IRequestLogger* inRequestLogger=NULL, Socket inBoundSock=kBAD_SOCKET, bool inReuseAddress=true) :
	fAddr(inAddr), fRequestLogger(inRequestLogger), fIsSSL(false), fBoundSock(inBoundSock), fSock(NULL), fReuseAddress (inReuseAddress) { }
	
	virtual ~XSBind()						{ if(fSock!=NULL) fSock->Close(), delete fSock; }

	void SetAddress(const VNetAddress& inAddr)	{ fAddr=inAddr; }
	const VString GetIP()					{ return fAddr.GetIP(); }
	PortNumber GetPort()					{ return fAddr.GetPort(); }
	
	void SetSSL()							{ fIsSSL=true; }
	
	bool IsSSL()							{ return fIsSSL; }
	
	void SetSock(XTCPSock* iSock)			{ fSock=iSock; }
	
	XTCPSock* GetSock()						{ return fSock; }
	
	VError Publish()
	{
		StTmpErrorContext errCtx;

		XTCPSock* sock=XTCPSock::NewServerListeningSock(fAddr, fBoundSock, fReuseAddress);

		SetSock(sock);
		
		if(sock!=NULL)
		{
			errCtx.Flush();
			return VE_OK;
		}
		
		return vThrowError(VE_SRVR_FAILED_TO_CREATE_LISTENING_SOCKET);
	}
	
	
private:
	
	VNetAddress		fAddr;
	IRequestLogger* fRequestLogger;	//jmo - Legacy, unused, stuff
	bool			fIsSSL;
	Socket			fBoundSock;
	XTCPSock*		fSock;
	bool			fReuseAddress;
};


VSockListener::VSockListener(IRequestLogger* inRequestLogger) :
fRequestLogger(inRequestLogger), fListenStarted(false), fKeyCertChain(NULL)
{}


VSockListener::~VSockListener()
{
	StopListeningAndClearPorts();
	
	SslFramework::ReleaseKeyCertChain(&fKeyCertChain);
}


bool VSockListener::AddListeningPort(VString inAddr, PortNumber inPort, bool iSsl, Socket inBoundSock, bool inReuseAddress)
{
	return AddListeningPort(VNetAddress(inAddr, inPort), iSsl, inBoundSock, inReuseAddress);
}


bool VSockListener::AddListeningPort(const VNetAddress& inAddr, bool iSsl, Socket inBoundSock, bool inReuseAddress)
{	
	assert(!fListenStarted);
	
	std::vector<XSBind*> *l_vector = iSsl ? &fSslListens : &fPlainListens;
	
	try
	{
		XSBind*	newBind = new XSBind (inAddr, fRequestLogger, inBoundSock, inReuseAddress);
		
		if(iSsl)
			newBind-> SetSSL();
		
		l_vector-> push_back ( newBind );
	}
	catch (...)
	{
		return false;
	}
	return true;
}


void VSockListener::SetCertificatePaths (const VFilePath& inCertPath, const VFilePath& inKeyPath)
{
	VError verr=VE_OK;
	bool done=false;
	
	//Try to handle intermediate certificates...	

	if(inCertPath.IsFile() && inKeyPath.IsFile())
	{	
		VFilePath certFolderPath;
		inCertPath.GetFolder(certFolderPath);
	
		VFilePath keyFolderPath;
		inKeyPath.GetFolder(keyFolderPath);
	
		if(certFolderPath.IsFolder() && keyFolderPath.IsFolder() && certFolderPath==keyFolderPath)
		{
			VString cert;
			inCertPath.GetName(cert);
			
			VString key;
			inKeyPath.GetName(key);
			
			verr=SetCertificateFolder(certFolderPath, key, cert);
			xbox_assert(verr==VE_OK);
			
			done=true;
		}
	}
	
	//Do load key and cert the old way...
	
	if(!done)
	{
		VFile certFile(inCertPath);
		
		VMemoryBuffer<> certBuffer;
	
		verr=certFile.GetContent(certBuffer);
	
		if(verr!=VE_OK)
			vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
		VFile keyFile(inKeyPath);
	
		VMemoryBuffer<> keyBuffer;
	
		verr=keyFile.GetContent(keyBuffer); 
	
		if(verr!=VE_OK)
			vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
		verr=SetKeyAndCertificate(keyBuffer, certBuffer);
		xbox_assert(verr==VE_OK);
	}		
}

//inKey should be "key.pem" and inCert should be "cert.pem"
VError VSockListener::SetCertificateFolder(const VFilePath& inCertFolderPath, const VString& inKey, const VString& inCert)
{
	VError verr=VE_OK;

	
	if(!inCertFolderPath.IsFolder())
		verr=vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
	
	VMemoryBuffer<> keyBuffer;
	
	if(verr==VE_OK)
	{
		VFilePath keyPath(inCertFolderPath);
		keyPath.SetFileName(inKey);
		
		VFile keyFile(keyPath);
		
		if(keyFile.Exists())
			verr=keyFile.GetContent(keyBuffer); 
		else
			verr=vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);

	}
	
	
	VMemoryBuffer<> certBuffer;
	
	if(verr==VE_OK)
	{
		VFilePath certPath(inCertFolderPath);
		certPath.SetFileName(inCert);
		
		VFile certFile(certPath);
		
		if(certFile.Exists())
			verr=certFile.GetContent(certBuffer); 
		else
			verr=vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
		
	}
	
	
	if(verr==VE_OK)
		verr=SetKeyAndCertificate(keyBuffer, certBuffer);
		

	VFolder certFolder(inCertFolderPath);
	
	sLONG succeedCount=0;
	sLONG failCount=0;

	for(VFileIterator fit(&certFolder) ; fit.IsValid() ; ++fit)
	{
		if(fit->MatchExtension(VString("pem")))
		{
			VString name;
			fit->GetName(name);
			
			if(name!="key.pem" && name !="cert.pem")
			{
				VMemoryBuffer<> intermediateCertBuffer;

				VError tmpErr=fit->GetContent(intermediateCertBuffer);
				
				if(tmpErr==VE_OK)
					tmpErr=PushIntermediateCertificate(intermediateCertBuffer);
				
				if(verr==VE_OK)
					succeedCount++;
				else
				{
					failCount++;
					
					DebugMsg ("[%d] VSockListener::SetCertificateFolder : Fail to load intermediate certificate %S\n",
							  VTask::GetCurrentID(), &name);
				}
			}
		}
	}
	
	
	if(verr==VE_OK && succeedCount==0 && failCount>0)
		verr=VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE;
	
	
	return verr;
}


VError VSockListener::SetKeyAndCertificate(const VMemoryBuffer<>& inKey, const VMemoryBuffer<>& inCertificate)
{
	SslFramework::ReleaseKeyCertChain(&fKeyCertChain);
	
	fKeyCertChain=SslFramework::RetainKeyCertChain(inKey, inCertificate);
	
	if(fKeyCertChain==NULL)
		return vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
	return VE_OK;
}


VError VSockListener::SetKeyAndCertificate (const XBOX::VString &inKey, const XBOX::VString &inCertificate)
{
	char			*keyData, *certificateData;
	XBOX::VError	error;

	keyData = certificateData = NULL;		
	if ((keyData = new char[inKey.GetLength() + 1]) != NULL 
	&& (certificateData = new char[inCertificate.GetLength() + 1]) != NULL) {

		VMemoryBuffer<> key, certificate;

		inKey.ToCString(keyData, inKey.GetLength() + 1);
		inCertificate.ToCString(certificateData, inCertificate.GetLength() + 1);

		key.SetDataPtr(keyData, inKey.GetLength(), inKey.GetLength() + 1);
		certificate.SetDataPtr(certificateData, inCertificate.GetLength(), inCertificate.GetLength() + 1);

		error = this->SetKeyAndCertificate(key, certificate);

		key.ForgetData();
		certificate.ForgetData();

	} else 

		error = VE_MEMORY_FULL;

	if (keyData != NULL)

		delete[] keyData;

	if (certificateData != NULL)

		delete[] certificateData;

	return error;
}


VError VSockListener::PushIntermediateCertificate(const VMemoryBuffer<>& inCertificate)
{
	return SslFramework::PushIntermediateCertificate(fKeyCertChain, inCertificate);
}


VError VSockListener::PushIntermediateCertificate(const XBOX::VString &inCertificate)
{
	char			*certificateData;
	XBOX::VError	error;
	
	certificateData = NULL;		
	if ((certificateData = new char[inCertificate.GetLength() + 1]) != NULL) {
		
		VMemoryBuffer<> certificate;
		
		inCertificate.ToCString(certificateData, inCertificate.GetLength() + 1);
		
		certificate.SetDataPtr(certificateData, inCertificate.GetLength(), inCertificate.GetLength() + 1);
		
		error = this->PushIntermediateCertificate(certificate);
		
		certificate.ForgetData();
		
	} else 
		
		error = VE_MEMORY_FULL;
	
	if (certificateData != NULL)
		
		delete[] certificateData;
	
	return error;
	
}


bool VSockListener::StartListening()
{
	bool l_res = false;
	
	if (!fListenStarted)
	{
		l_res = true;
		std::vector<XSBind*>::iterator		iterBind = fPlainListens. begin ( );
		while ( iterBind != fPlainListens. end ( ) )
		{
			if ( !( l_res = ( ( *iterBind )-> Publish ( ) == VE_OK ) ) )
				break;
			
			fAcceptIterator.AddServiceSocket((*iterBind)->GetSock());
			
			iterBind++;
		}
		if ( l_res )
		{
			iterBind = fSslListens. begin ( );
			while ( iterBind != fSslListens. end ( ) )
			{
				VError verr=(*iterBind)->Publish();
				
				if(verr!=VE_OK)
				{
					l_res=false;
					break;
				}
				
				fAcceptIterator.AddServiceSocket((*iterBind)->GetSock());
				
				iterBind++;
			}
		}
		
		fListenStarted = true;	/* To indicate that there may be some cleaning up to do. */
	}
	
	return l_res;
}


void VSockListener::StopListeningAndClearPorts()
{
	std::for_each(fPlainListens.begin(), fPlainListens.end(), del_fun<XSBind>()); 
	fPlainListens.clear();
	
	std::for_each(fSslListens.begin(), fSslListens.end(), del_fun<XSBind>()); 
	fSslListens.clear();
	
	fAcceptIterator.ClearServiceSockets();
	
	fListenStarted = false;
}


bool VSockListener::SetBlocking (bool isBlocking)
{
	for (uLONG i = 0; i < fPlainListens.size(); ++i)
	{
		XSBind *bind = dynamic_cast<XSBind*> (fPlainListens[i]);
		
		if (bind)
		{
			if (bind->GetSock()->SetBlocking(isBlocking) != VE_OK)
				return false;
		}
	}
	
	return true;
}


XTCPSock* VSockListener::GetNewConnectedSocket(sLONG inMsTimeout)
{
	StTmpErrorContext errCtx;
	
	XTCPSock* sock=NULL;
	
	VError verr=fAcceptIterator.GetNewConnectedSocket(&sock, inMsTimeout);
	
	if(sock!=NULL && verr==VE_OK)
	{
		for(XSBindCIt cit=fSslListens.begin() ; cit!=fSslListens.end() ; ++cit)
		{
			if((*cit)->GetPort()==sock->GetServicePort())
			{
				verr=sock->PromoteToSSL(fKeyCertChain);
				break;
			}
		}
	}
	
	if(verr==VE_OK || verr==VE_SOCK_TIMED_OUT)
	{
		errCtx.Flush();
		
		return sock; 
	}
	
	vThrowError(VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET);
	
	delete sock;
	
	return NULL;
}


void VSockListener::ReleaseConnection(XTCPSock *iConnection)
{
	delete iConnection;
}


END_TOOLBOX_NAMESPACE

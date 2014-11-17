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
#ifndef __IStreamRequest__
#define __IStreamRequest__

#include "Kernel/VKernel.h"

BEGIN_TOOLBOX_NAMESPACE

/**

	@brief IStreamRequest proposes an interface for sending a request.
	
	example for sending a request:

	IStreamRequest *request = CreateRequest();
	if (request != NULL)
	{
		request->GetOutputStream()->Put<sLONG>( 42);
		request->GetOutputStream()->Put( CVSTR( "coucou"));
		
		VError err = request->Send();
		if (err == VE_OK)
		{
			// extract status code
			RequestStatusCode status = 	GetStatusCodeFromError( err);

			// extract result
			VString result;
			request->GetOutputStream()->Get( &result);
		}
		
		request->Release();
	}
	
**/

typedef sWORD	RequestStatusCode;

class IStreamRequest : public IRefCountable
{
public:

	/**
		@brief GetOutputStream returns the output stream to send request parameters.
		It is valid as long as Send() has not been called.
	**/
	virtual	VStream*			GetOutputStream() = 0;
	/**
		@brief GetInputStream returns the input stream to receive request results.
		It is valid if Send() has been called and returned VE_OK.
	**/
	virtual	VStream*			GetInputStream() = 0;
	/**
		@brief Send flushes & closes the output streams.
		It then gets the first result packet which contain the status code provided in IStreamRequestReply::InitReply on the server side.
		If the status code is 0, Send() opens the input stream that can be used to read the request results.
	**/
	virtual	VError				Send() = 0;
	
	/**
		@brief GetStatusCodeFromError extract the status code sent by IStreamRequestReply::InitReply.
		If the VError is VE_OK, GetStatusCodeFromError returns 0.
		If the VError has not been built with a status code, GetStatusCodeFromError returns the default value specified.
	**/
	static RequestStatusCode	GetStatusCodeFromError( VError inError, RequestStatusCode inDefault = -1)
		{
			return (inError == VE_OK) ? 0 : ((COMPONENT_FROM_VERROR( inError) == '4DRT') ? ERRCODE_FROM_VERROR( inError) : inDefault);
		}
};


/**
	@brief IStreamRequestReply proposes an interface for replying to a request.
	
	example to receive the request in the example for IStreamRequest:
	
	void ExecuteRequest( IStreamRequestReply *inRequest)
	{
		sLONG param1;
		VString param2;
		inRequest->GetInputStream()->Get( &param1);
		inRequest->GetInputStream()->Get( &param2);
		
		// check params and initialize the return stream with a status code first.
		if (param1 != 42)
			inRequest->InitReply( -1);	// error
		else
		{
			inRequest->InitReply( 0);	// no error
			
			VString result;
			inRequest->GetOutputStream()->Put( result);
		}
	}

**/
class IStreamRequestReply
{
public:
	/**
		@brief GetInputStream returns the input stream to receive request parameters.
		It is valid even after InitReply has been called.
	**/
	virtual	VStream*	GetInputStream() = 0;
	/**
		@brief GetOutputStream returns the output stream to send request results.
		It is valid only after InitReply has been called.
	**/
	virtual	VStream*	GetOutputStream() = 0;
	/**
		@brief InitReply opens the output stream and send the status code.
		The output stream is then available to send results.
	**/
	virtual	void		InitReply( RequestStatusCode inStatus) = 0;
};

END_TOOLBOX_NAMESPACE


#endif
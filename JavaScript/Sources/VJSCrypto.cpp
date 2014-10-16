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
#include "VJavaScriptPrecompiled.h"

#include "VJSBuffer.h"
#include "VJSCrypto.h"
#include "VJSGlobalClass.h"

#include "ServerNet/VServerNet.h"

USING_TOOLBOX_NAMESPACE




template<class ALGORITHM>
class VJSHashClass : public VJSClass<VJSHashClass<ALGORITHM>, ALGORITHM>
{
public:
	typedef VJSClass<VJSHashClass, ALGORITHM>	inherited;

	#if COMPIL_CLANG
#if USE_V8_ENGINE
	static void js_callStaticFunction_update( const v8::FunctionCallbackInfo<v8::Value>& args )
	{
	}
#else
	static JS4D::ValueRef js_callStaticFunction_update( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		_update( parms, (ALGORITHM*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
#if USE_V8_ENGINE
	static void js_callStaticFunction_digest( const v8::FunctionCallbackInfo<v8::Value>& args )
	{
	}
#else
	static JS4D::ValueRef js_callStaticFunction_digest( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		_digest( parms, (ALGORITHM*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
	#endif

	static	void	GetDefinition( typename inherited::ClassDefinition &outDefinition)
	{
		static typename inherited::StaticFunction functions[] =
		{
		#if COMPIL_CLANG
			// clang doesn't accept the usual syntax. Don't know why.
			{ "update",					js_callStaticFunction_update,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
			{ "digest",					js_callStaticFunction_digest,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		#else
			{ "update",					inherited::js_callStaticFunction<_update>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
			{ "digest",					inherited::js_callStaticFunction<_digest>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		#endif
			{ 0,						0,											0									},
		};

		outDefinition.className	= "hash";
		outDefinition.finalize = _Finalize;
		outDefinition.staticFunctions = functions;	
	}
	
private:

	static void __cdecl _Finalize( JS4D::ObjectRef inObject)
	{
#if USE_V8_ENGINE
#else
		delete (ALGORITHM*) JS4D::GetObjectPrivate( inObject);
#endif
	}

	static	void	_update( VJSParms_callStaticFunction& ioParms, ALGORITHM *inAlgorithm)
	{
		if (ioParms.IsStringParam( 1))
		{
			VString dataString;
			ioParms.GetStringParam( 1, dataString);
			
			VString encodingName;
			if (ioParms.CountParams() > 1)
				ioParms.GetStringParam( 2, encodingName);
			else
				encodingName = "utf8";

			CharSet charSet = VJSBufferObject::GetEncodingType( encodingName);
			if (charSet != VTC_UNKNOWN)
			{
				VStringConvertBuffer buffer( dataString, charSet);
				inAlgorithm->Update( buffer.GetCPointer(), buffer.GetSize());
			}
			else
			{
				vThrowError( VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingName);
			}
		}
		else if (ioParms.IsObjectParam( 1))
		{
			VJSObject data( ioParms.GetContext());
			VJSBufferObject *buffer = ioParms.GetParamObjectPrivateData<VJSBufferClass>( 1);
			if (buffer != NULL)
			{
				inAlgorithm->Update( buffer->GetDataPtr(), buffer->GetDataSize());
			}
			else
			{
				vThrowError( VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "1");
			}
		}
		else
		{
			vThrowError(XBOX::VE_JVSC_EXPECTING_PARAMETER, CVSTR("crypto.hash.update()"));
		}
	}


	static	void	_digest( VJSParms_callStaticFunction& ioParms, ALGORITHM *inAlgorithm)
	{
		typename ALGORITHM::digest_type digest;
		inAlgorithm->GetChecksum( digest);
		
		VString encodingName;
		if (!ioParms.GetStringParam( 1, encodingName))
		{
			// return a buffer object
			void *copy = malloc( sizeof( digest));
			if (copy != NULL)
			{
				::memcpy( copy, &digest, sizeof( digest));
				VJSBufferObject *bufferObject = new VJSBufferObject( sizeof( digest), copy);

				VJSObject bufferJSObject( ioParms.GetContext());
				bufferJSObject.MakeObject( VJSBufferClass::Class(), bufferObject);
				ioParms.ReturnValue( bufferJSObject);
				
				ReleaseRefCountable( &bufferObject);
			}
			else
				vThrowError( VE_MEMORY_FULL);
		}
		else if (encodingName.EqualToUSASCIICString( "base64"))
		{
			VString result;
			ALGORITHM::EncodeChecksumBase64( digest, result);
			ioParms.ReturnString( result);
		}
		else if (encodingName.EqualToUSASCIICString( "hex"))
		{
			VString result;
			ALGORITHM::EncodeChecksumHexa( digest, result);
			ioParms.ReturnString( result);
		}
		else
		{
			vThrowError( VE_JVSC_BUFFER_UNSUPPORTED_ENCODING, encodingName);
		}
	}

};



//===================================================================================


class VJSVerifyClass : public VJSClass<VJSVerifyClass, XBOX::VVerifier>
{
public:
	typedef VJSClass<VJSVerifyClass, XBOX::VVerifier>	inherited;

#if COMPIL_CLANG
#if USE_V8_ENGINE
	static void js_callStaticFunction_update( const v8::FunctionCallbackInfo<v8::Value>& args )
	{
	}
#else
	static JS4D::ValueRef js_callStaticFunction_update( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		_update( parms, (XBOX::VVerifier*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
#if USE_V8_ENGINE
	static void js_callStaticFunction_verify( const v8::FunctionCallbackInfo<v8::Value>& args )
	{
	}
#else
	static JS4D::ValueRef js_callStaticFunction_verify( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		_verify( parms, (XBOX::VVerifier*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
#endif

	static	void	GetDefinition(inherited::ClassDefinition &outDefinition)
	{
		static inherited::StaticFunction functions[] =
		{
			{ "update",					inherited::js_callStaticFunction<_update>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
			{ "verify",					inherited::js_callStaticFunction<_verify>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
			{ 0,						0,											0									},
		};

		outDefinition.className	= "verify";
		outDefinition.finalize = _Finalize;
		outDefinition.staticFunctions = functions;
	}

private:

	static void __cdecl _Finalize( JS4D::ObjectRef inObject)
	{
#if USE_V8_ENGINE
#else
		delete (XBOX::VVerifier*) JS4D::GetObjectPrivate( inObject);
#endif
	}

	static	void	_update( VJSParms_callStaticFunction& ioParms, XBOX::VVerifier *inVerifier)
	{
		if (ioParms.IsStringParam( 1))
		{
			//jmo - Handle VJSHashClass::_update and VJSVerifyClass::_update should handle string in the same way
			VString dataString;
			ioParms.GetStringParam( 1, dataString);

			VString encodingName;
			ioParms.GetStringParam( 2, encodingName);

			CharSet charSet = VJSBufferObject::GetEncodingType( encodingName);
			
			if (charSet != VTC_UNKNOWN)
			{
				VStringConvertBuffer buffer( dataString, charSet);
				inVerifier->Update( buffer.GetCPointer(), buffer.GetSize());
			}
		}
		else if (ioParms.IsObjectParam( 1))
		{
			VJSObject data( ioParms.GetContext());
			VJSBufferObject *buffer = ioParms.GetParamObjectPrivateData<VJSBufferClass>( 1);

			if (buffer != NULL)
			{
				inVerifier->Update( buffer->GetDataPtr(), buffer->GetDataSize());
			}
		}
	}


	static	void	_verify( VJSParms_callStaticFunction& ioParms, XBOX::VVerifier *inVerifier)
	{
		bool verified=false;

		VString pemCertificate;

		if (ioParms.GetStringParam(1, pemCertificate))
		{
			VPrivateKey privateKey;

			VError verr=privateKey.InitFromCertificate(pemCertificate);

			if(verr==VE_OK && ioParms.IsObjectParam(2))
			{
				VJSObject data( ioParms.GetContext());
				VJSBufferObject *buffer = ioParms.GetParamObjectPrivateData<VJSBufferClass>( 2);

				if (buffer != NULL)
				{
					verified=inVerifier->Verify(buffer->GetDataPtr(), buffer->GetDataSize(), privateKey);
				}
			}

			ioParms.ReturnBool(verified);
		}
	}
};

	//===================================================================================


const XBOX::VString	VJSCryptoClass::kModuleName	= CVSTR("crypto");

void VJSCryptoClass::RegisterModule ()
{
	VJSGlobalClass::RegisterModuleConstructor(kModuleName, _ModuleConstructor);
}

XBOX::VJSObject VJSCryptoClass::_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName)
{
	xbox_assert(inModuleName.EqualToString(kModuleName, true));

	VJSCryptoClass::Class();
	return VJSCryptoClass::CreateInstance(inContext, NULL);
}

void VJSCryptoClass::GetDefinition( ClassDefinition &outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "createHash",				js_callStaticFunction<_createHash>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
		{ "getHashes",				js_callStaticFunction<_getHashes>,				JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
#if WITH_CRYPTO_VERIFIER_API
		{ "createVerify",			js_callStaticFunction<_createVerify>,			JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly	},
#endif
		{ 0,						0,												0									},
	};

    outDefinition.className	= "crypto";
	outDefinition.initialize = js_initialize<_Initialize>;
	outDefinition.staticFunctions = functions;	
}


void VJSCryptoClass::_Initialize( const VJSParms_initialize& inParms, void *)
{
}


void VJSCryptoClass::_createHash( VJSParms_callStaticFunction& ioParms, void *)
{
	VJSObject hashObject( ioParms.GetContext());
	
	VString algorithm;
	ioParms.GetStringParam( 1, algorithm);
	
	if (algorithm.EqualToUSASCIICString( "sha1"))
	{
		hashObject.MakeObject( VJSHashClass<VChecksumSHA1>::Class(), new VChecksumSHA1());
	}
	else if (algorithm.EqualToUSASCIICString( "md5"))
	{
		hashObject.MakeObject( VJSHashClass<VChecksumMD5>::Class(), new VChecksumMD5());
	}

	ioParms.ReturnValue( hashObject);
}


void VJSCryptoClass::_getHashes( VJSParms_callStaticFunction& ioParms, void *)
{
	VJSArray hashes( ioParms.GetContext());

	hashes.PushString( CVSTR( "md5"));
	hashes.PushString( CVSTR( "sha1"));
	
	ioParms.ReturnValue( hashes);
}

void VJSCryptoClass::_createVerify( VJSParms_callStaticFunction& ioParms, void *)
{
	VError verr=VE_OK;

	VJSObject verifyObject( ioParms.GetContext());

	VString algo;
	ioParms.GetStringParam( 1, algo);

	VVerifier* verifier=new VVerifier();

	if(verifier==NULL)
		verr=VE_MEMORY_FULL;

	if(verr==VE_OK)
		verr=verifier->Init(algo);

	if(verr==VE_OK)
		verifyObject.MakeObject( VJSVerifyClass::Class(), verifier);
	else
	{
		delete verifier;
		verifier=NULL;
	}

	ioParms.ReturnValue( verifyObject);
}

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
#ifndef __VJS_PARAMS_H__
#define __VJS_PARAMS_H__

#include "JS4D.h"
#include "VJSContext.h"
#include "VJSValue.h"


BEGIN_TOOLBOX_NAMESPACE


#if !VERSIONWIN
	#define __cdecl
#endif

class XTOOLBOX_API VJSParms_withContext
{	
public:
#if  USE_V8_ENGINE
	explicit								VJSParms_withContext(JS4D::ContextRef inContext, const VJSObject& inObject);
	explicit								VJSParms_withContext(const v8::PropertyCallbackInfo<v8::Value>* info);
	explicit								VJSParms_withContext(const v8::FunctionCallbackInfo<v8::Value>& args);
	explicit								VJSParms_withContext(const v8::PropertyCallbackInfo<void>* info);
	explicit								VJSParms_withContext(const v8::PropertyCallbackInfo<v8::Array>& info);
#else
	explicit								VJSParms_withContext( JS4D::ContextRef inContext, JS4D::ObjectRef inObject);
#endif
											~VJSParms_withContext();

			const VJSContext&				GetContext() const;

			void							ProtectValue( JS4D::ValueRef inValue) const;
			void							UnprotectValue( JS4D::ValueRef inValue) const;

			const VJSObject&				GetObject() const;

			UniChar							GetWildChar() const;
			
protected:
			VJSObject						fObject;
};


//======================================================
class XTOOLBOX_API VJSParms_withException : public VJSParms_withContext
{
public:
#if USE_V8_ENGINE
	explicit								VJSParms_withException(const v8::FunctionCallbackInfo<v8::Value>& args);
	explicit								VJSParms_withException(const v8::PropertyCallbackInfo<v8::Value>* info);
	explicit								VJSParms_withException(const v8::PropertyCallbackInfo<void>* info);
	explicit								VJSParms_withException(const v8::PropertyCallbackInfo<void>& info);
#else
	explicit								VJSParms_withException( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException);
#endif
											~VJSParms_withException();

			void							EvaluateScript( const XBOX::VString& inScript, XBOX::VValueSingle** outResult);
			void							SetException(const VJSException& inException);

protected:
#if USE_V8_ENGINE
	mutable	VJSException					fException;
#else
			JS4D::ExceptionRef*				fException;
#endif
			VErrorTaskContext*				fTaskContext;
			VErrorContext*					fErrorContext;
};

//======================================================

class XTOOLBOX_API VJSParms_withReturnValue : public virtual VJSParms_withException
{
public:
#if USE_V8_ENGINE
	explicit								VJSParms_withReturnValue(const v8::FunctionCallbackInfo<v8::Value>& args);
	explicit								VJSParms_withReturnValue(const v8::FunctionCallbackInfo<v8::Value>& args, v8::Value* inConstructedValue);
#else
	explicit								VJSParms_withReturnValue( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException);
			const VJSValue&					GetReturnValue() const;
#endif
			
			template<class Type>
			void				ReturnNumber( const Type& inValue);

			void				ReturnBool( bool inValue);

			void				ReturnString( const XBOX::VString& inString);

			void				ReturnTime( const XBOX::VTime& inTime);

			void				ReturnDuration(const XBOX::VDuration& inDuration);

			void				ReturnVValue(const XBOX::VValueSingle& inValue, bool simpleDate);

			void				ReturnFile( XBOX::VFile *inFile);

			void				ReturnFolder( XBOX::VFolder *inFolder);

			void				ReturnFilePathAsFileOrFolder( const XBOX::VFilePath& inPath);
				
			void				ReturnValue( const VJSValue& inValue);
			void				ReturnJSONValue( const VJSONValue& inValue);

			void				ReturnNullValue();
			void				ReturnUndefinedValue();

protected:
#if USE_V8_ENGINE
			void				ReturnDouble( const double& inValue);	
	const	v8::FunctionCallbackInfo<v8::Value>*	fV8Arguments;
#else
			VJSValue			fReturnValue;
#endif
};

//======================================================

//======================================================

class XTOOLBOX_API VJSParms_withArguments : public virtual VJSParms_withException
{
public:
#if USE_V8_ENGINE
	explicit					VJSParms_withArguments(const v8::FunctionCallbackInfo<v8::Value>& args);
#else
	explicit					VJSParms_withArguments( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException);
#endif

			size_t				CountParams() const;
			
			// cast specified param into some type
			// returns false if index out of bound
			// index starts at 1.
			bool				GetStringParam( size_t inIndex, XBOX::VString& outString) const;
			bool				GetLongParam( size_t inIndex, sLONG *outValue) const;
			bool				GetLong8Param( size_t inIndex, sLONG8 *outValue) const;
			bool				GetULongParam( size_t inIndex, uLONG *outValue) const;
			bool				GetRealParam( size_t inIndex, Real *outValue) const;
			bool				GetBoolParam( size_t inIndex, bool *outValue) const;
			bool				GetBoolParam( size_t inIndex, const XBOX::VString& inNameForTrue, const XBOX::VString& inNameForFalse, size_t* outIndex = NULL) const;

			XBOX::VFile*		RetainFileParam( size_t inIndex, bool allowPath = true) const;
			XBOX::VFolder*		RetainFolderParam( size_t inIndex, bool allowPath = true) const;

			VJSValue			GetParamValue( size_t inIndex) const;
			XBOX::VValueSingle*	CreateParamVValue( size_t inIndex, bool simpleDate = false) const;
			bool				GetParamObject( size_t inIndex, VJSObject& outObject) const;
			bool				GetParamArray( size_t inIndex, VJSArray& outArray) const;
			bool				GetParamFunc( size_t inIndex, VJSObject& outObject, bool allowString = false) const;
			bool				GetParamJSONValue( size_t inIndex, VJSONValue& outValue) const;
			
			// returns true if param doesn't exist or is null (no exception is thrown)
			bool				IsNullParam( size_t inIndex) const;

			// returns true if param doesn't exist or is undefined
			bool				IsUndefinedParam( size_t inIndex) const;

			// returns true if param doesn't exist or is null or undefined.
			// this is usually what you use to test for an optional parameter.
			bool				IsNullOrUndefinedParam( size_t inIndex) const;
			
			// tells how is defined the javascript param
			bool				IsNumberParam( size_t inIndex) const;
			bool				IsBooleanParam( size_t inIndex) const;
			bool				IsStringParam( size_t inIndex) const;
			bool				IsObjectParam( size_t inIndex) const;	// null is not an object for c++
			bool				IsArrayParam( size_t inIndex) const;
			bool				IsFunctionParam( size_t inIndex) const;

			// returns the private data of specified param if it is a native object of class NATIVE_CLASS, NULL if it is not.
			// exception if index out of bound.
			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType *GetParamObjectPrivateData( size_t inIndex) const;

			
#if USE_V8_ENGINE
			VJSObject									fConstructedObject;
protected:
			const v8::FunctionCallbackInfo<v8::Value>* 	fV8Arguments; 
private:
#else
private:
            const JS4D::ValueRef*						fArguments;
			size_t										fArgumentCount;
#endif
};

//======================================================

class XTOOLBOX_API VJSParms_callAsConstructor : public VJSParms_withArguments
{
public:
#if USE_V8_ENGINE
	explicit					VJSParms_callAsConstructor(const v8::FunctionCallbackInfo<v8::Value>& args);
			
			void				ReturnConstructedObject(const VJSObject& inObject);
			void				ReturnUndefined();
			template<class T> 
			void				ReturnConstructedObject(typename T::PrivateDataType *inPrivateData);

#else
	explicit					VJSParms_callAsConstructor( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException);
			void				ReturnConstructedObject( const VJSObject& inObject);		
			JS4D::ObjectRef		GetConstructedObjectRef() const; 
#endif
			const VJSObject&	GetConstructedObject() const;
#if USE_V8_ENGINE
protected:
#else
private:
#endif
#if USE_V8_ENGINE
			//void				ReturnConstructedObjectProlog();
#else
			VJSObject			fConstructedObject;
#endif
};


//======================================================
#if USE_V8_ENGINE

class XTOOLBOX_API VJSParms_construct : public VJSParms_withArguments
{
public:
			typedef void (*fn_construct)( VJSParms_construct&);

	explicit					VJSParms_construct( const v8::FunctionCallbackInfo<v8::Value>& args );

			template<fn_construct fn>
			static void __cdecl Callback(VJSParms_callAsConstructor* parms);

			template<class JSCLASS>
			void				ReturnConstructedObject( typename JSCLASS::PrivateDataType *inPrivateData);

			void				ReturnConstructedObject( const VJSObject& inObject);

};

#else
class XTOOLBOX_API VJSParms_construct : public VJSParms_withArguments
{
/*
	Used for instantiating an object with new operator.
	Register your calback with JS4DMakeConstructor.
*/
public:
	explicit					VJSParms_construct(JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException);
			

			typedef void (*fn_construct)( VJSParms_construct&);
			template<fn_construct fn>
			static JS4D::ObjectRef __cdecl Callback( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException);

			template<class JSCLASS>
			void				ReturnConstructedObject( typename JSCLASS::PrivateDataType *inPrivateData);

			void				ReturnConstructedObject( const VJSObject& inObject);

			const VJSObject&	GetConstructedObject() const;

private:

			VJSObject			fConstructedObject;
};
#endif




class XTOOLBOX_API VJSParms_initialize : public VJSParms_withContext
{
public:
#if USE_V8_ENGINE
	explicit	VJSParms_initialize(JS4D::ContextRef inContext, const VJSObject& inObject) : VJSParms_withContext(inContext, inObject) {;}
#else
	explicit	VJSParms_initialize(JS4D::ContextRef inContext, JS4D::ObjectRef inObject) : VJSParms_withContext(inContext, inObject)	{ ; }
#endif	
private:
};


//======================================================


class XTOOLBOX_API VJSParms_finalize
{
public:
	explicit	VJSParms_finalize(JS4D::ObjectRef inObject) /*: fObject( inObject)*/	{ (void)inObject; }

/*
			// UnprotectValue() crashes because there is no context. 
			// JSValueUnprotect(NULL, someValueRef) doesn't work.
			
			void	UnprotectValue( JS4D::ValueRef inValue) const;
*/
	
private:
//			JS4D::ObjectRef			fObject;
};



//======================================================

class XTOOLBOX_API VJSParms_callStaticFunction : public VJSParms_withReturnValue, public VJSParms_withArguments
{
public:
	
#if USE_V8_ENGINE
	explicit					VJSParms_callStaticFunction( const v8::FunctionCallbackInfo<v8::Value>& args );
			JS4D::ContextRef	GetContextRef();
			void*				GetObjectPrivate() { return fThis.GetObjectPrivate(); }
			void				ReturnThis();
			void				ReturnRawValue( void* inData );
#else
	explicit					VJSParms_callStaticFunction(JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException);
			 
			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext);
			void				ReturnThis();
#endif
			
			const VJSObject&	GetThis() const;
private:
			VJSObject			fThis;
};


//======================================================

class XTOOLBOX_API VJSParms_callAsFunction : public VJSParms_withReturnValue, public VJSParms_withArguments
{
public:
#if USE_V8_ENGINE
	explicit					VJSParms_callAsFunction( const v8::FunctionCallbackInfo<v8::Value>& args );
			void				ReturnThis();
#else
	explicit					VJSParms_callAsFunction(JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException);

			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext);
			void				ReturnThis();
#endif
			// Use the returned (function) object to retrieve private C++ data.

			VJSObject			GetFunction ();

			const VJSObject&	GetThis() const;

private:

			VJSObject			fFunction;		// Object which is called as a function.
			VJSObject			fThis;
};




//======================================================

#if USE_V8_ENGINE
class XTOOLBOX_API VJSParms_withQueryPropertyCallbackReturnValue
{
public:

	explicit					VJSParms_withQueryPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<v8::Integer>* inInfo);
protected:
	const v8::PropertyCallbackInfo<v8::Integer>*	info;
};

class XTOOLBOX_API VJSParms_withGetPropertyCallbackReturnValue : public virtual VJSParms_withException
{
public:

	explicit					VJSParms_withGetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<v8::Value>* info);
			
			template<class Type>
			void				ReturnNumber( const Type& inValue) { ReturnDouble(static_cast<double>(inValue)); }
			
			void				ReturnBool( bool inValue);

			void				ReturnString( const XBOX::VString& inString);

			void				ReturnTime( const XBOX::VTime& inTime);

			void				ReturnDuration( const XBOX::VDuration& inDuration);

			void				ReturnVValue(const XBOX::VValueSingle& inValue, bool simpleDate);

			void				ReturnFile( XBOX::VFile *inFile);

			void				ReturnFolder( XBOX::VFolder *inFolder);

			void				ReturnFilePathAsFileOrFolder( const XBOX::VFilePath& inPath);
				
			void				ReturnValue( const VJSValue& inValue);
			void				ReturnJSONValue( const VJSONValue& inValue);

			void				ReturnNullValue();
			void				ReturnUndefinedValue();

			const VJSValue&		GetReturnValue() const;

protected:
			const v8::PropertyCallbackInfo<v8::Value>*	info;
			VJSValue			fReturnValue;
			void				ReturnDouble( double inValue);
						
};

class XTOOLBOX_API VJSParms_withSetPropertyCallbackReturnValue : public virtual VJSParms_withException
{
public:

	explicit			VJSParms_withSetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<void>* info);
	explicit			VJSParms_withSetPropertyCallbackReturnValue(const v8::PropertyCallbackInfo<v8::Value>* info);

	template<class Type>
	void				ReturnNumber(const Type& inValue) { ReturnDouble(static_cast<double>(inValue)); }

	void				ReturnBool(bool inValue);

	void				ReturnString(const XBOX::VString& inString);

	void				ReturnTime(const XBOX::VTime& inTime);

	void				ReturnDuration(const XBOX::VDuration& inDuration);

	void				ReturnVValue(const XBOX::VValueSingle& inValue, bool simpleDate);

	void				ReturnFile(XBOX::VFile *inFile);

	void				ReturnFolder(XBOX::VFolder *inFolder);

	void				ReturnFilePathAsFileOrFolder(const XBOX::VFilePath& inPath);

	void				ReturnValue(const VJSValue& inValue);
	void				ReturnJSONValue(const VJSONValue& inValue);

	void				ReturnNullValue();
	void				ReturnUndefinedValue();

	const VJSValue&		GetReturnValue() const;

protected:
	const v8::PropertyCallbackInfo<void>*		info;
	const v8::PropertyCallbackInfo<v8::Value>*	infoVal;

	void				ReturnDouble(double inValue);

};
#endif

class XTOOLBOX_API VJSParms_getProperty
#if USE_V8_ENGINE
: public VJSParms_withGetPropertyCallbackReturnValue
#else
: public VJSParms_withReturnValue
#endif
{
public:
#if USE_V8_ENGINE
	explicit					VJSParms_getProperty( const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Value>& info);
			void				ForwardToParent () ;
#else
	explicit					VJSParms_getProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException);
			// If you don't call any Returnxxx() function nor throw any exception, JSC will call parent class getProperty accessor.
			// Call ForwardToParent() if you think a Returnxxx() function has been called and you want to cancel it.
			void				ForwardToParent ();
			
			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext);
#endif
	
			bool				GetPropertyName( XBOX::VString& outPropertyName) const;
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;
		
private:
#if USE_V8_ENGINE
			XBOX::VString		fPropertyName;
#else
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
#endif
};


//======================================================
class XTOOLBOX_API VJSParms_setProperty
#if USE_V8_ENGINE
: public VJSParms_withSetPropertyCallbackReturnValue
#else
: public VJSParms_withException
#endif
{
public:
#if USE_V8_ENGINE
	explicit					VJSParms_setProperty(const XBOX::VString& inPropertyName, const v8::Handle<v8::Value>& inValue, const v8::PropertyCallbackInfo<void>& info);
	explicit					VJSParms_setProperty(const XBOX::VString& inPropertyName, const v8::Handle<v8::Value>& inValue, const v8::PropertyCallbackInfo<v8::Value>& info);

			bool				GetPropertyName( XBOX::VString& outPropertyName) const;
			
#else			
	explicit					VJSParms_setProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ValueRef inValue, JS4D::ExceptionRef* outException);
			bool				GetPropertyName( XBOX::VString& outPropertyName) const;

#endif	
			
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
			const VJSValue&		GetPropertyValue() const;
			XBOX::VValueSingle*	CreatePropertyVValue(bool simpleDate) const;
			
			// returns the private data of the property if it is a native object of class NATIVE_CLASS, NULL if it is not.
			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType *GetPropertyObjectPrivateData( ) const;
			
			bool				IsValueNull() const;
			bool				IsValueNumber() const;
			bool				IsValueBoolean() const;
			bool				IsValueString() const;
			bool				IsValueObject() const;
			
private:
			VJSValue			fValue;
#if USE_V8_ENGINE
			XBOX::VString		fPropertyName;
#else
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
#endif
};


//======================================================


class XTOOLBOX_API VJSParms_getPropertyNames : public VJSParms_withContext
{
public:
#if USE_V8_ENGINE
	explicit		VJSParms_getPropertyNames(const v8::PropertyCallbackInfo<v8::Array>& info, std::vector<XBOX::VString>& propertyNames);
#else
	explicit		VJSParms_getPropertyNames(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::PropertyNameAccumulatorRef inPropertyNames);
#endif
			void	ReturnNumericalRange( size_t inBegin, size_t inEnd);
			void	AddPropertyName( const XBOX::VString& inPropertyName);

private:
#if USE_V8_ENGINE
			std::vector<XBOX::VString>&			fPropertyNames;
#else
			JS4D::PropertyNameAccumulatorRef	fPropertyNames;
#endif
};

//======================================================

class XTOOLBOX_API VJSParms_hasProperty :
#if USE_V8_ENGINE
public VJSParms_withQueryPropertyCallbackReturnValue
#else
public VJSParms_withContext
#endif
{
public:

			bool				GetPropertyName( XBOX::VString& outPropertyName) const;
#if USE_V8_ENGINE
	explicit					VJSParms_hasProperty(const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Integer>& inInfo);
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
#else
	explicit					VJSParms_hasProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName);
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
#endif
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;

			// Return true if object has the property. Returning false will "forward to parent", it will checks static values and so on.

			void				ReturnBoolean (bool inBoolean);
			void				ReturnTrue ();
			void				ReturnFalse ();

			void				ForwardToParent ();

			bool				GetReturnedBoolean ();
			
private:
#if USE_V8_ENGINE
			XBOX::VString		fPropertyName;
#else
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
#endif
			bool				fReturnedBoolean;
};

//======================================================


class XTOOLBOX_API VJSParms_deleteProperty
#if USE_V8_ENGINE
#else
	: public VJSParms_withException
#endif
{
public:
			bool				GetPropertyName( XBOX::VString& outPropertyName) const;

#if USE_V8_ENGINE
	explicit					VJSParms_deleteProperty(const XBOX::VString& inPropertyName, const v8::PropertyCallbackInfo<v8::Boolean>& inInfo);
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
#else
	explicit					VJSParms_deleteProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException);
			bool				GetPropertyNameAsLong( sLONG *outValue) const;
#endif
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;

			void				ReturnBoolean (bool inBoolean);
			void				ReturnTrue ();
			void				ReturnFalse ();

			bool				GetReturnedBoolean ();
			
private:
#if USE_V8_ENGINE
			XBOX::VString		fPropertyName;
#else
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
#endif
			bool				fReturnedBoolean;
};

class XTOOLBOX_API VJSParms_hasInstance
#if  USE_V8_ENGINE
#else
	: public VJSParms_withException
#endif
{
public:

	// Callback for "instanceof".
	//
	//		inObjectToTest instanceof inPossibleConstructor
	//
	// Note that you should have assert(inObjectToTest == GetObject.GetObjectRef()).
#if  USE_V8_ENGINE
	explicit						VJSParms_hasInstance() :fPossibleConstructor(0), fObjectToTest(0) { xbox_assert(false); }
			VJSObject	*fFooObj;
			VJSContext	*fCtx;
			const VJSObject&		GetObject() const { return *fFooObj; }
			const VJSContext&		GetContext() const { return *fCtx;	}
#else
	explicit						VJSParms_hasInstance(JS4D::ContextRef inContext, JS4D::ObjectRef inPossibleConstructor, JS4D::ValueRef inObjectToTest, JS4D::ExceptionRef *outException);
#endif
			XBOX::JS4D::ObjectRef	GetPossibleContructor ()	const;
			XBOX::JS4D::ObjectRef	GetObjectToTest ()			const;
			
private:

			XBOX::JS4D::ObjectRef	fPossibleConstructor;
			XBOX::JS4D::ObjectRef	fObjectToTest;
};



#if USE_V8_ENGINE


template<class JSCLASS>
inline	void				VJSParms_construct::ReturnConstructedObject( typename JSCLASS::PrivateDataType *inPrivateData)
{
	if (inPrivateData != NULL)
	{
		//xbox::DebugMsg("VJSParms_construct::ReturnConstructedObject2 called !!!! \n");
		fConstructedObject.SetPrivateData(inPrivateData, (JS4D::ClassDefinition*)JSCLASS::Class());
		VJSParms_initialize	parms(fConstructedObject.fContext,fConstructedObject);
		JSCLASS::Initialize(parms, inPrivateData);
	}
	else
	{
		fConstructedObject.ClearRef();
	}
}

inline	void				VJSParms_construct::ReturnConstructedObject( const VJSObject& inObject)	
{ 
	//xbox::DebugMsg("VJSParms_construct::ReturnConstructedObject called !!!! \n");
	(void)inObject;
}

#else

template<VJSParms_construct::fn_construct fn>
inline	JS4D::ObjectRef __cdecl		VJSParms_construct::Callback( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
{
	JS4D::ObjectRef result;
	{
		VJSParms_construct parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
		fn( parms);
		result = parms.GetConstructedObject().GetObjectRef();
	}
	// don't return NULL without exception or JSC will crash
	if ( (result == NULL) && (*outException == NULL) )
	{
		*outException = JS4D::MakeException( inContext, CVSTR( "Failed to create object."));
	}
	return result;
}

template<class JSCLASS>
inline	void						VJSParms_construct::ReturnConstructedObject( typename JSCLASS::PrivateDataType *inPrivateData)
{
	if (inPrivateData != NULL)
	{
		fConstructedObject = JSCLASS::CreateInstance( GetContext(), inPrivateData );
	}
	else
	{
		fConstructedObject.ClearRef();
	}
}

inline	void					VJSParms_construct::ReturnConstructedObject( const VJSObject& inObject)	
{ 
	fConstructedObject = inObject;
}

inline	const VJSObject&		VJSParms_construct::GetConstructedObject() const			
{ 
	return fConstructedObject;
}

#endif


template<class NATIVE_CLASS>
typename NATIVE_CLASS::PrivateDataType *VJSParms_withArguments::GetParamObjectPrivateData( size_t inIndex) const
{
#if USE_V8_ENGINE
    JS4D::ClassRef classRef = NATIVE_CLASS::Class();
    //if (InternalGetParamObjectPrivateData((JS4D::ClassDefinition*)classRef,inIndex))
    {
        VJSValue value( GetParamValue( inIndex));
        return value.GetObjectPrivateData<NATIVE_CLASS>();
    }
    /*else
    {
		VJSValue value(GetParamValue(inIndex));
		NATIVE_CLASS::PrivateDataType* tmp = value.GetObjectPrivateData<NATIVE_CLASS>();
        return NULL;
    }*/

#else
    VJSValue value( GetParamValue( inIndex));
	return value.GetObjectPrivateData<NATIVE_CLASS>();
#endif
}


#if USE_V8_ENGINE
#else
inline	const VJSValue&		VJSParms_withReturnValue::GetReturnValue() const
{ 
	return fReturnValue;
}
#endif
			
template<class Type>
inline void					VJSParms_withReturnValue::ReturnNumber( const Type& inValue)	
{
#if USE_V8_ENGINE
	ReturnDouble(static_cast<double>(inValue));
#else
	VJSException	except;
	fReturnValue.SetNumber( inValue, &except);
	if (fException)
	{
		*fException = except.GetValue();
	}
#endif
}


#if  USE_V8_ENGINE
inline	VJSParms_withContext::VJSParms_withContext(JS4D::ContextRef inContext, const VJSObject& inObject)
: fObject(inObject)
{
	xbox_assert(inContext == inObject.GetContext());
}

#else
inline	VJSParms_withContext::VJSParms_withContext( JS4D::ContextRef inContext, JS4D::ObjectRef inObject) 
: fObject( inContext, inObject)
{
	if (!inObject)
	{
		int a = 1;
	}
	else
	{
		int a = 1;
	}
}
#endif

inline	const VJSContext&	VJSParms_withContext::GetContext() const
{ 
	return fObject.GetContext();
}

inline	const VJSObject&	VJSParms_withContext::GetObject() const					
{ 
	return fObject;
}

inline	UniChar				VJSParms_withContext::GetWildChar() const
{ 
	return GetContext().GetWildChar();
}


		
inline	const VJSObject&		VJSParms_callStaticFunction::GetThis() const
{ 
	return fThis;
}


inline	VJSObject				VJSParms_callAsFunction::GetFunction ()					
{ 
	return fFunction;	
}

inline	const VJSObject&		VJSParms_callAsFunction::GetThis() const		
{ 
	return fThis;
}
		

inline	bool					VJSParms_setProperty::GetPropertyNameAsLong( sLONG *outValue) const		
{ 
	return JS4D::StringToLong( fPropertyName, outValue);
}

inline	const VJSValue&			VJSParms_setProperty::GetPropertyValue() const	
{ 
	return fValue;
}

inline	XBOX::VValueSingle*		VJSParms_setProperty::CreatePropertyVValue(bool simpleDate) const
{ 
#if USE_V8_ENGINE
	XBOX::VValueSingle* res = fValue.CreateVValue(&fException, simpleDate);
#else
	VJSException tmpExc;
	XBOX::VValueSingle* res = fValue.CreateVValue(&tmpExc, simpleDate);
	if (fException)
	{
		*fException = tmpExc.GetValue();
	}
#endif
	return res;
}

	
template<class NATIVE_CLASS>
inline	typename NATIVE_CLASS::PrivateDataType *	VJSParms_setProperty::GetPropertyObjectPrivateData( ) const
{
#if USE_V8_ENGINE
	{
		return fValue.GetObjectPrivateData<NATIVE_CLASS>();
	}
#else
	if (fValue.GetValueRef() != NULL)
	{
		JS4D::ClassRef classRef = NATIVE_CLASS::Class();
		return static_cast<typename NATIVE_CLASS::PrivateDataType*>(
			JS4D::ValueIsObjectOfClass(	GetContext(),
										fValue.GetValueRef(),
										classRef)
			? JS4D::GetObjectPrivate(JS4D::ValueToObject(GetContext(), fValue.GetValueRef(), fException))
			: NULL);
	}
	else
		return NULL;
#endif
}
			
inline	bool				VJSParms_setProperty::IsValueNull() const		
{ 
	return fValue.IsNull();
}
inline	bool				VJSParms_setProperty::IsValueNumber() const			
{ 
	return fValue.IsNumber();
}
inline	bool				VJSParms_setProperty::IsValueBoolean() const			
{ 
	return fValue.IsBoolean();
}
inline	bool				VJSParms_setProperty::IsValueString() const			
{ 
	return fValue.IsString();
}
inline	bool				VJSParms_setProperty::IsValueObject() const	
{ 
	return fValue.IsObject();
}

#if USE_V8_ENGINE
inline	bool				VJSParms_hasProperty::GetPropertyName(XBOX::VString& outPropertyName) const
{
	outPropertyName = fPropertyName;
	return true;
}
#else
inline	bool				VJSParms_hasProperty::GetPropertyName(XBOX::VString& outPropertyName) const
{
	return JS4D::StringToVString(fPropertyName, outPropertyName);
}
#endif


inline	void					VJSParms_hasProperty::ReturnBoolean (bool inBoolean)		
{ 
	fReturnedBoolean = inBoolean;	
}
inline	void					VJSParms_hasProperty::ReturnTrue ()	
{ 
	fReturnedBoolean = true;	
}
inline	void					VJSParms_hasProperty::ReturnFalse ()	
{ 
	fReturnedBoolean = false;	
}

inline	void					VJSParms_hasProperty::ForwardToParent ()
{ 
	ReturnFalse();				
}

inline	bool					VJSParms_hasProperty::GetReturnedBoolean ()		
{ 
	return fReturnedBoolean;		
}
			
inline	bool					VJSParms_deleteProperty::GetPropertyName( XBOX::VString& outPropertyName) const
{
#if USE_V8_ENGINE
	outPropertyName = fPropertyName;
	return true;
#else
	return JS4D::StringToVString( fPropertyName, outPropertyName);
#endif
}


inline	void					VJSParms_deleteProperty::ReturnBoolean (bool inBoolean)		
{ 
	fReturnedBoolean = inBoolean;	
}
inline	void					VJSParms_deleteProperty::ReturnTrue ()		
{ 
	fReturnedBoolean = true;	
}
inline	void					VJSParms_deleteProperty::ReturnFalse ()	
{ 
	fReturnedBoolean = false;		
}

inline	bool					VJSParms_deleteProperty::GetReturnedBoolean ()	
{ 
	return fReturnedBoolean;		
}
			

inline	XBOX::JS4D::ObjectRef	VJSParms_hasInstance::GetPossibleContructor ()	const
{	
	return fPossibleConstructor;	
}	

inline	XBOX::JS4D::ObjectRef	VJSParms_hasInstance::GetObjectToTest ()	const	
{	
	return fObjectToTest;	
}

#if USE_V8_ENGINE

inline	JS4D::ContextRef		VJSParms_callStaticFunction::GetContextRef() 
{ 
	return fThis.GetContextRef(); 
}


inline	void					VJSParms_getProperty::ForwardToParent () 
{ 
	fReturnValue = VJSValue(fReturnValue.GetContext());	
} 

inline	bool					VJSParms_setProperty::GetPropertyName( XBOX::VString& outPropertyName) const
{ 
	outPropertyName = fPropertyName; 
	return true; 
}
inline	bool					VJSParms_hasProperty::GetPropertyNameAsLong( sLONG *outValue) const	
{ 
	return JS4D::StringToLong(fPropertyName, outValue);
} 

inline	bool					VJSParms_deleteProperty::GetPropertyNameAsLong( sLONG *outValue) const			
{
	(void)outValue;
	xbox_assert(false);
	return 0;  // GH JS4D::StringToLong( fPropertyName, outValue);}
}


template<class T>
inline	void					VJSParms_callAsConstructor::ReturnConstructedObject(typename T::PrivateDataType* /*inPrivateData*/)
{
	xbox_assert(false);
	//ReturnConstructedObjectProlog();
	//T::CreateInstance( GetContext(), inPrivateData, fConstructedObject);
}

#else

inline	void					VJSParms_callAsConstructor::ReturnConstructedObject( const VJSObject& inObject)	
{ 
	fConstructedObject = inObject;
}

inline	JS4D::ObjectRef			VJSParms_callAsConstructor::GetConstructedObjectRef() const
{
	return fConstructedObject.GetObjectRef();
}

inline	void					VJSParms_callStaticFunction::ReturnThis()
{ 
	fReturnValue = fThis;
}

inline	void					VJSParms_callAsFunction::ReturnThis()						
{ 
	fReturnValue = fThis;
}

inline	void					VJSParms_getProperty::ForwardToParent () 	
{ 
	fReturnValue = VJSValue(fReturnValue.GetContext());	
} 
			
inline	JS4D::ValueRef			VJSParms_getProperty::GetValueRefOrNull(JS4D::ContextRef inContext)		
{ 
	return GetReturnValue().GetValueRef(); 
}	

inline	bool					VJSParms_setProperty::GetPropertyName( XBOX::VString& outPropertyName) const
{ 
	return JS4D::StringToVString( fPropertyName, outPropertyName);
}

inline	bool					VJSParms_hasProperty::GetPropertyNameAsLong( sLONG *outValue) const	
{
	return JS4D::StringToLong( fPropertyName, outValue);
}
inline	bool					VJSParms_deleteProperty::GetPropertyNameAsLong( sLONG *outValue) const
{ 
	return JS4D::StringToLong( fPropertyName, outValue);
}
#endif

inline	const VJSObject&		VJSParms_callAsConstructor::GetConstructedObject() const
{ 
	return fConstructedObject;
}


END_TOOLBOX_NAMESPACE

#endif

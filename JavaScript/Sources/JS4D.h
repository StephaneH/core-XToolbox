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
#ifndef __JS4D__
#define __JS4D__


#include "Graphics/VGraphics.h"

#if  USE_V8_ENGINE
BEGIN_TOOLBOX_NAMESPACE
	class VJSParms_callAsConstructor;
	class VJSParms_callAsFunction;
END_TOOLBOX_NAMESPACE

	namespace v8 {
		//class HandleScope;
		//class Context;
		class Integer;
		class Isolate;
		class Value;
		class Date;
		class String;
		//template<class T> class NonCopyablePersistentTraits;
		//template<class T,class M> class Persistent;
		//template <class T> class Persistent;
		template<typename T> class FunctionCallbackInfo;
		template<class T> class Handle;
		template <class T> class Local;
		typedef void (*FunctionCallback)(const FunctionCallbackInfo<Value>& info);
		template<typename T> class PropertyCallbackInfo;
		typedef void (*AccessorGetterCallback)(
			Local<String> property,
			const PropertyCallbackInfo<Value>& info);

		typedef void (*AccessorSetterCallback)(
			Local<String> property,
			Local<Value> value,
			const PropertyCallbackInfo<void>& info);
	}
	#define K_WAK_PRIVATE_DATA_ID	"objectPrvData"
#else
// redeclare JavaScriptCore basic types

struct OpaqueJSContextGroup;
struct OpaqueJSContext;
struct OpaqueJSPropertyNameAccumulator;
struct OpaqueJSValue;
#endif

#if VERSIONMAC
namespace WK4D
{
#endif

	struct OpaqueJSClass;
	struct OpaqueJSContext;
	struct OpaqueJSValue;
	struct OpaqueJSString;
	struct OpaqueJSPropertyNameArray;
	struct OpaqueJSPropertyNameAccumulator;

#if VERSIONMAC
};
#endif

#if !VERSIONMAC
#define WK4D
#endif

BEGIN_TOOLBOX_NAMESPACE

class VJSValue;
class VJSObject;
class VJSParms_getProperty;
class VJSParms_setProperty;
class VJSParms_hasProperty;

#if !VERSIONWIN
	#define __cdecl
#endif


class XTOOLBOX_API JS4D
{
public:
#if  USE_V8_ENGINE
	//typedef v8::Handle<v8::Context>*						v8ContextRef;
	typedef	void*											ClassRef;
	typedef	v8::Isolate*									ContextRef;
	typedef	v8::Value*										ValueRef;
	typedef v8::Handle<v8::Value>*							HandleValueRef;
	typedef	v8::Value*										ObjectRef;
	typedef	v8::Value*										ExceptionRef;
#else
//#error "GH SHOULD NOT PASS HERE on V8!!!!!"
	typedef	struct WK4D::OpaqueJSClass*						ClassRef;
	typedef	const struct WK4D::OpaqueJSContext*				ContextRef;
	typedef	const struct WK4D::OpaqueJSValue*				ValueRef;
	typedef	struct WK4D::OpaqueJSValue*						ObjectRef;
	typedef	const struct WK4D::OpaqueJSValue*				ExceptionRef;
#endif
#if  USE_V8_ENGINE
	typedef	v8::Isolate*									GlobalContextRef;
	typedef	void*											StringRef;
	typedef std::vector<XBOX::VString>						PropertyNameArrayRef;
#else
	typedef	struct WK4D::OpaqueJSString*					StringRef;
	typedef	struct WK4D::OpaqueJSContext*					GlobalContextRef;
	typedef struct WK4D::OpaqueJSPropertyNameArray*			PropertyNameArrayRef;
#endif
	typedef unsigned										ClassAttributes;
	typedef unsigned										PropertyAttributes;
	typedef struct WK4D::OpaqueJSPropertyNameAccumulator*	PropertyNameAccumulatorRef;

	enum {
#if  USE_V8_ENGINE
		PropertyAttributeNone         = 0,		//PropertyAttribute.None,
		PropertyAttributeReadOnly     = 1<<0,	// PropertyAttribute.ReadOnly,
		PropertyAttributeDontEnum     = 1<<1,	// PropertyAttribute.DontEnum,
		PropertyAttributeDontDelete   = 1<<2	// PropertyAttribute.DontDelete
#else
		PropertyAttributeNone         = 0,		// kJSPropertyAttributeNone,
		PropertyAttributeReadOnly     = 1<<1,	// kJSPropertyAttributeReadOnly,
		PropertyAttributeDontEnum     = 1<<2,	// kJSPropertyAttributeDontEnum,
		PropertyAttributeDontDelete   = 1<<3	// kJSPropertyAttributeDontDelete
#endif
	};

	enum { 
		ClassAttributeNone						= 0,	// kJSClassAttributeNone,
		ClassAttributeNoAutomaticPrototype		= 1>>1	// kJSClassAttributeNoAutomaticPrototype
	};


	// Types returned by GetType().
	enum EType {
	
		// A VJSValue with a NULL ValueRef is considered as undefined.

		eTYPE_UNDEFINED	= 0,	// kJSTypeUndefined, 

		// null is a special JavaScript value.

		eTYPE_NULL		= 1,	// kJSTypeNull,

		// Primitive types.

    	eTYPE_BOOLEAN	= 2,	// kJSTypeBoolean,
    	eTYPE_NUMBER	= 3,	// kJSTypeNumber,
		eTYPE_STRING	= 4,	// kJSTypeString,

		// An object can be a function, an Array, etc.

		eTYPE_OBJECT	= 5,	// kJSTypeObject 

	};

	typedef void (__cdecl *ObjectInitializeCallback) ( ContextRef ctx, ObjectRef object);
	typedef void (__cdecl *ObjectFinalizeCallback) ( ObjectRef object);
#if  USE_V8_ENGINE
//	typedef void (__cdecl *ObjectCallAsConstructorCallback) (VJSParms_callAsConstructor* parms);
	typedef void (__cdecl *ObjectCallAsConstructorCallback) (const v8::FunctionCallbackInfo<v8::Value>& args);

	class V8ObjectGetPropertyCallback;

	typedef void (*ObjectGetPropertyCallback) (XBOX::VJSParms_getProperty* ioParms, void *inData);
	class V8ObjectSetPropertyCallback;
	typedef void (*ObjectSetPropertyCallback) (XBOX::VJSParms_setProperty* ioParms, void *inData);
	typedef void(*ObjectHasPropertyCallback) (XBOX::VJSParms_hasProperty* ioParms, void *inData);
#else
	typedef ObjectRef (__cdecl *ObjectCallAsConstructorCallback) ( ContextRef ctx, ObjectRef constructor, size_t argumentCount, const ValueRef arguments[], ExceptionRef* exception);
	typedef ValueRef (__cdecl *ObjectGetPropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ExceptionRef* exception);
	typedef bool (__cdecl *ObjectSetPropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ValueRef value, ExceptionRef* exception);
	typedef bool(__cdecl *ObjectHasPropertyCallback) (ContextRef ctx, ObjectRef object, StringRef propertyName); 
#endif
	typedef bool (__cdecl *ObjectDeletePropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ExceptionRef* exception);
#if  USE_V8_ENGINE
	typedef void(__cdecl *ObjectGetPropertyNamesCallback) (ContextRef ctx, ObjectRef object, std::vector<XBOX::VString>& outPropertyNames);
	typedef void (__cdecl *ObjectCallAsFunctionCallback) (const v8::FunctionCallbackInfo<v8::Value>& args);
	//typedef void (__cdecl *ObjectCallAsConstructorFunctionCallback) (VJSParms_callAsFunction* parms);
#else
	typedef void (__cdecl *ObjectGetPropertyNamesCallback) ( ContextRef ctx, ObjectRef object, PropertyNameAccumulatorRef propertyNames);
	typedef ValueRef (__cdecl *ObjectCallAsFunctionCallback) ( ContextRef ctx, ObjectRef function, ObjectRef thisObject, size_t argumentCount, const ValueRef arguments[], ExceptionRef* exception);
#endif
	typedef bool (__cdecl *ObjectHasInstanceCallback)  ( ContextRef ctx, ObjectRef constructor, ValueRef possibleInstance, ExceptionRef* exception);
	typedef ValueRef (__cdecl *ObjectConvertToTypeCallback) ( ContextRef ctx, ObjectRef object, EType type, ValueRef* exception);

	static	XBOX::VValueSingle*		ValueToVValue( ContextRef inContext, ValueRef inValue, ExceptionRef *outException = NULL, bool simpleDate = false);

	// produces a js null value if VValueSingle is null
	static	void					VValueToValue(VJSValue& ioValue, const XBOX::VValueSingle& inValue, ExceptionRef *outException, bool simpleDate);
	static	void					_VJSONValueToValue(VJSValue& ioValue, const VJSONValue& inValue, JS4D::ExceptionRef *outException, std::map<VJSONObject*, JS4D::ObjectRef>& ioConvertedObjects);

	// converts a JavaScriptCore value into a xbox VJSONValue.
	static	void					VJSONValueToValue(VJSValue& ioValue, const XBOX::VJSONValue& inValue, ExceptionRef *outException);
	static	bool					ValueToVJSONValue( ContextRef inContext, ValueRef inValue, VJSONValue& outJSONValue, ExceptionRef *outException);

	// return NULL if VFile or VFolder is NULL.
	// warning: NULL is not a valid ObjectRef, and we can't return a js null or undefined value because these are not objects.
	static	void					VFileToObject(VJSObject& ioObject, XBOX::VFile *inFile, ExceptionRef *outException);
	static	void					VFolderToObject(XBOX::VJSObject& ioObject, XBOX::VFolder *inFolder, ExceptionRef *outException);
	static	void					VFilePathToObjectAsFileOrFolder(XBOX::VJSObject& ioObject, const XBOX::VFilePath& inPath, ExceptionRef *outException);
#if USE_V8_ENGINE
	static	bool					StringToVString( void* inJSString, XBOX::VString& outString);
#else
	static	bool					ValueToString(ContextRef inContext, ValueRef inValue, XBOX::VString& outString, ExceptionRef *outException = NULL);
	static	StringRef				VStringToString( const XBOX::VString& inString);
	static	bool					StringToVString( StringRef inJSString, XBOX::VString& outString);
#endif	
	// various wakanda apis accepts accepts a file path or an url. This method centralizes the detection algorithm.
	// returns true if the url is valid
	static	bool					GetURLFromPath( const VString& inPath, XBOX::VURL& outURL);
	

#if USE_V8_ENGINE
	// returns false if the string doesn't appear to be a valid positive or negative 32bits integer.
	static	bool					StringToLong( const XBOX::VString& inJSString, sLONG *outValue);
#else
	// returns false if the string doesn't appear to be a valid positive or negative 32bits integer.
	static	bool					StringToLong( StringRef inJSString, sLONG *outValue);
	static	ValueRef				VStringToValue(ContextRef inContext, const XBOX::VString& inString);
#endif
	
	static	VJSValue				VPictureToValue(ContextRef inContext, const XBOX::VPicture& inPict);

	static	VJSValue				VBlobToValue(ContextRef inContext, const VBlob& inBlob, const VString& inContentType = CVSTR("application/octet-stream"));

	static	void					BoolToValue( VJSValue& ioValue, bool inValue);
	
#if USE_V8_ENGINE
	static	void					DoubleToValue(ContextRef inContext, double inValue, VJSValue& ioValue);
	static	void					VStringToValue(const XBOX::VString& inString, VJSValue& ioValue);
#else
	static	ValueRef				DoubleToValue( ContextRef inContext, double inValue);
#endif

	static	sLONG8					GetDebugContextId( ContextRef inContext );


template<class Type>
#if USE_V8_ENGINE
	static	void					NumberToValue(ContextRef inContext, const Type& inValue, VJSValue& ioValue)
	{
		DoubleToValue(inContext, static_cast<double>(inValue), ioValue);
	}
#else
	static	ValueRef				NumberToValue( ContextRef inContext, const Type& inValue)
				{ return DoubleToValue( inContext, static_cast<double>( inValue));}
#endif

	// produces a Date object
	// warning: returns NULL and not a js null value if the VTime is null
#if USE_V8_ENGINE
	static	void					VTimeToObject(ContextRef inContext, const XBOX::VTime& inDate, ValueRef ioPersVal, ExceptionRef *outException, bool simpleDate);
#else
	static	ObjectRef				VTimeToObject(ContextRef inContext, const XBOX::VTime& inDate, ExceptionRef *outException, bool simpleDate);
#endif
	// fills a VTime from a js object that implements getTime() function.
	// in case of failure, VTime::IsNull() returns true.
	// it's caller responsibility to check inObject is really a Date using ValueIsInstanceOf
#if USE_V8_ENGINE
	static	bool					DateObjectToVTime(ContextRef inContext, ValueRef, XBOX::VTime& outTime, ExceptionRef *outException, bool simpleDate);
#else
	static	bool					DateObjectToVTime( ContextRef inContext, ObjectRef inObject, XBOX::VTime& outTime, ExceptionRef *outException, bool simpleDate);
#endif	
	// produces a number containing the duration in milliseconds
	// produces a js null value if VDuration is null
#if USE_V8_ENGINE
	static	void					VDurationToValue(ContextRef inContext, const XBOX::VDuration& inDuration, VJSValue& ioValue);
#else
	static	ValueRef				VDurationToValue( ContextRef inContext, const XBOX::VDuration& inDuration);
#endif

#if USE_V8_ENGINE
	static	bool					ValueToVDuration( ContextRef inContext, const v8::Handle<v8::Value>& inValue, XBOX::VDuration& outDuration, ExceptionRef *outException);
#else
	static	bool					ValueToVDuration( ContextRef inContext, ValueRef inValue, XBOX::VDuration& outDuration, ExceptionRef *outException);
#endif
	
	// evaluates some javascript text and returns the result in outValue.
	static	void					EvaluateScript( ContextRef inContext, const XBOX::VString& inScript, VJSValue& outValue, ExceptionRef *outException);

	// Tests whether a JavaScript value's type is the undefined type.
	static	bool					ValueIsUndefined( ContextRef inContext, ValueRef inValue);
	
	static	VJSValue				MakeUndefined(ContextRef inContext);
	static	VJSValue				MakeNull( ContextRef inContext);

	// Tests whether a JavaScript value's type is the null type.
	static	bool					ValueIsNull( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the boolean type.
	static	bool					ValueIsBoolean( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the number type.
	static	bool					ValueIsNumber( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the string type.
	static	bool					ValueIsString( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the object type.
	static	bool					ValueIsObject( ContextRef inContext, ValueRef inValue);
	
#if USE_V8_ENGINE
	//static	ObjectRef				ValueToObject( ContextRef inContext, v8::Value* inPersValue, ExceptionRef* outException);
#endif
	static	ObjectRef				ValueToObject( ContextRef inContext, ValueRef inValue, ExceptionRef* outException);
	static	ObjectRef				MakeObject( ContextRef inContext, ClassRef inClassRef, void* inPrivateData);

	// Tests whether a JavaScript value is an object with a given class in its class chain.
	// true if value is an object and has jsClass in its class chain, otherwise false.
#if USE_V8_ENGINE
	static	bool					ValueIsObjectOfClass(ContextRef inContext, ValueRef inValue, ClassRef inClassRef);
	static	bool					ObjectIsFunction( ContextRef inContext, ValueRef inObjectValue);
	// An object is a value so there's usually no need for a converter,
	// but ObjectToValue will produces a js null value if being passed a NULL ObjectRef
	static	void					ObjectToValue(XBOX::VJSValue& ioValue, const XBOX::VJSObject inObject); 
#else
	static	bool					ValueIsObjectOfClass( ContextRef inContext, ValueRef inValue, ClassRef inClassRef);
	static	bool					ObjectIsFunction( ContextRef inContext, ObjectRef inObjectRef);
	// An object is a value so there's usually no need for a converter,
	// but ObjectToValue will produces a js null value if being passed a NULL ObjectRef
	static	ValueRef				ObjectToValue(ContextRef inContext, ObjectRef inObject); 
#endif
	
	// same result as instanceof javascript operator 
	static	bool					ValueIsInstanceOf( ContextRef inContext, ValueRef inValue, const XBOX::VString& inConstructorName, ExceptionRef *outException);

	// Return type of value, note that eTYPE_OBJECT is very broad (an object can be a function, an Array, a Date, a String, etc.).
	static	EType					GetValueType( ContextRef inContext, ValueRef inValue);

	// creates a function object from some js code as body
	static	ObjectRef				MakeFunction( ContextRef inContext, const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, ExceptionRef *outException);

	// call a function
	static	ValueRef				CallFunction (const ContextRef inContext, const ObjectRef inFunctionObject, const ObjectRef inThisObject, sLONG inNumberArguments, const ValueRef *inArguments, ExceptionRef *ioException);
	
	/*
		JSC Docmentation extract :
		"JavaScript values that are on the machine stack, in a register, 
		protected by JSValueProtect, set as the global object of an execution context, 
		or reachable from any such value will not be collected.

		During JavaScript execution, you are not required to call this function; the 
		JavaScript engine will garbage collect as needed. JavaScript values created
		within a context group are automatically destroyed when the last reference
		to the context group is released."

		A value may be protected multiple times and must be unprotected an equal number of times before becoming eligible for garbage collection.

		To force garbage collection, call VJSContext::GarbageCollect
 		
	*/
	static	void					ProtectValue( ContextRef inContext, ValueRef inValue);
	static	void					UnprotectValue( ContextRef inContext, ValueRef inValue);

	// if being passed a non null javascript exception, throw an xbox VError that describe the exception.
	// returns true if a VError was thrown.
	static	bool					ThrowVErrorForException( ContextRef inContext, ExceptionRef inException);

	// If the current VTask error context is not empty, and if outException is not NULL, creates a javascript exception object and returns true.
	static	bool					ConvertErrorContextToException( ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException);

#if  !USE_V8_ENGINE
	// Creates a standard and simple Exception object with a message string
	static	ExceptionRef			MakeException( ContextRef inContext, const VString& inMessage);
#endif

#if  USE_V8_ENGINE
	// Gets an object's private data.
	static	void*					GetObjectPrivate(ContextRef inContext, ObjectRef inObject);
	static	void					SetObjectPrivate(ContextRef inContext, ObjectRef inObject, void* inData);
#else
	// Gets an object's private data.
	static	void*					GetObjectPrivate( ObjectRef inObject);
#endif


	/*
		Converts a JavaScript string into a null-terminated UTF8 string, 
		and copies the result into an external byte buffer.
		The destination byte buffer into which to copy a null-terminated 
		UTF8 representation of string. On return, buffer contains a UTF8 string 
		representation of string. If bufferSize is too small, buffer will contain only 
		partial results. If buffer is not at least bufferSize bytes in size, 
		behavior is undefined. 
		returns the number of bytes written into buffer (including the null-terminator byte).
	*/
	static	size_t					StringRefToUTF8CString( StringRef inStringRef, void *outBuffer, size_t inBufferSize);


	typedef struct {
		const char* name;
		ObjectGetPropertyCallback getProperty;
		ObjectSetPropertyCallback setProperty;
		PropertyAttributes attributes;
	} StaticValue;

	typedef struct {
		const char* name;
		ObjectCallAsFunctionCallback callAsFunction;
		PropertyAttributes attributes;
	} StaticFunction;

	typedef struct ClassDefinition_st {
		ClassDefinition_st()
		:attributes(0)
		,className(NULL)
		,parentClass(NULL)
		,staticValues(NULL)
		,staticFunctions(NULL)
		,initialize(NULL)
		,finalize(NULL)
		,hasProperty(NULL)
		,getProperty(NULL)
		,setProperty(NULL)
		,deleteProperty(NULL)
		,getPropertyNames(NULL)
		,callAsFunction(NULL)
		,callAsConstructor(NULL)
		,hasInstance(NULL)
		{
			;
		}
		ClassAttributes						attributes;

		const char*							className;
		ClassRef							parentClass;
			
		const StaticValue*					staticValues;
		const StaticFunction*				staticFunctions;
		
		ObjectInitializeCallback			initialize;
		ObjectFinalizeCallback				finalize;
		ObjectHasPropertyCallback			hasProperty;
		ObjectGetPropertyCallback			getProperty;
		ObjectSetPropertyCallback			setProperty;
		ObjectDeletePropertyCallback		deleteProperty;
		ObjectGetPropertyNamesCallback		getPropertyNames;
#if  USE_V8_ENGINE
		ObjectCallAsFunctionCallback		callAsFunction;
//		ObjectCallAsConstructorFunctionCallback		callAsFunction;
#else
		ObjectCallAsFunctionCallback		callAsFunction;
#endif
		ObjectCallAsConstructorCallback		callAsConstructor;
		ObjectHasInstanceCallback			hasInstance;
	} ClassDefinition;

	static	ClassRef				ClassCreate( const ClassDefinition* inDefinition);
};

END_TOOLBOX_NAMESPACE

#endif

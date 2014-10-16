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


#if USE_V8_ENGINE
#include <v8.h>
#include <V4DContext.h>
using namespace v8;
#include <string.h>

#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "VJSValue.h"
#include "VJSJSON.h"
#include "VJSRuntime_Image.h"
#include "VJSRuntime_blob.h"
#include "VJSRuntime_file.h"
#include "VJSW3CFileSystem.h"

USING_TOOLBOX_NAMESPACE



#if USE_V8_ENGINE

#else
bool JS4D::ValueToString( ContextRef inContext, ValueRef inValue, VString& outString, ExceptionRef *outException)
{
	bool ok=false;

	JSStringRef jsString = JSValueToStringCopy( inContext, inValue, outException);
	ok = StringToVString( jsString, outString);
	if (jsString != NULL)
		JSStringRelease( jsString);
	return ok;
}

JS4D::StringRef JS4D::VStringToString( const VString& inString)
{
	JS4D::StringRef	jsString;

	jsString = JSStringCreateWithCharacters( inString.GetCPointer(), inString.GetLength());
	return jsString;
}

#endif

bool JS4D::StringToVString( StringRef inJSString, VString& outString)
{
	bool ok;
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
#else
	if (inJSString != NULL)
	{
		size_t length = JSStringGetLength( inJSString);
		UniChar *p = outString.GetCPointerForWrite( length);
		if (p != NULL)
		{
			::memcpy( p, JSStringGetCharactersPtr( inJSString), length * sizeof( UniChar));
			ok = outString.Validate( length);
		}
		else
		{
			outString.Clear();
			ok = false;
		}
	}
	else
	{
		ok = false;
		outString.Clear();
	}
#endif
	return ok;
}

#if USE_V8_ENGINE


#else

JS4D::ValueRef JS4D::VStringToValue(ContextRef inContext, const VString& inString)
{
	ValueRef value;

	if (inString.IsNull())
		return JSValueMakeNull(inContext);

	JSStringRef jsString = JSStringCreateWithCharacters(inString.GetCPointer(), inString.GetLength());
	value = JSValueMakeString(inContext, jsString);
	JSStringRelease(jsString);

	return value;
}
#endif

#if USE_V8_ENGINE
bool JS4D::StringToLong(const VString& inJSString, sLONG *outResult)
#else
bool JS4D::StringToLong( StringRef inJSString, sLONG *outResult)
#endif
{
	bool ok = false;
#if USE_V8_ENGINE
	if (inJSString.GetLength() > 0)
	{
		VStringConvertBuffer bufferTmp(inJSString, VTC_UTF_16);
		//DebugMsg("JS4D::StringToLong vstrLen=%d bufConv.len=%d bufConv.siz=%d\n", inJSString.GetLength(), bufferTmp.GetLength(), bufferTmp.GetSize());
		const UniChar* p = (UniChar*)bufferTmp.GetCPointer();
		const UniChar* p_end = p + (bufferTmp.GetLength()/2);
#else
	if (inJSString != NULL)
	{
		const JSChar* p = JSStringGetCharactersPtr(inJSString);
		const JSChar* p_end = p + JSStringGetLength(inJSString); 
#endif

		if (p != p_end)
		{
			// neg sign ?
			ok = true;
			bool neg;
			if (*p != CHAR_HYPHEN_MINUS)
			{
				neg = false;
			}
			else
			{
				neg = true;
				++p;
			}

			sLONG8 result = 0;

			while(p != p_end)
			{
				if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
				{
					result *= 10;
					result += *p - CHAR_DIGIT_ZERO;
					++p;
				}
				else
				{
					ok = false;
					break;
				}
			}

			// overflow?
			if (result > MaxLongInt)
				ok = false;

			if (ok && neg)
				result = -result;

			if (ok && (outResult != NULL))
				*outResult = static_cast<sLONG>( result);
		}
	}
	return ok;
}



#if USE_V8_ENGINE
void JS4D::BoolToValue(VJSValue& ioValue, bool inValue)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioValue.GetContext());
	v8::HandleScope handle_scope(ioValue.GetContext());
	Context::Scope context_scope(ioValue.GetContext(), *v8PersContext);

	Handle<v8::Boolean>	tmpBool = v8::Boolean::New(inValue);
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	if (tmpBool.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(tmpBool.val_);
		ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
}

VJSValue JS4D::VPictureToValue(ContextRef inContext, const VPicture& inPict)
{
	VJSValue value(JS4D::MakeNull(inContext));
	if (!inPict.IsNull())
	{
		VPicture* newPict = nil;
		if ((&inPict) != nil)
			newPict = new VPicture(inPict);
		VJSContext	vjsContext(inContext);
		VJSPictureContainer* xpic = new VJSPictureContainer(newPict, /*true,*/ vjsContext);
		VJSObject	tmpObj = VJSImage::CreateInstance(vjsContext, xpic);
		xpic->Release();
		JS4D::ObjectToValue(value, tmpObj);
	}

	return value;
}

VJSValue JS4D::VBlobToValue(ContextRef inContext, const VBlob& inBlob, const VString& inContentType)
{
	VJSValue value(inContext);

	if (!inBlob.IsNull())
	{
		VJSBlobValue_Slice* blobValue = VJSBlobValue_Slice::Create(inBlob, inContentType);
		if (blobValue != NULL)
		{
			VJSContext	vjsContext(inContext);
			VJSObject	tmpObj = VJSBlob::CreateInstance(vjsContext, blobValue);
			ReleaseRefCountable(&blobValue);
			JS4D::ObjectToValue(value, tmpObj);
		}
	}
	if (!value.fValue)
	{
		value = JS4D::MakeNull(inContext);
	}
	return value;
}

void JS4D::VStringToValue(const XBOX::VString& inString, VJSValue& ioValue)
{
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioValue.fContext);
	v8::HandleScope handle_scope(ioValue.fContext);
	Context::Scope context_scope(ioValue.fContext, *v8PersContext);

	if (inString.IsNull())
	{
		Handle<Primitive>	tmpNull = v8::Null();
		if (tmpNull.val_)
		{
			internal::Object** p = reinterpret_cast<internal::Object**>(tmpNull.val_);
			ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
				p));
		}
		return;
	}

	Handle<String>	tmpStr = v8::String::NewFromTwoByte(ioValue.fContext, inString.GetCPointer(), v8::String::kNormalString, inString.GetLength());
	if (tmpStr.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(tmpStr.val_);
		ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}

}

#else
void JS4D::BoolToValue(VJSValue& ioValue, bool inValue)
{
	ioValue.fValue = JSValueMakeBoolean(ioValue.fContext, inValue);
}

VJSValue JS4D::VPictureToValue(ContextRef inContext, const VPicture& inPict)
{
	VJSValue value(JS4D::MakeNull(inContext));

	if (!inPict.IsNull())
	{
		VPicture* newPict = nil;
		if ((&inPict) != nil)
			newPict = new VPicture(inPict);
		VJSContext	vjsContext(inContext);
		VJSPictureContainer* xpic = new VJSPictureContainer(newPict, /*true,*/ vjsContext);
		value.fValue = VJSImage::CreateInstance(vjsContext, xpic).GetObjectRef();
		xpic->Release();
	}

	return value;
}

VJSValue JS4D::VBlobToValue(ContextRef inContext, const VBlob& inBlob, const VString& inContentType)
{
	VJSValue value(JS4D::MakeNull(inContext));

	if (!inBlob.IsNull())
	{
		VJSBlobValue_Slice* blobValue = VJSBlobValue_Slice::Create(inBlob, inContentType);
		if (blobValue != NULL)
		{
			VJSContext	vjsContext(inContext);
			value.fValue = VJSBlob::CreateInstance(vjsContext, blobValue).GetObjectRef();
			ReleaseRefCountable(&blobValue);
		}
	}

	return value;
}
#endif

#if USE_V8_ENGINE
void JS4D::DoubleToValue(ContextRef inContext, double inValue, VJSValue& ioValue)
{
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioValue.GetContext());
	v8::HandleScope handle_scope(ioValue.GetContext());
	Context::Scope context_scope(ioValue.GetContext(), *v8PersContext);
	Handle<v8::Number>	tmpNb = v8::Number::New(inValue);
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	if (tmpNb.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(tmpNb.val_);
		ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
}
#else
JS4D::ValueRef JS4D::DoubleToValue( ContextRef inContext, double inValue)
{
	return JSValueMakeNumber( inContext, inValue);
}
#endif

sLONG8 JS4D::GetDebugContextId( ContextRef inContext )
{
#if USE_V8_ENGINE
	return (sLONG8)v8::Isolate::GetCurrent();
#else
	return (sLONG8)JS4DGetDebugContextId(inContext);
#endif
}

#if USE_V8_ENGINE
void JS4D::VTimeToObject(ContextRef inContext, const XBOX::VTime& inDate, ValueRef ioPersVal, ExceptionRef *outException, bool simpleDate)
{
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	
	VTime	unixStartTime;
	unixStartTime.FromUTCTime(1970,1,1,0,0,0,0);
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(inContext);
	v8::HandleScope handle_scope(inContext);
	Context::Scope context_scope(inContext, *v8PersContext);
	double		dateMS = inDate.GetMilliseconds() - unixStartTime.GetMilliseconds();
	Handle<Value>	result = v8::Date::New(dateMS);
}
#else
JS4D::ObjectRef JS4D::VTimeToObject(ContextRef inContext, const VTime& inTime, ExceptionRef *outException, bool simpleDate)
{
	JS4D::ObjectRef date;

	if (inTime.IsNull())
		return NULL;	// can't return JSValueMakeNull as an object
	
	sWORD year, month, day, hour, minute, second, millisecond;
	
	if (simpleDate)
	{
		inTime.GetUTCTime(year, month, day, hour, minute, second, millisecond);
		VString s;
		s = ToString((sLONG)year) + "-";

		if (month < 10)
			s += "0" + ToString((sLONG)month) + "-";
		else
			s +=  ToString((sLONG)month) + "-";

		if (day < 10)
			s += "0" + ToString((sLONG)day);
		else
			s += ToString((sLONG)day);

		s += "T00:00:00.000Z";

		JSValueRef	args[1];
		JSStringRef jsarg = JS4D::VStringToString(s);
		args[0] = JSValueMakeString(inContext, jsarg);
		JSStringRef jsClassName = JSStringCreateWithUTF8CString("Date");
		JSObjectRef constructor = JSValueToObject(inContext, JSObjectGetProperty(inContext, JSContextGetGlobalObject(inContext), jsClassName, NULL), NULL);
		JSStringRelease(jsClassName);
		date = JSObjectCallAsConstructor(inContext, constructor, 1, args, outException);
		JSStringRelease(jsarg);
	}
	else
	{
		inTime.GetLocalTime(year, month, day, hour, minute, second, millisecond);

		JSValueRef	args[7];
		args[0] = JSValueMakeNumber(inContext, year);
		args[1] = JSValueMakeNumber(inContext, month - 1);
		args[2] = JSValueMakeNumber(inContext, day);
		args[3] = JSValueMakeNumber(inContext, hour);
		args[4] = JSValueMakeNumber(inContext, minute);
		args[5] = JSValueMakeNumber(inContext, second);
		args[6] = JSValueMakeNumber(inContext, millisecond);

#if NEW_WEBKIT
		date = JSObjectMakeDate( inContext, 7, args, outException);
#else
		JSStringRef jsClassName = JSStringCreateWithUTF8CString("Date");
		JSObjectRef constructor = JSValueToObject(inContext, JSObjectGetProperty(inContext, JSContextGetGlobalObject(inContext), jsClassName, NULL), NULL);
		JSStringRelease(jsClassName);
		date = JSObjectCallAsConstructor(inContext, constructor, 7, args, outException);
#endif
	}

	return date;
}
#endif

#if USE_V8_ENGINE
void JS4D::VDurationToValue(ContextRef inContext, const XBOX::VDuration& inDuration, VJSValue& ioValue)
{
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}

	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(inContext);
	v8::HandleScope handle_scope(inContext);
	Context::Scope context_scope(inContext, *v8PersContext);
	if (inDuration.IsNull())
	{
		Handle<Primitive>	tmpNull = v8::Null();
		if (tmpNull.val_)
		{
			internal::Object** p = reinterpret_cast<internal::Object**>(tmpNull.val_);
			ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
				p));
		}
		return;
	}
	else
	{
		Handle<Number>	result = v8::Number::New(inDuration.ConvertToMilliseconds());
		if (result.val_)
		{
			internal::Object** p = reinterpret_cast<internal::Object**>(result.val_);
			ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
				p));
		}
	}
}
#else
JS4D::ValueRef JS4D::VDurationToValue( ContextRef inContext, const VDuration& inDuration)
{
	if (inDuration.IsNull())
		return JSValueMakeNull( inContext);

	return JSValueMakeNumber( inContext, inDuration.ConvertToMilliseconds());
}
#endif

#if USE_V8_ENGINE
bool JS4D::ValueToVDuration( ContextRef inContext, const v8::Handle<v8::Value>& inValue, XBOX::VDuration& outDuration, ExceptionRef *outException)
{
	bool ok=false;

	if (inValue->IsNumber())
	{
		outDuration.FromNbMilliseconds(inValue->ToNumber()->Value());
		ok = true;
	}
	else
	{
		outDuration.SetNull(true);
	}
	return ok;
}
#else
bool JS4D::ValueToVDuration( ContextRef inContext, ValueRef inValue, VDuration& outDuration, ExceptionRef *outException)
{
	bool ok=false;
	double r = JSValueToNumber( inContext, inValue, outException);
	sLONG8 n = (sLONG8) r;
	if (n == r)
	{
		outDuration.FromNbMilliseconds( n);
		ok = true;
	}
	else
	{
		outDuration.SetNull( true);
		ok = false;
	}
	return ok;
}
#endif
#if USE_V8_ENGINE
bool JS4D::DateObjectToVTime(ContextRef inContext, ValueRef inDate, XBOX::VTime& outTime, ExceptionRef *outException, bool simpleDate)
{
	bool ok = false;
	if (simpleDate)
	{
		Local<Object>	dateObj = inDate->ToObject();
		Local<Value>	propDateObj = dateObj->Get(String::New("getDate"));
		Local<Value>	propMonthObj = dateObj->Get(String::New("getMonth"));
		Local<Value>	propYearObj = dateObj->Get(String::New("getYear"));
		if (propDateObj->IsFunction() && propMonthObj->IsFunction() && propYearObj->IsFunction())
		{
			double	day = propDateObj->ToObject()->CallAsFunction(dateObj,0,NULL)->ToNumber()->Value();
			double	month = propMonthObj->ToObject()->CallAsFunction(dateObj,0,NULL)->ToNumber()->Value();
			double	year = propYearObj->ToObject()->CallAsFunction(dateObj,0,NULL)->ToNumber()->Value();
			outTime.FromUTCTime(year+1900, month+1, day, 0, 0, 0, 0);
			ok = true;
		}
	}
	else
	{
		Date*	datePtr = Date::Cast(inDate);
		double	nbMS = datePtr->ValueOf();
		outTime.FromUTCTime( 1970, 1, 1, 0, 0, 0, 0);
		outTime.AddMilliseconds(nbMS);
		ok = true;
	}
	
	if (!ok)
		outTime.SetNull(true);
	
	return ok;
}
#else
bool JS4D::DateObjectToVTime( ContextRef inContext, ObjectRef inObject, VTime& outTime, ExceptionRef *outException, bool simpleDate)
{
	// it's caller responsibility to check inObject is really a Date using ValueIsInstanceOf
	
	// call getTime()
	bool ok = false;

	if (simpleDate)
	{
		JSStringRef jsStringGetDate = JSStringCreateWithUTF8CString( "getDate");
		JSStringRef jsStringGetMonth = JSStringCreateWithUTF8CString( "getMonth");
		JSStringRef jsStringGetYear = JSStringCreateWithUTF8CString( "getYear");
		JSValueRef getDate = JSObjectGetProperty( inContext, inObject, jsStringGetDate, outException);
		JSValueRef getMonth = JSObjectGetProperty( inContext, inObject, jsStringGetMonth, outException);
		JSValueRef getYear = JSObjectGetProperty( inContext, inObject, jsStringGetYear, outException);

		JSValueRef day =  JS4DObjectCallAsFunction( inContext, JSValueToObject( inContext, getDate, outException), inObject, 0, NULL, outException);
		JSValueRef month =  JSObjectCallAsFunction( inContext, JSValueToObject( inContext, getMonth, outException), inObject, 0, NULL, outException);
		JSValueRef year =  JSObjectCallAsFunction( inContext, JSValueToObject( inContext, getYear, outException), inObject, 0, NULL, outException);
		JSStringRelease( jsStringGetDate);
		JSStringRelease( jsStringGetMonth);
		JSStringRelease( jsStringGetYear);

		ok = true;
		outTime.FromUTCTime(JSValueToNumber( inContext, year, outException)+1900, JSValueToNumber( inContext, month, outException)+1, JSValueToNumber( inContext, day, outException), 0, 0, 0, 0);
		if (!ok)
			outTime.SetNull( true);
	}
	else
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "getTime");
		JSValueRef getTime = JSObjectGetProperty( inContext, inObject, jsString, outException);
		JSObjectRef getTimeFunction = JSValueToObject( inContext, getTime, outException);
		JSStringRelease( jsString);
		JSValueRef result = (getTime != NULL) ? JS4DObjectCallAsFunction( inContext, getTimeFunction, inObject, 0, NULL, outException) : NULL;
		if (result != NULL)
		{
			// The getTime() method returns the number of milliseconds since midnight of January 1, 1970.
			double r = JSValueToNumber( inContext, result, outException);
			sLONG8 n = (sLONG8) r;
			if (n == r)
			{
				outTime.FromUTCTime( 1970, 1, 1, 0, 0, 0, 0);
				outTime.AddMilliseconds( n);
				ok = true;
			}
			else
			{
				outTime.SetNull( true);
			}
		}
		else
		{
			outTime.SetNull( true);
		}
	}
	return ok;
}
#endif

VValueSingle *JS4D::ValueToVValue( ContextRef inContext, ValueRef inValue, ExceptionRef *outException, bool simpleDate)
{
	if (inValue == NULL)
		return NULL;
	
	VValueSingle *value = NULL;

#if USE_V8_ENGINE

	if (inValue->IsUndefined() || inValue->IsNull())
	{
		value = NULL;	//new VEmpty;
	}

	if (inValue->IsBoolean())
	{
		value = new VBoolean(inValue->ToBoolean()->Value());
	}

	if (inValue->IsNumber())
	{
		value = new VReal(inValue->ToNumber()->Value());
	}
	if (inValue->IsString())
	{
		Handle<Value>	tmpVal = Handle<Value>::New(inContext, inValue);
		String::Value	valStr(tmpVal);

		VString *s = new VString;
		size_t length = valStr.length();
		UniChar *p = s->GetCPointerForWrite(length * sizeof(UniChar));
		if (p != NULL)
		{
			::memcpy(p, *valStr, length * sizeof(UniChar) );
			s->Validate( length);
			value = s;
		}
		else
		{
			delete s;
			value = NULL;
		}
	}

	if (inValue->IsObject())
	{
		ClassDefinition*	classDef = NULL;
		if (inValue->IsDate())
		{
			VTime *time = new VTime;
			if (time != NULL)
			{
				DateObjectToVTime(inContext, inValue, *time, outException, simpleDate);
				value = time;
			}
			else
			{
				value = NULL;
			}
		}
		else if (ValueIsInstanceOf(inContext, inValue, ((ClassDefinition*)VJSFolderIterator::Class())->className, NULL))
		{
			value = NULL;
			JS4DFolderIterator* container = static_cast<JS4DFolderIterator*>(JS4D::GetObjectPrivate(inContext, inValue));
			if (testAssert( container != NULL))
			{
				VFolder* folder = container->GetFolder();
				if (folder != NULL)
				{
					VString *s = new VString;
					if (s != NULL)
						folder->GetPath( *s, FPS_POSIX, true);
					value = s;
				}
			}
		}
		else if (ValueIsInstanceOf(inContext, inValue, ((ClassDefinition*)VJSFileIterator::Class())->className, NULL))
		{
			value = NULL;
			JS4DFileIterator* container = static_cast<JS4DFileIterator*>(JS4D::GetObjectPrivate(inContext, inValue));
			if (testAssert( container != NULL))
			{
				VFile* file = container->GetFile();
				if (file != NULL)
				{
					VString *s = new VString;
					if (s != NULL)
						file->GetPath( *s, FPS_POSIX, true);
					value = s;
				}
			}
		}
		else if (ValueIsInstanceOf(inContext, inValue, ((ClassDefinition*)VJSBlob::Class())->className, NULL))
		{
			// remember File inherits from Blob so one must check File before Blob
			VJSBlob::PrivateDataType* blobValue = static_cast<VJSBlob::PrivateDataType*>(JS4D::GetObjectPrivate(inContext, inValue));
			if (testAssert( blobValue != NULL))
			{
				VJSDataSlice *slice = blobValue->RetainDataSlice();
				VBlobWithPtr *blob = new VBlobWithPtr;
				if ( (blob != NULL) && (slice != NULL) && (slice->GetDataSize() > 0) )
				{
					if (blob->PutData( slice->GetDataPtr(), slice->GetDataSize()) != VE_OK)
					{
						delete blob;
						blob = NULL;
					}
				}
				value = blob;
				ReleaseRefCountable( &slice);
			}
			else
			{
				value = NULL;
			}
		}
		else if (ValueIsInstanceOf(inContext, inValue, ((ClassDefinition*)VJSImage::Class())->className, NULL))
		{
			VJSImage::PrivateDataType* piccontainer = static_cast<VJSImage::PrivateDataType*>(JS4D::GetObjectPrivate(inContext, inValue));
			if (testAssert( piccontainer != NULL))
			{
				VPicture *vpic = piccontainer->GetPict();
				value = (vpic != NULL) ? new VPicture(*vpic) : NULL;
			}
			else
			{
				value = NULL;
			}
		}
		else
		{
			value = NULL;
		}
	}

#else
	JSType type = JSValueGetType( inContext, inValue);
	switch( type)
	{
		case kJSTypeUndefined:
		case kJSTypeNull:
			value = NULL;	//new VEmpty;
			break;
		
		case kJSTypeBoolean:
			value = new VBoolean( JSValueToBoolean( inContext, inValue));
			break;

		case kJSTypeNumber:
			value = new VReal( JSValueToNumber( inContext, inValue, outException));
			break;

		case kJSTypeString:
			{
				JSStringRef jsString = JSValueToStringCopy( inContext, inValue, outException);
				if (jsString != NULL)
				{
					VString *s = new VString;
					size_t length = JSStringGetLength( jsString);
					UniChar *p = (s != NULL) ? s->GetCPointerForWrite( length) : NULL;
					if (p != NULL)
					{
						::memcpy( p, JSStringGetCharactersPtr( jsString), length * sizeof( UniChar));
						s->Validate( length);
						value = s;
					}
					else
					{
						delete s;
						value = NULL;
					}
					JSStringRelease( jsString);
				}
				else
				{
					value = NULL;
				}
				break;
			}
		
		case kJSTypeObject:
			{
				if (ValueIsInstanceOf( inContext, inValue, CVSTR( "Date"), outException))
				{
					VTime *time = new VTime;
					if (time != NULL)
					{
						JSObjectRef dateObject = JSValueToObject( inContext, inValue, outException);

						DateObjectToVTime( inContext, dateObject, *time, outException, simpleDate);
						value = time;
					}
					else
					{
						value = NULL;
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSFolderIterator::Class()))
				{
					value = NULL;
					JS4DFolderIterator* container = static_cast<JS4DFolderIterator*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( container != NULL))
					{
						VFolder* folder = container->GetFolder();
						if (folder != NULL)
						{
							VString *s = new VString;
							if (s != NULL)
								folder->GetPath( *s, FPS_POSIX, true);
							value = s;
						}
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSFileIterator::Class()))
				{
					value = NULL;
					JS4DFileIterator* container = static_cast<JS4DFileIterator*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( container != NULL))
					{
						VFile* file = container->GetFile();
						if (file != NULL)
						{
							VString *s = new VString;
							if (s != NULL)
								file->GetPath( *s, FPS_POSIX, true);
							value = s;
						}
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSBlob::Class()))
				{
					// remember File inherits from Blob so one must check File before Blob
					VJSBlob::PrivateDataType* blobValue = static_cast<VJSBlob::PrivateDataType*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( blobValue != NULL))
					{
						VJSDataSlice *slice = blobValue->RetainDataSlice();
						VBlobWithPtr *blob = new VBlobWithPtr;
						if ( (blob != NULL) && (slice != NULL) && (slice->GetDataSize() > 0) )
						{
							if (blob->PutData( slice->GetDataPtr(), slice->GetDataSize()) != VE_OK)
							{
								delete blob;
								blob = NULL;
							}
						}
						value = blob;
						ReleaseRefCountable( &slice);
					}
					else
					{
						value = NULL;
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSImage::Class()))
				{
					VJSImage::PrivateDataType* piccontainer = static_cast<VJSImage::PrivateDataType*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( piccontainer != NULL))
					{
						VPicture *vpic = piccontainer->GetPict();
						value = (vpic != NULL) ? new VPicture(*vpic) : NULL;
					}
					else
					{
						value = NULL;
					}
				}
				else
				{
					value = NULL;
				}
				break;
			}
		
		default:
			value = NULL;
			break;
	}
#endif
	return value;
}


void JS4D::VValueToValue(VJSValue& ioValue, const VValueSingle& inValue, ExceptionRef *outException, bool simpleDate)
{

#if USE_V8_ENGINE
	JS4D::ContextRef	v8Context = ioValue.fContext;
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioValue.fContext);
	HandleScope			handle_scope(ioValue.fContext);
	Handle<Context>		context = Handle<Context>::New(v8Context,*v8PersContext);
	Context::Scope		context_scope(context);
	
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	ValueKind	valKind = inValue.GetValueKind();
	switch (valKind)
	{
		case VK_BYTE:
		case VK_WORD:
		case VK_LONG:
		case VK_LONG8:
		case VK_REAL:
		case VK_FLOAT:
		case VK_SUBTABLE_KEY:
			if (inValue.IsNull())
			{
				ioValue = JS4D::MakeNull(ioValue.fContext);
			}
			else
			{
				Handle<Number>	tmpNb = v8::Number::New(inValue.GetReal());
				if (tmpNb.val_)
				{
					internal::Object** p = reinterpret_cast<internal::Object**>(tmpNb.val_);
					ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
						reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
						p));
				}
			}
			break;

		case VK_BOOLEAN:
			if (inValue.IsNull())
			{
				ioValue = JS4D::MakeNull(ioValue.fContext);
			}
			else
			{
				Handle<v8::Boolean>	tmpBool = v8::Boolean::New(inValue.GetBoolean());
				if (tmpBool.val_)
				{
					internal::Object** p = reinterpret_cast<internal::Object**>(tmpBool.val_);
					ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
						reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
						p));
				}

			}
			break;

		case VK_UUID:
			if (inValue.IsNull())
			{
				ioValue = JS4D::MakeNull(ioValue.fContext);
			}
			else
			{
				VString tempS1;
				inValue.GetString(tempS1);
				Handle<String>	tmpStr1 = v8::String::NewFromTwoByte(ioValue.fContext, tempS1.GetCPointer(), v8::String::kNormalString, tempS1.GetLength());
				if (tmpStr1.val_)
				{
					internal::Object** p = reinterpret_cast<internal::Object**>(tmpStr1.val_);
					ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
						reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
						p));
				}
			}
			break;
			
		case VK_TEXT:
		case VK_STRING:
		{
#if VERSIONDEBUG
			ioValue.SetString(dynamic_cast<const VString&>(inValue), NULL);
#else
			ioValue.SetString((const VString&)( inValue),NULL);
#endif
			break;
		}

		case VK_TIME:
		{
			if (inValue.IsNull())
			{
				ioValue = JS4D::MakeNull(ioValue.fContext);
			}
			else
			{
#if VERSIONDEBUG
				const VTime&	tmpTime = dynamic_cast<const VTime&>(inValue);
#else
				const VTime&	tmpTime = (const VTime&)(inValue);
#endif
				VTime	unixStartTime;
				unixStartTime.FromUTCTime(1970, 1, 1, 0, 0, 0, 0);
				double		dateMS = tmpTime.GetMilliseconds() - unixStartTime.GetMilliseconds();
				Handle<Value>	tmpDate = v8::Date::New(dateMS);

				if (tmpDate.val_)
				{
					internal::Object** p = reinterpret_cast<internal::Object**>(tmpDate.val_);
					ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
						reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
						p));
				}
			}
			break;
		}

		case VK_DURATION:
		{
#if VERSIONDEBUG
			VDurationToValue(ioValue.fContext, dynamic_cast<const VDuration&>(inValue), ioValue);
#else
			VDurationToValue( ioValue.fContext, (const VDuration&)( inValue),ioValue);
#endif
			break;
		}

		case VK_IMAGE:
			ioValue = VPictureToValue(ioValue.fContext, (const VPicture&)(inValue));
			break;

		case VK_BLOB:
		{
			if (inValue.GetTrueValueKind() == VK_BLOB_OBJ)
			{
				VJSONValue val;
				inValue.GetJSONValue(val);
				if (val.IsNull())
				{
					ioValue = JS4D::MakeNull(ioValue.fContext);
				}
				else if (val.IsUndefined())
				{
					ioValue = JS4D::MakeUndefined(ioValue.fContext);
				}
				else
				{
					VJSContext jscontext(ioValue.fContext);
					VJSJSON json(jscontext);
					VJSException exep;
					VString s;
					val.Stringify(s);

					json.Parse(ioValue,s, &exep);
				}
			}
			else
			{
				ioValue = VBlobToValue(ioValue.fContext, (const VBlob&)(inValue));
			}
			break;
		}

	}
	xbox_assert(ioValue.fValue);
#else
	ioValue.fValue = NULL;
	switch( inValue.GetValueKind())
	{
		case VK_BYTE:
		case VK_WORD:
		case VK_LONG:
		case VK_LONG8:
		case VK_REAL:
		case VK_FLOAT:
		case VK_SUBTABLE_KEY:
			ioValue.fValue = inValue.IsNull() ? JSValueMakeNull(ioValue.fContext) : JSValueMakeNumber(ioValue.fContext, inValue.GetReal());
			break;

		case VK_BOOLEAN:
			ioValue.fValue = inValue.IsNull() ? JSValueMakeNull(ioValue.fContext) : JSValueMakeBoolean(ioValue.fContext, inValue.GetBoolean() != 0);
			break;
			
		case VK_UUID:
			if (inValue.IsNull())
				ioValue.fValue = JSValueMakeNull(ioValue.fContext);
			else
			{
				VString tempS;
				inValue.GetString(tempS);
				ioValue.SetString(tempS, NULL);
			}
			break;
		
		case VK_TEXT:
		case VK_STRING:
#if VERSIONDEBUG
			ioValue.SetString(dynamic_cast<const VString&>(inValue), NULL);
#else
			ioValue.SetString((const VString&)( inValue),NULL);
#endif
			break;
		
		case VK_TIME:
#if VERSIONDEBUG
			ioValue.fValue = inValue.IsNull() ? JSValueMakeNull(ioValue.fContext) : VTimeToObject(ioValue.fContext, dynamic_cast<const VTime&>(inValue), outException, simpleDate);
#else
			ioValue.fValue = inValue.IsNull() ? JSValueMakeNull( ioValue.fContext) : VTimeToObject( ioValue.fContext, (const VTime&)( inValue), outException, simpleDate);
#endif
			break;
		
		case VK_DURATION:
#if VERSIONDEBUG
			ioValue.fValue = VDurationToValue(ioValue.fContext, dynamic_cast<const VDuration&>(inValue));
#else
			ioValue.fValue = VDurationToValue( ioValue.fContext, (const VDuration&)( inValue));
#endif
			break;

		case VK_BLOB:
			if (inValue.GetTrueValueKind() == VK_BLOB_OBJ)
			{
				VJSONValue val;
				inValue.GetJSONValue(val);
				if (val.IsNull())
					ioValue.fValue = JSValueMakeNull(ioValue.fContext);
				else if (val.IsUndefined())
					ioValue.fValue = JSValueMakeUndefined(ioValue.fContext);
				else
				{
					VJSContext jscontext(ioValue.fContext);
					VJSException exep;
					/*
					VJSJSON json(jscontext);
					VString s;
					val.Stringify(s);

					json.Parse(ioValue,s, &exep);
					*/
					ioValue.SetJSONValue(val, &exep);
				}
			}
			else
				ioValue = VBlobToValue(ioValue.fContext, (const VBlob&)(inValue));
			break;

		case VK_IMAGE:
			ioValue = VPictureToValue(ioValue.fContext, (const VPicture&)(inValue));
			break;

		case VK_BLOB_OBJ:
			{
				VJSONValue val;
				inValue.GetJSONValue(val);
				if (val.IsNull())
					ioValue.fValue = JSValueMakeNull(ioValue.fContext);
				else if (val.IsUndefined())
					ioValue.fValue = JSValueMakeUndefined(ioValue.fContext);
				else
				{
					VJSContext jscontext(ioValue.fContext);
					VJSException exep;
					/*
					VJSJSON json(jscontext);
					VString s;
					val.Stringify(s);

					json.Parse(ioValue,s, &exep);
					*/
					ioValue.SetJSONValue(val, &exep);
				}
			}
			break;

		default:
			ioValue.fValue = JSValueMakeUndefined(ioValue.fContext);	// exception instead ?
			break;
	}
#endif
}

#if USE_V8_ENGINE
void JS4D::_VJSONValueToValue(VJSValue& ioValue, const VJSONValue& inValue, JS4D::ExceptionRef *outException, std::map<VJSONObject*, JS4D::ObjectRef>& ioConvertedObjects)
{
	JS4D::ContextRef	v8Context = ioValue.fContext;
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioValue.fContext);
	HandleScope			handle_scope(ioValue.fContext);
	Handle<Context>		context = Handle<Context>::New(v8Context,*v8PersContext);
	Context::Scope		context_scope(context);
	JS4D::ValueRef exception = NULL;

	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}

	VJSContext	vjsContext(ioValue.fContext);

	switch (inValue.GetType())
	{
	case JSON_undefined:
	{
		ioValue = JS4D::MakeUndefined(vjsContext);
		break;
	}

	case JSON_null:
	{
		ioValue = JS4D::MakeNull(vjsContext);
		break;
	}

	case JSON_true:
	{
		ioValue.SetBool(true);
		break;
	}

	case JSON_false:
	{
		ioValue.SetBool(false);
		break;
	}

	case JSON_string:
	{
		VString s;
		inValue.GetString( s);
		JS4D::VStringToValue(s, ioValue);
		break;
	}

	case JSON_number:
	{
		ioValue.SetNumber(inValue.GetNumber());
		break;
	}

	case JSON_array:
	{
		JS4D::ObjectRef array = NULL;
		const VJSONArray *jsonArray = inValue.GetArray();
		size_t count = jsonArray->GetCount();
		if (count == 0)
		{
			Local<Object>	tmpArr = v8::Array::New(0);
			if (tmpArr.val_)
			{
				internal::Object** p = reinterpret_cast<internal::Object**>(tmpArr.val_);
				array = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
					reinterpret_cast<internal::Isolate*>(v8Context),
					p));
			}
		}
		else
		{
			Local<Array>	tmpArr = v8::Array::New(count);
			if (tmpArr.val_ != NULL)
			{
				if (tmpArr.val_)
				{
					internal::Object** p = reinterpret_cast<internal::Object**>(tmpArr.val_);
					array = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
						reinterpret_cast<internal::Isolate*>(v8Context),
						p));
				}
				for( size_t i = 0 ; (i < count) && (exception == NULL) ; ++i)
				{
					VJSValue	tmpVal(ioValue.fContext);
					_VJSONValueToValue(tmpVal, (*jsonArray)[i], &exception, ioConvertedObjects);
					if (tmpVal.fValue == NULL)
					{
						V8::DisposeGlobal(reinterpret_cast<internal::Object**>(array));
						array = NULL;
						break;
					}
					Handle<Value>	hdlVal = Handle<Value>::New(vjsContext, tmpVal.fValue);
					tmpArr->Set(i, hdlVal);
				}
			}
		}
		ioValue.fValue = array;
		break;
	}

	case JSON_object:
	{
		std::pair<std::map<VJSONObject*, JS4D::ObjectRef>::iterator, bool> i_ConvertedObjects = ioConvertedObjects.insert(std::map<VJSONObject*, JS4D::ObjectRef>::value_type(inValue.GetObject(), NULL));

		if (!i_ConvertedObjects.second)
		{
			// already converted object

			//set a ref before putting it into ioValue
			internal::Object** p = reinterpret_cast<internal::Object**>(i_ConvertedObjects.first->second);
			ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
				reinterpret_cast<internal::Isolate*>(v8Context),
				p));
		}
		else
		{
			Local<Object>	tmpObj = v8::Object::New();
			JS4D::ObjectRef jsObject = NULL;
			if (tmpObj.val_)
			{
				// set a reference before adding the object to i_ConvertedObjects
				internal::Object** p = reinterpret_cast<internal::Object**>(tmpObj.val_);
				jsObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
					reinterpret_cast<internal::Isolate*>(v8Context),
					p));
			}
			if (jsObject != NULL)
			{
				xbox_assert(i_ConvertedObjects.first->second == NULL);
				i_ConvertedObjects.first->second = jsObject;
				i_ConvertedObjects.first = ioConvertedObjects.end();	// calling _VJSONValueToValue recursively make this iterator invalid

				for (VJSONPropertyConstOrderedIterator i(inValue.GetObject()); i.IsValid() && (exception == NULL); ++i)
				{
					const VString& name = i.GetName();

					VJSValue	tmpVal(ioValue.fContext);
					_VJSONValueToValue(tmpVal, i.GetValue(), &exception, ioConvertedObjects);
					if ((tmpVal.fValue != NULL) && (name.GetLength() > 0))
					{
						Handle<Value>	hdlVal = Handle<Value>::New(vjsContext, tmpVal.fValue);
						Handle<String>	keyStr = v8::String::NewFromTwoByte(vjsContext, name.GetCPointer(), v8::String::kNormalString, name.GetLength());
						tmpObj->Set(
							keyStr,
							hdlVal,
							(v8::PropertyAttribute)JS4D::PropertyAttributeNone);
					}
					else
					{
						V8::DisposeGlobal(reinterpret_cast<internal::Object**>(jsObject));
						jsObject = NULL;
						break;
					}

				}
				//set a refrence before putting the object into ioValue
				internal::Object** p = reinterpret_cast<internal::Object**>(jsObject);
				ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
					reinterpret_cast<internal::Isolate*>(v8Context),
					p));
			}
		}
		break;
	}

	default:
		xbox_assert( false);
		break;
	}
}
#else
void JS4D::_VJSONValueToValue(VJSValue& ioValue, const VJSONValue& inValue, JS4D::ExceptionRef *outException, std::map<VJSONObject*, JS4D::ObjectRef>& ioConvertedObjects)
{
	VJSContext	vjsContext(ioValue.fContext);
	JS4D::ValueRef exception = NULL;
	switch( inValue.GetType())
	{
		case JSON_undefined:
			{
				ioValue.fValue = JSValueMakeUndefined(vjsContext);
				break;
			}

		case JSON_null:
			{
				ioValue.fValue = JSValueMakeNull(vjsContext);
				break;
			}

		case JSON_true:
			{
				ioValue.fValue = JSValueMakeBoolean(vjsContext, true);
				break;
			}

		case JSON_false:
			{
				ioValue.fValue = JSValueMakeBoolean(vjsContext, false);
				break;
			}
			
		case JSON_string:
			{
				VString s;
				inValue.GetString( s);
				JSStringRef jsString = JSStringCreateWithCharacters( s.GetCPointer(), s.GetLength());
				ioValue.fValue = JSValueMakeString(vjsContext, jsString);
				JSStringRelease( jsString);
				break;
			}

		case JSON_date:
			{
				VTime dd;
				inValue.GetTime(dd);
				ioValue.fValue = JS4D::VTimeToObject(vjsContext, dd, &exception, false);
				break;
			}
			
		case JSON_number:
			{
				ioValue.fValue = JSValueMakeNumber(vjsContext, inValue.GetNumber());
				break;
			}

		case JSON_array:
			{
				JSObjectRef array = NULL;
				const VJSONArray *jsonArray = inValue.GetArray();
				size_t count = jsonArray->GetCount();
				if (count == 0)
				{
					array = JSObjectMakeArray(vjsContext, 0, NULL, &exception);
				}
				else
				{
					JS4D::ValueRef *values = new JS4D::ValueRef[count];
					if (values != NULL)
					{
						JS4D::ValueRef *currentValue = values;
						for( size_t i = 0 ; (i < count) && (exception == NULL) ; ++i)
						{
							VJSValue	tmpVal(ioValue.fContext);
							_VJSONValueToValue(tmpVal, (*jsonArray)[i], &exception, ioConvertedObjects);
							if (tmpVal.fValue == NULL)
								break;
							JSValueProtect(vjsContext, tmpVal.fValue);
							*currentValue++ = tmpVal.fValue;
						}
						if (currentValue == values + count)
						{
							array = JSObjectMakeArray(vjsContext, count, values, &exception);
						}
						do
						{
							JSValueUnprotect(vjsContext, *--currentValue);
						} while( currentValue != values);
					}
					delete [] values;
				}
				ioValue.fValue = array;
				break;
			}

		case JSON_object:
			{
				std::pair<std::map<VJSONObject*,JSObjectRef>::iterator,bool> i_ConvertedObjects = ioConvertedObjects.insert( std::map<VJSONObject*,JSObjectRef>::value_type( inValue.GetObject(), NULL));
				
				if (!i_ConvertedObjects.second)
				{
					// already converted object
					ioValue.fValue = i_ConvertedObjects.first->second;
				}
				else
				{
					JSObjectRef jsObject = JSObjectMake(vjsContext, NULL, NULL);
					if (jsObject != NULL)
					{
						xbox_assert( i_ConvertedObjects.first->second == NULL);
						i_ConvertedObjects.first->second = jsObject;
						i_ConvertedObjects.first = ioConvertedObjects.end();	// calling _VJSONValueToValue recursively make this iterator invalid

						for( VJSONPropertyConstOrderedIterator i( inValue.GetObject()) ; i.IsValid() && (exception == NULL) ; ++i)
						{
							const VString& name = i.GetName();
							JSStringRef jsName = JSStringCreateWithCharacters( name.GetCPointer(), name.GetLength());

							VJSValue	tmpVal(ioValue.fContext);
							_VJSONValueToValue(tmpVal, i.GetValue(), &exception, ioConvertedObjects);
							if ((tmpVal.fValue != NULL) && (jsName != NULL))
							{
								JSObjectSetProperty(vjsContext, jsObject, jsName, tmpVal.fValue, JS4D::PropertyAttributeNone, &exception);
							}
							else
							{
								jsObject = NULL;	// garbage collector will hopefully collect this partially built object
								break;
							}

							if (jsName != NULL)
								JSStringRelease( jsName);
						}
					}
					ioValue.fValue = jsObject;
				}
				break;
			}
		
		default:
			xbox_assert( false);
			ioValue.fValue = NULL;
			break;
	}

	if (outException != NULL)
		*outException = exception;
}
#endif

void JS4D::VJSONValueToValue(VJSValue& ioValue, const VJSONValue& inValue, JS4D::ExceptionRef *outException)
{
	std::map<VJSONObject*,JS4D::ObjectRef> convertedObjects;

	_VJSONValueToValue(ioValue, inValue, outException, convertedObjects);

#if USE_V8_ENGINE
	std::map<VJSONObject*, JS4D::ObjectRef>::iterator	itObjs = convertedObjects.begin();
	while (itObjs != convertedObjects.end())
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>((*itObjs).second));
		itObjs++;
	}
#endif
}

#if USE_V8_ENGINE

#else

JS4D::ExceptionRef JS4D::MakeException( JS4D::ContextRef inContext, const VString& inMessage)
{
	JSStringRef jsString = JS4D::VStringToString( inMessage);
	JSValueRef argumentsErrorValues[] = { JSValueMakeString( inContext, jsString) };
	JSStringRelease( jsString);

	return JSObjectMakeError( inContext, 1, argumentsErrorValues, NULL);
}


static bool _ValueToVJSONValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue, VJSONValue& outJSONValue, JS4D::ExceptionRef *outException, std::map<JSObjectRef,VJSONObject*>& ioConvertedObjects, sLONG& ioStackLevel)
{
	if (inValue == NULL)
	{
		outJSONValue.SetUndefined();
		return true;
	}
	
	if (ioStackLevel > 100)
	{
		// navigator.mimeTypes[0].enabledPlugin is cyclic and we can't detect it because enabledPlugin returns a different object for each call
		*outException = JS4D::MakeException( inContext, CVSTR( "Maximum call stack size exceeded."));	// same error as webkit
		outJSONValue.SetUndefined();
		return false;
	}
	
	++ioStackLevel;
	
	bool ok = true;
	JS4D::ValueRef exception = NULL;
	JSType type = JSValueGetType( inContext, inValue);
	switch( type)
	{
		case kJSTypeUndefined:
			outJSONValue.SetUndefined();
			break;
		
		case kJSTypeNull:
			outJSONValue.SetNull();
			break;
		
		case kJSTypeBoolean:
			outJSONValue.SetBool( JSValueToBoolean( inContext, inValue));
			break;

		case kJSTypeNumber:
			outJSONValue.SetNumber( JSValueToNumber( inContext, inValue, &exception));
			break;

		case kJSTypeString:
			{
				JSStringRef jsString = JSValueToStringCopy( inContext, inValue, &exception);
				VString s;
				ok = JS4D::StringToVString( jsString, s);
				if (ok)
				{
					outJSONValue.SetString( s);
				}
				else
				{
					outJSONValue.SetUndefined();
				}
				if (jsString != NULL)
					JSStringRelease( jsString);
				break;
			}
		
		case kJSTypeObject:
			{
				if (JS4D::ValueIsInstanceOf(inContext, inValue, CVSTR("Date"), &exception))
				{
					JSObjectRef dateObject = JSValueToObject(inContext, inValue, &exception);
					VTime dd;
					if (exception == nil)
					{
						JS4D::DateObjectToVTime(inContext, dateObject, dd, &exception, false);
						if (exception != nil)
							ok = false;
					}
					else
						ok = false;

					if (ok)
						outJSONValue.SetTime(dd);
					else
						outJSONValue.SetUndefined();
				}
				else if (JS4D::ValueIsInstanceOf( inContext, inValue, CVSTR( "Array"), &exception))
				{
					// get count of array elements
					JSObjectRef arrayObject = JSValueToObject( inContext, inValue, &exception);
					JSStringRef jsString = JSStringCreateWithUTF8CString( "length");
					JSValueRef result = (jsString != NULL) ? JSObjectGetProperty( inContext, arrayObject, jsString, &exception) : NULL;
					double r = (result != NULL) ? JSValueToNumber( inContext, result, NULL) : 0;
					size_t length = (size_t) r;
					if (jsString != NULL)
						JSStringRelease( jsString);

					VJSONArray *jsonArray = new VJSONArray;
					if ( (jsonArray != NULL) && jsonArray->Resize( length) && (exception == NULL) )
					{
						for( size_t i = 0 ; (i < length) && ok && (exception == NULL) ; ++i)
						{
							JSValueRef elemValue = JSObjectGetPropertyAtIndex( inContext, arrayObject, i, &exception);
							if (elemValue != NULL)
							{
								VJSONValue value;
								ok = _ValueToVJSONValue( inContext, elemValue, value, &exception, ioConvertedObjects, ioStackLevel);
								jsonArray->SetNth( i+1, value);
							}
						}
					}
					else
					{
						ok = false;
					}

					if (ok && (exception == NULL) )
					{
						outJSONValue.SetArray( jsonArray);
					}
					else
					{
						outJSONValue.SetUndefined();
					}
					ReleaseRefCountable( &jsonArray);
				}
				else
				{
					JSObjectRef jsObject = JSValueToObject( inContext, inValue, &exception);

					if (JSObjectIsFunction( inContext, jsObject))
					{
						// skip function objects (are also skipped by JSON.stringify and produce cyclic structures)
						outJSONValue.SetUndefined();
						ok = true;	// not fatal not to break conversion process
					}
					else
					{
						// if the object has a function property named 'toJSON', let's use it instead of collecting its properties
						// this is so that native objects like Date() are properly interpreted.
						JSStringRef jsFunctionName = JSStringCreateWithUTF8CString( "toJSON");
						JSValueRef toJSONValue = JSObjectGetProperty( inContext, jsObject, jsFunctionName, &exception);
						JSStringRelease( jsFunctionName);
						JSObjectRef toJSONObject = ((toJSONValue != NULL) && JSValueIsObject( inContext, toJSONValue)) ? JSValueToObject( inContext, toJSONValue, &exception) : NULL;
						if ( (toJSONObject != NULL) && JSObjectIsFunction( inContext, toJSONObject))
						{
							JSValueRef jsonValue = JS4DObjectCallAsFunction( inContext, toJSONObject, jsObject, 0, NULL, &exception);
							if (_ValueToVJSONValue( inContext, jsonValue, outJSONValue, &exception, ioConvertedObjects, ioStackLevel))
							{
								// if we got a string we need to parse it as JSON
								if (outJSONValue.IsString())
								{
									StErrorContextInstaller errorContext( false, false);
									VString s, jsonString;
									outJSONValue.GetString( s);
									s.GetJSONString( jsonString, JSON_WithQuotesIfNecessary);
									if (VJSONImporter::ParseString( jsonString, outJSONValue, VJSONImporter::EJSI_Strict) != VE_OK)
									{
										JS4D::ConvertErrorContextToException( inContext, errorContext.GetContext(), &exception);
										outJSONValue.SetUndefined();
										ok = false;
									}
								}
							}
						}
						else
						{
							std::pair<std::map<JSObjectRef,VJSONObject*>::iterator,bool> i_ConvertedObjects = ioConvertedObjects.insert( std::map<JSObjectRef,VJSONObject*>::value_type( jsObject, NULL));
							
							if (!i_ConvertedObjects.second)
							{
								// already converted object
								outJSONValue.SetObject( i_ConvertedObjects.first->second);
							}
							else
							{
								VJSONObject *jsonObject = new VJSONObject;
								
								xbox_assert( i_ConvertedObjects.first->second == NULL);
								i_ConvertedObjects.first->second = jsonObject;
								i_ConvertedObjects.first = ioConvertedObjects.end();	// calling _ValueToVJSONValue recursively make this iterator invalid

								// get prototype object to filter out its properties

								JSValueRef prototypeValue = JSObjectGetPrototype( inContext, jsObject);
								JSObjectRef prototypeObject = (prototypeValue != NULL) ? JSValueToObject( inContext, prototypeValue, &exception) : NULL;
								
								JSPropertyNameArrayRef namesArray = JSObjectCopyPropertyNames( inContext, jsObject);
								if ( (jsonObject != NULL) && (namesArray != NULL) )
								{
									size_t size = JSPropertyNameArrayGetCount( namesArray);
									for( size_t i = 0 ; (i < size) && ok ; ++i)
									{
										JSStringRef jsName = JSPropertyNameArrayGetNameAtIndex( namesArray, i);
										
										// just like JSON.stringify or structure cloning, skip properties from the prototype chain
										if ( (prototypeObject == NULL) || !JSObjectHasProperty( inContext, prototypeObject, jsName))
										{
											VString name;
											ok = JS4D::StringToVString( jsName, name);

											// some property of 'window' sometimes throw an exception when accessing them
											JS4D::ValueRef propertyException = NULL;
											JSValueRef valueRef = JSObjectGetProperty( inContext, jsObject, jsName, &propertyException);
											if ( ok && (propertyException == NULL) && (valueRef != NULL) )
											{
												VJSONValue jsonValue;
												if (ok)
													ok = _ValueToVJSONValue( inContext, valueRef, jsonValue, &exception, ioConvertedObjects, ioStackLevel);

												if (ok)
													ok = jsonObject->SetProperty( name, jsonValue);
											}
										}

										//JSStringRelease( jsName);	// owned by namesArray
									}
								}
								else
								{
									ok = false;
								}

								if (namesArray != NULL)
									JSPropertyNameArrayRelease( namesArray);

								if (ok && (exception == NULL) )
								{
									outJSONValue.SetObject( jsonObject);
								}
								else
								{
									outJSONValue.SetUndefined();
								}
								ReleaseRefCountable( &jsonObject);
							}
						}
					}
				}
				break;
			}
		
		default:
			outJSONValue.SetUndefined();
			ok = false;
			break;
	}
	
	if (outException != NULL)
		*outException = exception;

	--ioStackLevel;
	
	return ok && (exception == NULL);
}
#endif

bool JS4D::ValueToVJSONValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue, VJSONValue& outJSONValue, JS4D::ExceptionRef *outException)
{
	bool ok = false;
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
#else
	std::map<JSObjectRef,VJSONObject*> convertedObjects;
	sLONG stackLevel = 0;
	ok = _ValueToVJSONValue( inContext, inValue, outJSONValue, outException, convertedObjects, stackLevel);
#endif
	return ok;
}


bool JS4D::GetURLFromPath( const VString& inPath, XBOX::VURL& outURL)
{
	VIndex index = inPath.FindRawString( CVSTR( "://"));
	if (index > 0)//URL
	{
		outURL.FromString( inPath, true);
	}
	else if (!inPath.IsEmpty()) //system path
	{
		outURL.FromFilePath( inPath, eURL_POSIX_STYLE);
	}
	else
	{
		outURL.Clear();
	}

	return outURL.IsConformRFC();
}

void JS4D::VFileToObject(VJSObject& ioObject, XBOX::VFile *inFile, ExceptionRef *outException)
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioObject.fContext);
	v8::HandleScope handle_scope(ioObject.fContext);
	Context::Scope		context_scope(ioObject.fContext, *v8PersContext);
	JS4DFileIterator* file = (inFile == NULL) ? NULL : new JS4DFileIterator(inFile);
	if (ioObject.fObject)
	{
		V4DContext::SetObjectPrivateData(ioObject.fContext, ioObject.fObject, file);
		//VJSParms_initialize	parms(ioObject.fContext,ioObject.fObject);
		JS4D::ClassDefinition*	classDef = (JS4D::ClassDefinition*)VJSFileIterator::Class();
		classDef->initialize(ioObject.fContext, ioObject.fObject);
	}
	else
	{
		VJSContext	vjsContext(ioObject.fContext);
		ioObject = VJSFileIterator::CreateInstance(vjsContext, file);
	}
	ReleaseRefCountable(&file);

#else
	ioObject.fObject = NULL;

	JS4DFileIterator* file = (inFile == NULL) ? NULL : new JS4DFileIterator( inFile);
	ioObject.fObject = (file != NULL) ? JSObjectMake(ioObject.fContext, VJSFileIterator::Class(), file) : NULL;
	ReleaseRefCountable( &file);
#endif
}

void JS4D::VFolderToObject(XBOX::VJSObject& ioObject, XBOX::VFolder *inFolder, ExceptionRef *outException)
{
#if USE_V8_ENGINE
	Persistent<Context>*	v8PersContext = V4DContext::GetPersistentContext(ioObject.fContext);
	v8::HandleScope handle_scope(ioObject.fContext);
	Context::Scope		context_scope(ioObject.fContext, *v8PersContext);
	JS4DFolderIterator* folder = (inFolder == NULL) ? NULL : new JS4DFolderIterator(inFolder);
	if (ioObject.fObject)
	{
		V4DContext::SetObjectPrivateData(ioObject.fContext, ioObject.fObject, folder);
		//VJSParms_initialize	parms(ioObject.fContext,ioObject.fObject);
		JS4D::ClassDefinition*	classDef = (JS4D::ClassDefinition*)VJSFolderIterator::Class();
		classDef->initialize(ioObject.fContext, ioObject.fObject);
	}
	else
	{
		VJSContext	vjsContext(ioObject.fContext);
		ioObject = VJSFolderIterator::CreateInstance(vjsContext,folder);
	}
	ReleaseRefCountable(&folder);
#else
	ioObject.fObject = NULL;
	JS4DFolderIterator* folder = (inFolder == NULL) ? NULL : new JS4DFolderIterator( inFolder);
	ioObject.fObject = (folder != NULL) ? JSObjectMake(ioObject.fContext, VJSFolderIterator::Class(), folder) : NULL;
	ReleaseRefCountable( &folder);
#endif
}


void JS4D::VFilePathToObjectAsFileOrFolder(XBOX::VJSObject& ioObject, const XBOX::VFilePath& inPath, ExceptionRef *outException)
{
	if (inPath.IsFolder())
	{
		VFolder *folder = new VFolder( inPath);
		VFolderToObject(ioObject, folder, outException);
		ReleaseRefCountable( &folder);
	}
	else if (inPath.IsFile())
	{
		VFile *file = new VFile( inPath);
		VFileToObject( ioObject, file, outException);
		ReleaseRefCountable( &file);
	}
	else
	{
#if USE_V8_ENGINE
		xbox_assert(false);
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioObject.fObject));
		ioObject.fObject = NULL;
#else
		ioObject.fObject = NULL;	// exception instead ?
#endif
	}
}


void JS4D::EvaluateScript( ContextRef inContext, const VString& inScript, VJSValue& outValue, ExceptionRef *outException)
{
#if USE_V8_ENGINE
	DebugMsg("JS4D::EvaluateScript called for script=<%S>\n",&inScript);
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );

#else
	JSStringRef jsScript = JS4D::VStringToString( inScript);
	
	outValue = VJSValue(inContext ,( (jsScript != NULL) ? JS4DEvaluateScript( inContext, jsScript, NULL /*thisObject*/, NULL /*jsUrl*/, 0 /*startingLineNumber*/, outException) : NULL) );
	
	JSStringRelease( jsScript);
#endif
}


bool JS4D::ValueIsUndefined( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue == NULL) || inValue->IsUndefined();
#else
	return (inValue == NULL) || JSValueIsUndefined( inContext, inValue);
#endif
}

#if USE_V8_ENGINE
VJSValue JS4D::MakeUndefined(ContextRef inContext)
{
	VJSValue	result(inContext);
	Handle<Primitive>	undefVal = v8::Undefined(inContext);
	if (undefVal.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(undefVal.val_);
		result.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
	return result;
}

VJSValue JS4D::MakeNull(ContextRef inContext)
{
	VJSValue	result(inContext);
	Handle<Primitive>	nullVal = v8::Null(inContext);
	if (nullVal.val_)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(nullVal.val_);
		result.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
	return result;
}
#else
VJSValue JS4D::MakeUndefined(ContextRef inContext)
{
	VJSValue	tmpVal(inContext);
	tmpVal.fValue = JSValueMakeUndefined(inContext);
	return tmpVal;
}
VJSValue JS4D::MakeNull(ContextRef inContext)
{
	VJSValue	tmpVal(inContext);
	tmpVal.fValue = JSValueMakeNull( inContext);
	return tmpVal;
}
#endif

bool JS4D::ValueIsNull( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue == NULL) || inValue->IsNull();
#else
	return (inValue == NULL) || JSValueIsNull( inContext, inValue);
#endif
}

bool JS4D::ValueIsBoolean( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue != NULL) && inValue->IsBoolean();
#else
	return (inValue != NULL) && JSValueIsBoolean( inContext, inValue);
#endif
}


bool JS4D::ValueIsNumber( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue != NULL) && inValue->IsNumber();
#else
	return (inValue != NULL) && JSValueIsNumber( inContext, inValue);
#endif
}


bool JS4D::ValueIsString( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue != NULL) && inValue->IsString();
#else
	return (inValue != NULL) && JSValueIsString( inContext, inValue);
#endif
}


bool JS4D::ValueIsObject( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	return (inValue != NULL) && inValue->IsObject();
#else
	return (inValue != NULL) && JSValueIsObject( inContext, inValue);
#endif
}


JS4D::ObjectRef JS4D::ValueToObject( ContextRef inContext, ValueRef inValue, ExceptionRef* outException)
{
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	return NULL;
#else
	return (inValue != NULL) ? JSValueToObject( inContext, inValue, outException) : NULL;
#endif
}

#if USE_V8_ENGINE
void JS4D::ObjectToValue(XBOX::VJSValue& ioValue, const XBOX::VJSObject inObject)
{
	if (ioValue.fValue)
	{
		V8::DisposeGlobal(reinterpret_cast<internal::Object**>(ioValue.fValue));
		ioValue.fValue = NULL;
	}
	if (inObject.fObject)
	{
		internal::Object** p = reinterpret_cast<internal::Object**>(inObject.fObject);
		ioValue.fValue = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
			reinterpret_cast<internal::Isolate*>(v8::Isolate::GetCurrent()),
			p));
	}
}
#else
JS4D::ValueRef JS4D::ObjectToValue( ContextRef inContext, ObjectRef inObject)
{
	return (inObject != NULL) ? inObject : JSValueMakeNull( inContext);
}
#endif

JS4D::ObjectRef JS4D::MakeObject( ContextRef inContext, ClassRef inClassRef, void* inPrivateData)
{
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	return NULL;
#else
	return JSObjectMake( inContext, inClassRef, inPrivateData);
#endif
}

#if USE_V8_ENGINE
bool JS4D::ValueIsObjectOfClass(ContextRef inContext, ValueRef inValue, ClassRef inClassRef)
{
	xbox_assert(v8::Isolate::GetCurrent() == inContext);
	V4DContext*				v8Ctx = (V4DContext*)inContext->GetData();
	Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
	HandleScope				handleScope(inContext);
	bool isSame = false;
	if (inValue && inValue->IsObject())
	{
		Local<Object>		locObj = inValue->ToObject();
		Local<String>		constName = locObj->GetConstructorName();
		String::Utf8Value	utf8Name(constName);
		isSame = !::strcmp(*utf8Name,((JS4D::ClassDefinition*)inClassRef)->className);
		if (!isSame)
		{
			DebugMsg("JS4D::ValueIsObjectOfClass '%s' != '%s'\n",((JS4D::ClassDefinition*)inClassRef)->className,*utf8Name);
		}
	}	
	return isSame;
}
#else
bool JS4D::ValueIsObjectOfClass( ContextRef inContext, ValueRef inValue, ClassRef inClassRef)
{
	return (inValue != NULL) && JSValueIsObjectOfClass( inContext, inValue, inClassRef);
}
#endif

#if USE_V8_ENGINE
void* JS4D::GetObjectPrivate(ContextRef inContext, ValueRef inObject)
{
	return V4DContext::GetObjectPrivateData(inContext,inObject);
}
void JS4D::SetObjectPrivate(ContextRef inContext, ValueRef inObject, void* inData)
{
	V4DContext::SetObjectPrivateData(inContext,inObject,inData);
}

#else
void *JS4D::GetObjectPrivate( ObjectRef inObject)
{
	return JSObjectGetPrivate( inObject);
}
#endif

size_t JS4D::StringRefToUTF8CString( StringRef inStringRef, void *outBuffer, size_t inBufferSize)
{
#if USE_V8_ENGINE
	return 0;
#else
	return JSStringGetUTF8CString( inStringRef, (char *) outBuffer, inBufferSize);
#endif
}


JS4D::EType JS4D::GetValueType( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE
	if (!inContext || !inValue)
	{
		return eTYPE_UNDEFINED;
	}
	if (inValue->IsUndefined())
	{
		return eTYPE_UNDEFINED;
	}
	if (inValue->IsNull())
	{
		return eTYPE_NULL;
	}
	if (inValue->IsBoolean())
	{
		return eTYPE_BOOLEAN;
	}
	if (inValue->IsNumber())
	{
		return eTYPE_NUMBER;
	}
	if (inValue->IsString())
	{
		return eTYPE_STRING;
	}
	if (inValue->IsObject())
	{
		return eTYPE_OBJECT;
	}
	return eTYPE_UNDEFINED;
#else
	assert_compile( eTYPE_UNDEFINED == (EType) kJSTypeUndefined);
	assert_compile( eTYPE_NULL		== (EType) kJSTypeNull);
	assert_compile( eTYPE_BOOLEAN	== (EType) kJSTypeBoolean);
    assert_compile( eTYPE_NUMBER	== (EType) kJSTypeNumber);
	assert_compile( eTYPE_STRING	== (EType) kJSTypeString);
	assert_compile( eTYPE_OBJECT	== (EType) kJSTypeObject);
	return (inContext == NULL || inValue == NULL) ? eTYPE_UNDEFINED : (EType) JSValueGetType( inContext, inValue);
#endif
}


bool JS4D::ValueIsInstanceOf( ContextRef inContext, ValueRef inValue, const VString& inConstructorName, ExceptionRef *outException)
{
#if USE_V8_ENGINE
	VStringConvertBuffer	bufferTmp(inConstructorName, VTC_UTF_8);
	if (inValue->IsObject())
	{
		xbox_assert(v8::Isolate::GetCurrent() == inContext);
		V4DContext*				v8Ctx = (V4DContext*)inContext->GetData();
		Persistent<Context>*	v8PersContext = v8Ctx->GetPersistentContext(inContext);
		HandleScope				handleScope(inContext);
		Local<Context>			locCtx = Handle<Context>::New(inContext,*v8PersContext);
		Context::Scope			context_scope(locCtx);
		Handle<Object> obj = inValue->ToObject();
		Local<String> constrName = obj->GetConstructorName();
		v8::String::Utf8Value  valueStr( constrName );
		if (!strcmp(bufferTmp.GetCPointer(), *valueStr))
		{
			return true;
		}
	}
	return false;
#else
	JSStringRef jsString = JS4D::VStringToString( inConstructorName);
	JSObjectRef globalObject = JSContextGetGlobalObject( inContext);
	JSObjectRef constructor = JSValueToObject( inContext, JSObjectGetProperty( inContext, globalObject, jsString, outException), outException);
	xbox_assert( constructor != NULL);
	JSStringRelease( jsString);

	return (constructor != NULL) && (inValue != NULL) && JSValueIsInstanceOfConstructor( inContext, inValue, constructor, outException);
#endif
}

#if USE_V8_ENGINE
bool JS4D::ObjectIsFunction( ContextRef inContext, ValueRef inObjectValue)
{
	return inObjectValue->IsFunction();
}
#else
bool JS4D::ObjectIsFunction( ContextRef inContext, ObjectRef inObjectRef)
{
	return (inObjectRef != NULL) && JSObjectIsFunction( inContext, inObjectRef);
}
#endif

JS4D::ObjectRef JS4D::MakeFunction( ContextRef inContext, const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, ExceptionRef *outException)
{
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	return NULL;
#else
	StringRef jsName = VStringToString( inName);
	StringRef jsBody = VStringToString( inBody);
	bool ok = (jsName != NULL) && (jsBody != NULL);

	StringRef jsUrl;
	VString url;
	if (inUrl != NULL)
	{
		inUrl->GetAbsoluteURL( url, true);
		jsUrl = VStringToString( url);
	}
	else
	{
		jsUrl = NULL;
	}


	size_t countNames = ( !ok || (inParamNames == NULL) || inParamNames->empty() ) ? 0 : inParamNames->size();
	StringRef *names = (countNames == 0) ? NULL : new StringRef[countNames];
	if (names != NULL)
	{
		size_t j = 0;
		for( VectorOfVString::const_iterator i = inParamNames->begin() ; i != inParamNames->end() ; ++i)
		{
			StringRef s = VStringToString( *i);
			if (s == NULL)
				ok = false;
			names[j++] = s;
			
		}
	}
	else
	{
		if (countNames != 0)
		{
			ok = false;
			countNames = 0;
		}
	}
	
	ObjectRef functionObject = NULL;
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
#else
	functionObject = ok ? JS4DObjectMakeFunction( inContext, jsName, countNames, names, jsBody, jsUrl, inStartingLineNumber, outException) : NULL;

	for( JSStringRef *i = names ; i != names + countNames ; ++i)
	{
		if (*i != NULL)
			JSStringRelease( *i);
	}
	
	if (jsName != NULL)
		JSStringRelease( jsName);

	if (jsBody != NULL)
		JSStringRelease( jsBody);

	if (jsUrl != NULL)
		JSStringRelease( jsUrl);
#endif	
	return functionObject;

#endif
}

JS4D::ValueRef JS4D::CallFunction (const ContextRef inContext, const ObjectRef inFunctionObject, const ObjectRef inThisObject, sLONG inNumberArguments, const ValueRef *inArguments, ExceptionRef *ioException)
{
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	return NULL;
#else
	return JS4DObjectCallAsFunction(inContext, inFunctionObject, inThisObject, inNumberArguments, inArguments, ioException);
#endif
}


void JS4D::ProtectValue( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE

#else
	if (inValue != NULL)
		JSValueProtect( inContext, inValue);
#endif
}


void JS4D::UnprotectValue( ContextRef inContext, ValueRef inValue)
{
#if USE_V8_ENGINE

#else
	if (inValue != NULL)
		JSValueUnprotect( inContext, inValue);
#endif
}


bool JS4D::ThrowVErrorForException( ContextRef inContext, ExceptionRef inException)
{
	bool errorThrown = true;
#if USE_V8_ENGINE
	if ( (inException != NULL) && !inException->IsUndefined())
	{
		for(;;)
			DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
	}
	errorThrown = false;
#else
	if ( (inException != NULL) && !JSValueIsUndefined( inContext, inException))
	{
		VTask::GetCurrent()->GetDebugContext().DisableUI();

		bool bHasBeenAborted = false;

		// first throw an error for javascript
		VString description;
		if (ValueToString( inContext, inException, description, /*outException*/ NULL))
		{
			if ( description. BeginsWith ( CVSTR ( "SyntaxError" ) ) && JSValueIsObject ( inContext, inException ) ) // Just to be super safe at the moment to avoid breaking everything
			{
				JSObjectRef				jsExceptionObject = JSValueToObject ( inContext, inException, NULL );
				if ( jsExceptionObject != NULL)
				{
					JSStringRef			jsLine = JSStringCreateWithUTF8CString ( "line" );
					if ( JSObjectHasProperty ( inContext, jsExceptionObject, jsLine ) )
					{
						JSValueRef		jsValLine = JSObjectGetProperty ( inContext, jsExceptionObject, jsLine, NULL );
						if ( jsValLine != NULL)
						{
							double nLine = JSValueToNumber( inContext, jsValLine, NULL );
							VString		vstrLine ( "Error on line " );
							vstrLine. AppendReal ( nLine );
							vstrLine. AppendCString ( ". " );
							description. Insert ( vstrLine, 1 );
						}
					}
					JSStringRelease ( jsLine );
				}
			}
			else if ( description. Find ( CVSTR ( "Error: Execution aborted by caller." ) ) > 0 && JSValueIsObject ( inContext, inException ) )
				bHasBeenAborted = true;

			if ( !bHasBeenAborted )
			{
				bool wasthrown = false;
				JSObjectRef				jsExceptionObject = JSValueToObject ( inContext, inException, NULL );
				if ( jsExceptionObject != NULL)
				{
					JSStringRef _error = JSStringCreateWithUTF8CString ( "error" );
					JSStringRef _message = JSStringCreateWithUTF8CString ( "message" );
					JSStringRef _componentSignature = JSStringCreateWithUTF8CString ( "componentSignature" );
					JSStringRef _errCode = JSStringCreateWithUTF8CString ( "errCode" );
					JSValueRef jserror = JSObjectGetProperty ( inContext, jsExceptionObject, _error, NULL );
					if (jserror != NULL)
					{
						if (JSValueIsObject(inContext, jserror) && JS4D::ValueIsInstanceOf( inContext, jserror, CVSTR( "Array"), NULL))
						{
							JSObjectRef arrayObject = JSValueToObject( inContext, jserror, NULL);
							JSStringRef _length = JSStringCreateWithUTF8CString( "length");
							JSValueRef result = (_length != NULL) ? JSObjectGetProperty( inContext, arrayObject, _length, NULL) : NULL;
							double r = (result != NULL) ? JSValueToNumber( inContext, result, NULL) : 0;
							size_t length = (size_t) r;
							if (_length != NULL)
								JSStringRelease( _length);

							wasthrown = true;
							for( size_t i = 0 ; (i < length) ; ++i)
							{
								JSValueRef elemValue = JSObjectGetPropertyAtIndex( inContext, arrayObject, i, NULL);
								if (elemValue != NULL)
								{
									JSObjectRef elemobj = JSValueToObject( inContext, elemValue, NULL);
									if (elemobj != NULL)
									{
										JSValueRef jsmessage = JSObjectGetProperty ( inContext, elemobj, _message, NULL );
										if (jsmessage != NULL)
										{
											VString mess;
											JS4D::ValueToString(inContext, jsmessage, mess, NULL);
											JSValueRef jserrcode = JSObjectGetProperty ( inContext, elemobj, _errCode, NULL );
											double r1 = (jserrcode != NULL) ? JSValueToNumber( inContext, jserrcode, NULL) : 0;
											JSValueRef jscompsig = JSObjectGetProperty ( inContext, elemobj, _componentSignature, NULL );
											VString compsig;
											if (jscompsig != NULL)
												JS4D::ValueToString(inContext, jscompsig, compsig, NULL);
											uLONG sigpart = 0;
											if (compsig.GetLength() > 3)
												sigpart = (compsig[0] << 24) + (compsig[1] << 16) + (compsig[2] << 8) + compsig[3];
											VError errcode = MAKE_VERROR(sigpart, (sLONG)r1);
											VErrorImported *err = new VErrorImported(errcode, mess);
											VTask::GetCurrent()->PushRetainedError( err);
										}
									}
								}
							}
						}
					}
					JSStringRelease(_error);
					JSStringRelease(_message);
					JSStringRelease(_componentSignature);
					JSStringRelease(_errCode);
				}

				if (!wasthrown)
				{
					JSStringRef _line = JSStringCreateWithUTF8CString ( "line" );
					JSStringRef _sourceURL = JSStringCreateWithUTF8CString ( "sourceURL" );
					JSStringRef _message = JSStringCreateWithUTF8CString ( "message" );

					JSValueRef jsmessage = JSObjectGetProperty ( inContext, jsExceptionObject, _message, NULL );
					if (jsmessage != NULL)
					{
						wasthrown = true;
						VString mess;
						JS4D::ValueToString(inContext, jsmessage, mess, NULL);
						VString linenum, url;
						JSValueRef jurl = JSObjectGetProperty ( inContext, jsExceptionObject, _sourceURL, NULL );
						JSValueRef jline = JSObjectGetProperty ( inContext, jsExceptionObject, _line, NULL );
						if (jline != NULL)
						{
							JS4D::ValueToString(inContext, jline, linenum, NULL);
							if (jurl != NULL)
								JS4D::ValueToString(inContext, jurl, url, NULL);
							vThrowError(VE_JVSC_EXCEPTION_AT_LINE, mess, linenum, url);
						}
						else
						{
							vThrowError(VE_JVSC_EXCEPTION, mess);
						}

					}

					JSStringRelease(_line);
					JSStringRelease(_sourceURL);
					JSStringRelease(_message);	
				}

				if (!wasthrown)
				{
					StThrowError<>	error( MAKE_VERROR( 'JS4D', 1));
					error->SetString( "error_description", description);
				}
			}
		}
		
		VTask::GetCurrent()->GetDebugContext().EnableUI();
		errorThrown = !bHasBeenAborted;
	}
	else
	{
		errorThrown = false;
	}
#endif
	return errorThrown;
}

#if USE_V8_ENGINE
bool JS4D::ConvertErrorContextToException( ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException)
{
	bool exceptionGenerated = false;

	if ( (inErrorContext != NULL) && !inErrorContext->IsEmpty() )//&& (outException != NULL) )
	{
		exceptionGenerated = true;
		VErrorBase *error = inErrorContext->GetLast();
		
		if ( (error != NULL) )//&& (*outException == NULL) )	// don't override existing exception
		{
			VJSContext	vjsContext(inContext);
			VJSObject	e(vjsContext);
			VString		description;
			
			OsType component = COMPONENT_FROM_VERROR( error->GetError());

			if (!VJSFileErrorClass::ConvertFileErrorToException( error, e))
			{
				for( VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin() ; description.IsEmpty() && (i != inErrorContext->GetErrorStack().rend()) ; ++i)
				{
					(*i)->GetErrorDescription( description);
				}
				/*if (description.Find("Unable to retrieve"))
				{
					int a = 1;
				}*/
				Handle<String>	excStr = v8::String::NewFromTwoByte(inContext, description.GetCPointer(), v8::String::kNormalString, description.GetLength());
				Handle<Value>	hdlExc = v8::Exception::Error(excStr);
				internal::Object** p = reinterpret_cast<internal::Object**>(hdlExc.val_);
				e.fObject = reinterpret_cast<v8::Value*>(V8::GlobalizeReference(
					reinterpret_cast<internal::Isolate*>(inContext),
					p));
			}
		
			// copy all error stack in 'messages' array
			{
				VJSContext vjsContext(inContext);
				VJSArray arr(vjsContext);
				VJSArray arrErrs(vjsContext);

				sLONG count = 0;
				for( VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin() ; (i != inErrorContext->GetErrorStack().rend()) ; ++i)
				{
					VJSObject errobj(vjsContext);
					errobj.MakeEmpty();
					(*i)->GetErrorDescription( description);
					/*if (description.Find("Unable to retrieve"))
					{
						int a = 1;
					}*/
					VError err = (*i)->GetError();
					errobj.SetProperty( CVSTR( "message"), description);
					sLONG errcode = ERRCODE_FROM_VERROR(err);
					OsType compsig = COMPONENT_FROM_VERROR(err);
					errobj.SetProperty( CVSTR( "errCode"),  errcode);
					VString compname;
					compname.FromOsType( compsig);
					errobj.SetProperty( CVSTR( "componentSignature"), compname);
					arrErrs.PushValue(errobj);
					VJSValue jsdesc(vjsContext);
					jsdesc.SetString(description);
					arr.PushValue(jsdesc);
					count++;
				}

				if (count > 0)
				{
					e.SetProperty("messages", arr, PropertyAttributeNone);
					e.SetProperty("error", arrErrs, PropertyAttributeNone);
				}
			}
			Handle<Value>	hdlVal = Handle<Value>::New(inContext, e.fObject);
			v8::ThrowException(hdlVal);
		}
	}
	else
	{
		exceptionGenerated = false;
	}

	return exceptionGenerated;
}

#else
bool JS4D::ConvertErrorContextToException(ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException)
{
	bool exceptionGenerated = false;

	if ((inErrorContext != NULL) && !inErrorContext->IsEmpty() && (outException != NULL))
	{
		exceptionGenerated = true;
		VErrorBase *error = inErrorContext->GetLast();

		if ((error != NULL) && (*outException == NULL))	// don't override existing exception
		{
			VJSContext	vjsContext(inContext);
			VJSObject	e(vjsContext);
			VString		description;

			OsType component = COMPONENT_FROM_VERROR(error->GetError());

			if (!VJSFileErrorClass::ConvertFileErrorToException(error, e))
			{
				for (VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin(); description.IsEmpty() && (i != inErrorContext->GetErrorStack().rend()); ++i)
				{
					(*i)->GetErrorDescription(description);
				}
				JSStringRef jsString = JS4D::VStringToString(description);
				JSValueRef argumentsErrorValues[] = { JSValueMakeString(inContext, jsString) };
				JSStringRelease(jsString);

				e = VJSObject(inContext, JSValueToObject(inContext, JSObjectMakeError(inContext, 1, argumentsErrorValues, NULL), NULL));
			}

			// copy all error stack in 'messages' array
			{
				VJSContext vjsContext(inContext);
				VJSArray arr(vjsContext);
				VJSArray arrErrs(vjsContext);

				sLONG count = 0;
				for (VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin(); (i != inErrorContext->GetErrorStack().rend()); ++i)
				{
					VJSObject errobj(vjsContext);
					errobj.MakeEmpty();
					(*i)->GetErrorDescription(description);
					VError err = (*i)->GetError();
					errobj.SetProperty(CVSTR("message"), description);
					sLONG errcode = ERRCODE_FROM_VERROR(err);
					OsType compsig = COMPONENT_FROM_VERROR(err);
					errobj.SetProperty(CVSTR("errCode"), errcode);
					VString compname;
					compname.FromOsType(compsig);
					errobj.SetProperty(CVSTR("componentSignature"), compname);
					arrErrs.PushValue(errobj);
					VJSValue jsdesc(vjsContext);
					jsdesc.SetString(description);
					arr.PushValue(jsdesc);
					count++;
				}

				if (count > 0)
				{
					e.SetProperty("messages", arr, PropertyAttributeNone);
					e.SetProperty("error", arrErrs, PropertyAttributeNone);
				}
			}

			*outException = e.GetObjectRef();
		}
	}
	else
	{
		exceptionGenerated = false;
	}

	return exceptionGenerated;
}
#endif


#if USE_V8_ENGINE
static std::vector<JS4D::StaticFunction>	gStaticFunctionsVect;
#endif
JS4D::ClassRef JS4D::ClassCreate( const ClassDefinition* inDefinition)
{
#if USE_V8_ENGINE
if (!inDefinition)
{
		/*for( int idx = 0; idx<gStaticFunctionsVect.size(); ++idx)
		{
			DebugMsg("JS4D::ClassCreate treating static function %s\n",gStaticFunctionsVect[idx].name);
		}*/
		return NULL;
}
#endif

	// copy our private structures to WK4D ones
	
#if USE_V8_ENGINE
	DebugMsg("JS4D::ClassCreate called for <%s>\n",inDefinition->className);
#else
	std::vector<JSStaticValue> staticValues;
#endif
	if (inDefinition->staticValues != NULL)
	{
		for( const StaticValue *i = inDefinition->staticValues ; i->name != NULL ; ++i)
		{
			//DebugMsg("JS4D::ClassCreate treat static value %s\n",i->name);
#if USE_V8_ENGINE
#else
			JSStaticValue value;
			value.name = i->name;
			value.getProperty = i->getProperty;
			value.setProperty = i->setProperty;
			value.attributes = i->attributes;
			staticValues.push_back( value);
#endif
		}
#if USE_V8_ENGINE
#else
		JSStaticValue value_empty;
		value_empty.name = NULL;
		value_empty.getProperty = NULL;
		value_empty.setProperty = NULL;
		value_empty.attributes = 0;
		staticValues.push_back( value_empty);
#endif
	}
	
#if USE_V8_ENGINE
#else
	std::vector<JSStaticFunction> staticFunctions;
#endif
	if (inDefinition->staticFunctions != NULL)
	{
		for( const StaticFunction *i = inDefinition->staticFunctions ; i->name != NULL ; ++i)
		{
			//DebugMsg("JS4D::ClassCreate treat static function %s for %s\n",i->name,inDefinition->className);
#if USE_V8_ENGINE
			/*if (inDefinition)
			{
				StaticFunction statFunc;
				statFunc.name = strdup(i->name);
				statFunc.callAsFunction = i->callAsFunction;
				statFunc.attributes = i->attributes;
			}
			else
			{

			}*/
#else
			JSStaticFunction function;
			function.name = i->name;
			function.callAsFunction = i->callAsFunction;
			function.attributes = i->attributes;
			staticFunctions.push_back( function);
#endif
		}
#if USE_V8_ENGINE
#else
		JSStaticFunction function_empty;
		function_empty.name = NULL;
		function_empty.callAsFunction = NULL;
		function_empty.attributes = 0;
		staticFunctions.push_back( function_empty);
#endif
	}
	
	
#if USE_V8_ENGINE
	DebugMsg("JS4D::ClassCreate JSClassDefinition for %s\n",inDefinition->className);
	ClassDefinition	classDef = *inDefinition;
	if (inDefinition->className)
	{
		classDef.className = strdup(inDefinition->className);
	}
	if (inDefinition->parentClass)
	{
		//classDef.parentClass = ...(inDefinition->parentClass);
	}
	JS4D::ClassDefinition*	newDef = new JS4D::ClassDefinition;
	*newDef = *inDefinition;
	V4DContext::AddNativeClass(newDef);
	return (JS4D::ClassRef)newDef;
#else
	JSClassDefinition def = kJSClassDefinitionEmpty;
//	def.version
	def.attributes			= inDefinition->attributes;
	def.className			= inDefinition->className;
	def.parentClass			= inDefinition->parentClass;
	def.staticValues		= staticValues.empty() ? NULL : &staticValues.at(0);
	def.staticFunctions		= staticFunctions.empty() ? NULL : &staticFunctions.at(0);
	def.initialize			= inDefinition->initialize;
	def.finalize			= inDefinition->finalize;
	def.hasProperty			= inDefinition->hasProperty;
	def.getProperty			= inDefinition->getProperty;
	def.setProperty			= inDefinition->setProperty;
	def.deleteProperty		= inDefinition->deleteProperty;
	def.getPropertyNames	= inDefinition->getPropertyNames;
	def.callAsFunction		= inDefinition->callAsFunction;
	def.callAsConstructor	= inDefinition->callAsConstructor;
	def.hasInstance			= inDefinition->hasInstance;
//	def.convertToType		= inDefinition->convertToType;
	return JSClassCreate( &def);
#endif

}




// -----------------------------------------------------------------------------------------------------



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
#ifndef __VJSJSON__
#define __VJSJSON__


#include "JS4D.h"
#include "VJSContext.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VJSJSON : public XBOX::VObject
{
public:
#if USE_V8_ENGINE
#else
	typedef void (*fn_callAsFunction)( VJSParms_callAsFunction&);
	template<fn_callAsFunction fn>
	static JS4D::ValueRef __cdecl js_callAsFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{

		VJSParms_callAsFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		fn( parms);
		return (parms.GetReturnValue() != NULL) ? parms.GetReturnValue() : JS4D::MakeNull( inContext);
	}
#endif
								// create a smart pointer to the global JSON object
			explicit			VJSJSON( const XBOX::VJSContext& inContext, XBOX::VJSException *outException = NULL);

		/*
			extract from ECMA-262:
		
			The parse function parses a JSON text (a JSON-formatted String) and produces an ECMAScript value.
			The JSON format is a restricted form of ECMAScript literal. JSON objects are realized as ECMAScript objects.
			JSON arrays are realized as ECMAScript arrays. JSON strings, numbers, booleans, and null are realized as ECMAScript Strings, Numbers, Booleans, and null.
			JSON uses a more limited set of white space characters than WhiteSpace and allows Unicode code points U+2028 and U+2029 to directly appear in JSONString literals without using an escape sequence.

			The optional reviver parameter is a function that takes two parameters, (key and value).
			It can filter and transform the results.
			It is called with each of the key/value pairs produced by the parse, and its return value is used instead of the original value.
			If it returns what it received, the structure is not modified. If it returns undefined then the property is deleted from the result.
		*/
			void				Parse( VJSValue& ioValue, const XBOX::VString& inJSON, XBOX::VJSException *outException = NULL);

		/*
			For conveniency, you should pass a function that accept a VJSParms_callAsFunction& as parameter as follows
		
			void myReviver( VJSParms_callAsFunction& inParms);
			VJSValue result = myJSON.Parse( someJSON, VJSJSON::js_callAsFunction<myReviver>);
		*/
			VJSValue			Parse( const XBOX::VString& inJSON, JS4D::ObjectCallAsFunctionCallback inReviverFunction, XBOX::VJSException *outException = NULL);
			
			

		/*
			extract from ECMA-262:

			The stringify function returns a String in JSON format representing an ECMAScript value.
			It can take three parameters.

			The first parameter is required.
			The value parameter is an ECMAScript value, which is usually an object or array, although it can also be a String, Boolean, Number or null.

			The optional replacer parameter is either a function that alters the way objects and arrays are stringified,
			or an array of Strings and Numbers that acts as a white list for selecting the object properties that will be stringified.

			The optional space parameter is a String or Number that allows the result to have white space injected into it to improve human readability.
		*/
			void				Stringify( const VJSValue& inValue, XBOX::VString& outJSON, XBOX::VJSException *outException = NULL);
			
		/*
			pass in inSpace the number of spaces you want for readibility.
		*/
			void				StringifyWithSpaces( const VJSValue& inValue, sLONG inSpaces, XBOX::VString& outJSON, JS4D::ExceptionRef *outException = NULL);
		
private:

#if USE_V8_ENGINE
#else
	static	void				_GetJSON(VJSJSON& ioJSON, JS4D::ExceptionRef *outException);
			void				_GetParseFunction( JS4D::ExceptionRef *outException);
			void				_GetStringifyFunction( JS4D::ExceptionRef *outException);
			void				_Parse(XBOX::VJSValue& ioValue,JS4D::StringRef inJSON, JS4D::ExceptionRef *outException);
			XBOX::VJSValue		_Parse(JS4D::StringRef inJSON, JS4D::ObjectRef inReviverFunction, JS4D::ExceptionRef *outException);

			XBOX::VJSObject		fJSONObject;
			XBOX::VJSObject		fParseFunction;
			XBOX::VJSObject		fStringifyFunction;
#endif
			void				_Stringify(const XBOX::VJSValue& inValue, const XBOX::VJSValue inReplacer, const XBOX::VJSValue inSpace, XBOX::VString& outJSON, JS4D::ExceptionRef *outException);
			
			XBOX::VJSContext	fContext;
};


END_TOOLBOX_NAMESPACE


#endif

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
#ifndef __VJSONTools__
#define __VJSONTools__

#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VValueBag.h"


BEGIN_TOOLBOX_NAMESPACE

class VJSONValue;

/**@brief	VJSONImporter contains two kind of routines:
				-> Low-level, to parse a JSON string as you want
				-> High-level, to fill an object from the JSON string
 
			Using VJSONImporter
				*	If you need to parse a JSON string to extract some values from it, then loop using GetNextJSONToken(), until you
					get what you want (or you get VJSONImporter::jsonNone). Most of the time, you'll need to handle the different
					tokens, jumping until the next and ignoring its value, or handling its value.
					
					Examples use can be found in VJSONArrayReader::VJSONArrayReader() or VJSONImporter::JSONObjectToBag()
 
				*	Some ready-to-use routines can also be used, check if they fullfill your needs. Right no, there is only JSONObjectToBag
 
			Please, do not add routines here unless you're sure you need them. For example, we could have added some routines such as
			GetPreviousJSONToken() or Rewind(), to help to navigate inside the string. Or some CounElements()/GetNthElement() accessors.
			Or we could have added a constructor thar retains the string. etc. etc.
				=> but at this time, no one needs such routines. So, we let the class as simple as possible.
 
*/
class XTOOLBOX_API VJSONImporter : public VObject
{
public:
		typedef enum {
			jsonNone = 0,
			jsonBeginObject,
			jsonEndObject,
			jsonBeginArray,
			jsonEndArray,
			jsonSeparator,
			jsonAssigne,
			jsonString,
			jsonDate
		} JsonToken;
	
		enum
		{
			EJSI_ReturnUndefinedWhenMalformed = 1,	// on maformed json always return json_undefined instead of what has been parsed so far
			EJSI_QuotesMandatoryForString = 2,	// strings must be surrounded by double quotes (required by standard grammar)
			EJSI_AllowDates = 64,
			EJSI_Default = 0,
			EJSI_Strict = EJSI_ReturnUndefinedWhenMalformed | EJSI_QuotesMandatoryForString
		};
		typedef uLONG	EJSONImporterOptions;
		
									VJSONImporter( const VString& inJSONString, EJSONImporterOptions inOptions = EJSI_Default);
	virtual							~VJSONImporter();
			
			/**@brief	GetNextJSONToken() parses the string until it reaches a token.
				It fills outString with the content that is between previous token -> this token
			*/
			JsonToken				GetNextJSONToken( VString& outString, bool* withQuotes, VError *outError = NULL);

			/**@brief	This GetNextJSONToken() is faster, and must be used only if you need to jump until a specific token*/
			JsonToken				GetNextJSONToken( VError *outError = NULL);

			// parse json string and produces a json value
			VError					Parse( VJSONValue& outValue);

			// Parse some string and produces a value.
	static	VError					ParseString( const VString& inString, VJSONValue& outValue, EJSONImporterOptions inOptions = EJSI_Default);

			// Parse a file contents as string.
			// default file encoding (if there's no bom) is utf-8
	static	VError					ParseFile( const VFile *inFile, VJSONValue& outValue, EJSONImporterOptions inOptions = EJSI_Default);
			
			/**@brief	JSONObjectToBag() fills outBag with the values contained in the string
			 
						WARNING: JSONObjectToBag will not work with any JSON string, because it expects the string:
								-> Starts with {, ends with } (it's a full JSON object)
								-> Contains names and values, and values are properly assignes to names
								-> For sub-objects, it creates a sub-bag that has a bool L"____objectunic" set to true
								-> . . . (see the code)
			 
			*/
			VError					JSONObjectToBag( VValueBag& outBag);
			
			void					SetSourceID( const VString& inSourceID)		{ fSourceID = inSourceID;}
			const VString&			GetSourceID() const							{ return fSourceID;}
	
private:
									VJSONImporter( const VJSONImporter&);	// forbidden
			VJSONImporter&			operator=( const VJSONImporter&);	// forbidden

	static	VString					_TokenToString( JsonToken inToken);

			UniChar					_GetNextChar(bool& outIsEOF);
			VString					_GetStartTokenFirstChar() const;
			VError					_StringToValue( const VString& inString, bool inWithQuotes, VJSONValue& outValue, const char *inExpectedString);
			VError					_ParseObject( VJSONValue& outValue);
			VError					_ParseArray( VJSONValue& outValue);
			bool					_ParseNumber( const UniChar *inString);
			VError					_ThrowErrorMalformed() const;
			VError					_ThrowErrorInvalidToken( const VString& inFoundToken, const char *inExpectedString) const;
			VError					_ThrowErrorUnterminated( const char *inExpectedString) const;
			VError					_ThrowErrorExtraComma( const char *inExpectedString) const;
			
			VString					fString;
			EJSONImporterOptions	fOptions;
			VIndex					fInputLen;
			const UniChar*			fCurChar;
			const UniChar*			fStartChar;
			const UniChar*			fStartToken;
			
			VString					fSourceID;	// for error reporting purpose. Might be an url or a file path of offending json.

			uLONG					fRecursiveCallCount;//<<< right now (2009-05-29), only used by JSONObjectToBag
	
};

/** @brief	VJSONArrayWriter creates a JSON array: ["string",123,"2008-12-10T00:00:00",3.14,true]
			No spaces, no human-more-easy-readable formating. For example, the php json_decode does'nt want carrage return,
			it only allows a space after the comma between each element.
			It's for quick access and creation, write only.
 
			Some examples:
			------------------------------------
			void ARoutine(XBOX::VString &outS)
			{
				XBOX::VJSONArrayWriter		jsonArray;

				jsonArray.AddString("coucou", false);// false because "coucou" has no forbidden chars
				jsonArray.AddBool(true);
				jsonArray.AddLong(123);
				
				jsonArray.GetArray(outS, true); // Close the array and return it
			}
			
			------------------------------------ Using a VString&
			void ARoutine(XBOX::VString &ioS)
			{
				XBOX::VJSONArrayWriter		jsonArray(ioS);

				jsonArray.AddString("coucou", false);
				jsonArray.AddBool(true);
				jsonArray.AddLong(123);
				jsonArray.Close(); // ioS is now a valid JSON array
			}
			
			------------------------------------ 
			void ARoutine()
			{
				XBOX::VJSONArrayWriter		jsonArray;

				jsonArray.AddString("coucou", false);
				jsonArray.AddBool(true);
				jsonArray.AddLong(123);
				jsonArray.Close();	// Close before using GetArray() with no parameters
				DoSomethingWithTheConstVString(jsonArray.GetArray());
			}
			
			
*/
class XTOOLBOX_API VJSONArrayWriter : public VObject
{
public:
							VJSONArrayWriter() : fArrayRef(fArray)										{ Open(); }
	/** @brief	This constructor avoids duplicating the content for big arrays for nano seconds trapping. Of course, caller
				is responsible of *not* modifying inSourceString until creating the array is done.
	*/
							VJSONArrayWriter(VString & inSourceString) : fArrayRef(inSourceString)		{ Open(); }
							~VJSONArrayWriter()		{}
	
	/** @name Open/Close an array */
	//@{
		void				Open()		{ fArrayRef = "["; fIsClosed = false; }
		void				Close();
		void				Clear()		{ fArrayRef = "[]"; fIsClosed = true; }
	//@}
	

	/** @name Append values */
	//@{
		void				Add(const VValueSingle &inAny, JSONOption inModifier = JSON_Default);
	/** @brief	Most of the time this AddString() will be used for hard coded values such as:
					jsonArray.AddString("Toto", JSON_AlreadyEscapedChars);
	*/
		void				AddString(const char *inCStr, JSONOption inModifier = JSON_Default);
	/** @brief	Appends the non quoted string "null" */
		void				AddNull();
	/** @brief WARNING: inArray is closed if needed*/
		void				AddJSONArray(VJSONArrayWriter &inArray);
	/** @brief if speed is really critical, those accessors don't use a VValueSingle */
		void				AddBool(bool inValue);
		void				AddLong(sLONG inValue);
		void				AddReal(Real inValue);
		void				AddString(const VString& inValue, JSONOption inModifier = JSON_Default);
	
	/** @name	Append several values, "one shot".
				If inCountOfElements == 0, does nothing
	*/
		void				AddLongs(const sLONG *inArray, sLONG inCountOfElements);
		void				AddWords(const sWORD *inArray, sLONG inCountOfElements);
		void				AddReals(const Real *inArray, sLONG inCountOfElements);
		void				AddBools(const char *inArray, sLONG inCountOfElements);
		void				AddStrings(const std::vector<VString> &inStrings, JSONOption inModifier = JSON_Default);
	//@}
	
	/** @name Accessors */
	//@{
	/** @brief The array is closed before being copied in outValue. If inAndClearIt == true, then Start() is called*/
		void				GetArray(VString &outValue, bool inAndClearIt);
	/** @brief The array is _not_ closed here (routine is const) => a call to Close() must have been done previousely*/
		const VString &		GetArray()		{ return fArrayRef; }
		bool				IsClosed() const	{ return fIsClosed; }
	//@}

private:
		inline void			_ReopenIfNeeded();
	
		VString				fArray;
		VString&			fArrayRef;
		bool				fIsClosed;
};

/** @brief	VJSONArrayReader 
			
			Read from a JSON string. VJSONArrayReader reads the string until it reachs the "open-array" tag, [. So, it's ok to pass an array
			encapsulated in a JSON object: "{[...]}".
 
			First implementation, 2009-05-26: routines expect a single-dimension/single type array
			-------------------------------------------------------------------------------------------
				For example: [1, 2, 3, 4, 5], or ["this", "is", "a", "valid\t", "stuff\r"]
 
				If the string sontains different types, say [1, true, "blah", "2008-12-10T00:00:00"], the converter just tries to convert each
				value, which can produce unexpected results (no crash, but bad values returned)
 
				If the array contains subarrays, they are just ignored:
						[1, 2, 3, [4, 5, 6], 7, 8] => fills just 1, 2, 3, 7, 8
			
*/
class XTOOLBOX_API VJSONArrayReader : public VObject
{
public:
							VJSONArrayReader();
							VJSONArrayReader(const VString& inJSONString);
							~VJSONArrayReader()		{ }
	
	/** @brief		ToArray/Short/Long/Real/Bool()
					
					First, call GetCountOfelements(), then allocate your array and call ToArray/...
	 */
	sLONG					GetCountOfElements(VError *outParsingError = NULL);
	VError					ToArrayShort(sWORD *outArray);
	VError					ToArrayLong(sLONG *outArray);
	VError					ToArrayReal(Real *outArray);
	VError					ToArrayBool(char *outArray);
	
	/** @brief		Templated accessor is slower than the typed accessors, for sure
					Works only with XBOX type: s/uLONG, s/uWORD, s/uLONG8, Real, s/uBYTE, s/uCHAR, and with bool and char
	*/
	/*
	template <typename ScalarType>
	VError					ToArray(ScalarType *outArray);
	*/
	
	/**@brief		Copy/append the values */
	VError					ToArrayString(VectorOfVString &outStrings, bool inDoAppend = false);
	/**@brief		For wuick access, when you want to avoid copying the vector, and just need to loop thru it for example*/
	const VectorOfVString&	GetStringsValues(VError *outParsingError = NULL) const
							{
								if(outParsingError != NULL)
									*outParsingError = fLastParseError;
								return fValues;
							}
	
	/** @brief		FromJSONString()
					Re-parse the string.
	*/
	void					FromJSONString(const VString& inJSONString);
	
	/**@brief		vector-like accessors*/
	 const VString&			operator[](VIndex inIndex) const
							{
								xbox_assert(inIndex >= 0 && inIndex < fVectorSize); 
								return fValues[inIndex];
							}
	
private:
	void					_Parse(const VString &inJSONString);
	
	VError					fLastParseError;
	VectorOfVString			fValues;
	sLONG					fVectorSize;//<<cached value, for fValues.size()
	
};



/** @brief	VJSONSingleObjectWriter
			
			Build a JSON object string which looks like to:
				{"member1":value1,"member2":value2,...,"memberN":valueN}
			where member is the name of the member.
			The value is for example a single value ("Hello world!"), an other object ({"company":"4D"}) or an array ([1,2,3,4,5]).
*/

class XTOOLBOX_API VJSONSingleObjectWriter : public VObject
{
public:
			VJSONSingleObjectWriter();
	virtual ~VJSONSingleObjectWriter();

			/**	@brief	Add a member to the JSON object */
			bool		AddMember( const VString& inName, const VValueSingle &inValue, JSONOption inModifier = JSON_Default);
			/** @brief	Returns the JSON object String */
			void		GetObject( VString& outObject);

private:
			VString		fObject;
			bool		fIsClosed;
			sLONG		fMembersCount;
};


END_TOOLBOX_NAMESPACE

#endif	// __VJSONTools__

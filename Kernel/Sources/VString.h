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
#ifndef __VString__
#define __VString__

#include "Kernel/Sources/VValueSingle.h"
#include "Kernel/Sources/VAssert.h"
#include "Kernel/Sources/VInterlocked.h"
#include "Kernel/Sources/VMemoryCpp.h"
#include "Kernel/Sources/VMemoryBuffer.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VFromUnicodeConverter;
class VToUnicodeConverter;
class VArrayString;
class VBlob;
class VString;
struct VInlineString;
class VCollator;

typedef std::vector<VString> VectorOfVString;

/*!
	@header	VString
	@abstract Strings management classes.
	@discussion
		VString can be viewed as a null-terminated UTF16 string buffer.
		It uses a private buffer which grows according to its needs.
		
		VStr<> is a template derived from VString that let you specify a
		private buffer size to optimize memory allocations.
		
		VOsTypeString is basically a VStr<4> convenient to use a MacOS 4-chars OSType
		as a VString.
		
		VStringConvertBuffer is a tool to ease charset conversions.
*/


// Length utility
XTOOLBOX_API VSize	UniCStringLength( const UniChar* inString);


class XTOOLBOX_API VString_info : public VValueInfo
{
public:
	VString_info( ValueKind inTrueKind = VK_STRING):VValueInfo( VK_STRING, inTrueKind)			{;}
	virtual	VValue*					Generate() const;
	virtual	VValue*					Generate(VBlob* inBlob) const;
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const;
	virtual void*					ByteSwapPtr( void *inBackStore, bool inFromNative) const;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;

	virtual CompareResult			CompareTwoPtrToData_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const;

	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
	virtual	VSize					GetAvgSpace() const;
};


/*!
	@class	VString
	@abstract	String management class.
	@discussion
		A VString is basically a pointer to a cpp memory block which contains
		an array of UTF16 characters. The last character is always 0x0000.
		
		Whenever you attempt to add some characters, VString may try to allocate a
		bigger buffer. This is the only occasion where the string buffer may be moved around.
		See the VStr<> template definition for specifying a pre-allocated buffer size.

		A VString is a VValue so it supports the IsDirty and the IsNull state flags.
		
		Conventions:
			- "length" is used to signify a number of characters.
			- "nbBytes" is used to signify a number of bytes.
			- the native types char & wchar_t are used in a parameter definition when it expects a hard-coded value.
		
*/

class XTOOLBOX_API VString : public VValueSingle
{ 
public:
	friend struct VInlineString;
	typedef VString_info	InfoType;
	static const InfoType	sInfo;
	
	/*!
		@function	VString
		@abstract	Creates an empty string
		if inNull is true, the VString is created NULL.
	*/
								VString();

		explicit				VString( bool inNull);
	
	/*!
		@function	VString
		@abstract	This is the copy constructor
		@discussion
			If you want to make a clone from a VString, uses instead VString::Clone
			which tries to allocate the cloned string in one single memory block.
		@param	inString The VString to copy.
	*/
								VString( const VString& inString);

	/*!
		@function	VString
		@abstract	Creates a string given a utf16 null-terminated string.
		@discussion
			example: VString str( L"coucou");
		@param	inUniCString The utf16 null-terminated string to copy.
	*/
								VString( const UniChar* inUniCString);
	
#if !WCHAR_IS_UNICHAR
								VString( const wchar_t* inUniCString);
#endif

	/*!
		@function	VString
		@abstract	Creates a string given a single utf16 character.
		@discussion
			example: VString str( CHAR_FULL_STOP);
		@param	inUniChar a utf16 character.
	*/
	explicit					VString( UniChar inUniChar);

	/*!
		@function	VString
		@abstract	Creates a string given a hard-coded C-string.
		@discussion
			example: VString str( "coucou");
			This should be used only for hard-coded strings since it assumes
			the character set is the one defined by the current C++ compiler.
		@param	inCString a hard-coded null-terminated string.
	*/
								VString( const char* inCString);

	/*!
		@function	VString
		@abstract	Creates a string given an arbitrary block of bytes.
		@discussion
			example: VString str( &spTemp[1], spTemp[0], VTC_StdLib_char); // from a pascal mac string
		@param	inBuffer address of the block of bytes.
		@param	inNbBytes number of bytes to take into account.
		@param	inCharSet the charset used in the given block of bytes.
			typically VTC_Win32Ansi, VTC_StdLib_char or VTC_UTF_16.
	*/
	explicit					VString( const void* inBuffer, VSize inNbBytes, CharSet inCharSet);

	/*!
		@function	VString
		@abstract	Creates a string referencing an existing utf16 null-terminated string.
		@discussion
			The VString will use the provided buffer instead of creating a new one.
			if inMaxLen is -1, it assumes the max len is the actual c-string len.

			examples:
			const VString str( L"coucou", -1);	// uses a hard-coded unicode CString as a VString without copy( Use CVSTR instead!).
			VString str( mybuffer, 80);	// creates a VString using a private 160 bytes buffer initially empty.

			note1: XToolBox defines UniChar as an unsigned short. This is generally the same as wchar_t.
			but with compilers which do not consider wchar_t and unsigned short as the same we need to define
			an alternate function.
			note2: the VString does not take ownership of the given string pointer. That means VString will not
			dispose this pointer( this lets you specify a hard-coded string pointer).
			note3: when using this constructor with compiler constant strings, care must be taken not to modify the VString chararacters
			since no copy is being performed, this would modify the global generated by the compiler. You should use the macro CVSTR for
			this purpose instead which ensures that resulting VString is declared const.
		@param	inUniCString address of the utf16 null-terminated string to reference.
		@param	inMaxBytes size in bytes of the given buffer. Must be at least large enough to contain the given
			string( including the null char). Extra space will be used whenever one tries to grow the VString.
			You can pass -1 which has the same effect as passing( UniCStringLength( inUniCString) + 1) * sizeof(UniChar) .
	*/
	explicit					VString( UniChar* inUniCString, VSize inMaxBytes);
	
#if !WCHAR_IS_UNICHAR
	explicit					VString( wchar_t* inUniCString, VSize inMaxBytes);
#endif

	/*!
		@function	VString
		@abstract	Creates a string referencing an existing utf16 null-terminated buffer string( with buffer size).
		@discussion
			The VString will use the provided buffer instead of creating a new one.
			This is a variation of VString::VString( UniChar* inUniCString, VSize inMaxBytes)
			which lets you specify the length of the specified string so that the function
			does not have to compute it by looking for the null character.
			See VString::VString(UniChar* inUniCString, VSize inMaxBytes) for more details.
		@param	inUniCString address of the utf16 null-terminated string to reference.
		@param	inLength number of characters of the given string.
		@param	inMaxBytes size in bytes of the given buffer. Must be at least large enough to contain the given
			string( including the null char). Extra space will be used whenever one tries to grow the VString.
			You can pass -1 which has the same effect as passing( inLength + 1) * sizeof(UniChar) .
	*/
	explicit					VString( UniChar* inUniCString, VIndex inLength, VSize inMaxBytes);
	
	/*
		@function	VString
		@abstract	Creates a new empty VString providing an inlined string.
	*/
	explicit					VString( const VInlineString& inString);

	/*!
		@function	~VString
		@abstract	Destructor.
	*/
	virtual						~VString();

	/*!
		@function	NewString
		@abstract	Creates a new empty VString specifying a number of chars to allocate the internal buffer.
		@discussion
			Allows to dynamically choose the size of the internal buffer so that it fits in the same cpp block as the VString object.
			You may prefer the VStr<> template if you know the size at compile time.
			example: VString *str = VString::NewString( 120); // allocates a 120 chars string in one contiguous block.
			Warning: such a string won't benefit from reference counting.
			
		@param	inNbChars number of chars used to allocate the buffer.
	*/
	static	VString*			NewString( VIndex inNbChars);

	/*!
		@function	DebugDump
		@abstract	dump the contents of the VString for debugging purpose.
		@discussion
			Inherited from VObject.
			Should not be used directly, call instead VObject::DebugDump( VString& outDump)
		@param	inTextBuffer buffer into which to dump the VString.
		@param	inBufferSize in: size of the buffer, out: number of bytes dumped.
		@result	the charset of what has been dumped into the buffer.
	*/
	virtual	CharSet				DebugDump( char* inTextBuffer, sLONG& inBufferSize) const;

	/*!
		@function	Clear
		@abstract	empties the string.
		@discussion
			VString::Clear() shrinks the buffer to its minimal size and sets the IsNull flag to false (as any other VValue).
			If you want to nullified the VString call SetNull( true).
	*/
	virtual	void				Clear();

	/*!
	 @function 	SubStrings 	 
	 @abstract 	Truncate a string to a sub-string
	 
	 @discussion 
			Description :	This method takes the all the chars from inFirst char through inNbChars,
							move them in the begining of the string( at index 1) and then truncate
							the string.
							The string is altered by the call of this method.
							
							Note : If you don't want to alter the original string in anyway,
							you may prefer to use the GetSubString() function.
						
	 
	 @param	inFirst		The offset where the sub-string starts
	 @param	inNbChars	The number of chars to go through
	*/
			void				SubString( VIndex inFirst, VIndex inNbChars);
	
	/*!
	 @function 	GetSubString 	 
	 @abstract 	Getting a sub-string
	 
	 @discussion 
			Description :	This method fills the outResult with a subset of the original string
							starting at inFirst char and going to inFirst+inNbChars.
							This method preserves the original string.
							
							Note : If you want to shrink the original string to a sub-string, you
							may prefer to use the SubString() function.
	 
	 @param	inFirst		The offset where the sub-string starts
	 @param	inNbChars	The number of chars to go through
	 @param	outResult	The resulting sub-string
	*/
			void				GetSubString( VIndex inFirst, VIndex inNbChars, VString& outResult) const;
	
	/*!
	 @function 	GetSubStrings 	 
	 @abstract 	Getting all the sub-strings separated by a certain char. See also Split()
	 
	 @discussion 
			Description :	This method fills outStrings( VArrayString) with all 
							the sub-strings separated by the inSeparatorChar.
							This function does not alter the original string.
	 
	 @param	inSeparatorChar		The UniChar separator( used to split the string)
	 @param	outStrings			The VArrayString containing the elements of the string 
	 @result	bool			The function returns false if the inSeparatorChar was not found in the string.
	*/
			bool				GetSubStrings( UniChar inDivider, VectorOfVString& outResultItems, bool inReturnEmptyStrings = false, bool inTrimSpaces = false) const;
			bool				GetSubStrings( UniChar inDivider, VArrayString& outResultItems, bool inReturnEmptyStrings = false, bool inTrimSpaces = false) const;
	
			/** 
			* @brief Return the table of strings separated by the given divider. The function returns false if the inSeparatorChar was not found in the string.
			* @result No element if the string was composed only by dividers. One element with the entire string if the divider was not found 
			*/
			bool				GetSubStrings( const VString& inDivider, VectorOfVString& outResultItems, bool inReturnEmptyStrings = false, bool inTrimSpaces = false) const;


			// joins all elements of an array into a string. The elements will be separated by a specified separator.
			bool				Join( const VectorOfVString& inStrings, UniChar inSeparator);
			
	/*!
		@function ConvertCarriageReturns
		@abstract Converts any carriage return sequence to given mode.
		@param inNewMode new carriage return mode
		@discussion
			ConvertCarriageReturns replaces any sequence like CR, CRLF or LF by the one specified by inNewMode.
	*/
			void				ConvertCarriageReturns( ECarriageReturnMode inNewMode);

	/*!
		@function GetCarriageReturnMode
		@abstract Analyze carriage return usage in the string.
		@discussion
			GetCarriageReturnMode analyze the string and tells wich line ending is used. If the strings has no line ending, returns the value passed as parameter.
	*/
			ECarriageReturnMode	GetCarriageReturnMode( ECarriageReturnMode inDefault = eCRM_NATIVE );

	/*!
		@function EnsureSize
		@abstract 
		@param inMaxNbChars
		@discussion
			Tries to resize internal buffer to hold inNbChars characters, not including the null char.
			if the result is true, you can safely use GetCPointerForWrite() to directly copy your bytes into the VString internal buffer.
			Don't forget to call VString::Validate afterwards to properly set internal flags and the null char
	*/
			bool				EnsureSize( VIndex inMaxNbChars)				{ return _PrepareWriteKeepContent( inMaxNbChars);}

	/*
		@function TrimeSpaces
		@abstract Removes leading and ending spaces 0x20
	*/
			void				TrimeSpaces();
			
	/*
		@function RemoveWhiteSpaces
		@abstract Removes leading and ending spaces 0x20 & 0x09
		@param inIntlMgr
			if inIntlMgr is not NULL, remove spaces according to inIntlMgr->IsSpace (all spaces like white-space, tab, carriage return & line feed are concerned)
			useful for instance to trim all unicode spaces in XML text before parsing XML
		@param inExcludingNoBreakSpace
			if true, exclude no-break spaces (only if inIntlMgr is not NULL and with ICU) - necessary to avoid to consolidate no-break spaces while parsing XHTML
	*/
			void				RemoveWhiteSpaces(	bool inLeadings = true, bool inTrailings = true, VIndex* outLeadingsRemoved = NULL, VIndex* outTrailingsRemoved = NULL, 
													VIntlMgr *inIntlMgr = NULL, bool inExcludingNoBreakSpace = false);
		
	/*
		@function GetEnsuredSize
		@abstract returns the max number of characters the VString can receive without having to reallocate its buffer.
		@discussion
			This is the max value you can pass to EnsureSize() without inducing reallocation.
	*/
			VIndex				GetEnsuredSize() const							{ return _IsShared() ? 0 : fMaxLength;}
	
			/**
			*	@brief Call this function after having modified internal string buffer so that the VString can properly adjust internal variables.
			*		it sets the length property to given value and appends the null char.
			*		returns false on error( such as a given length bigger than internal buffer size)
			*	@param inNbChars	
			*/
			bool				Validate( VIndex inNbChars);
	
	
			/**
			*	@brief By default, VString are only limited in size by available memory.	
			*		if you want a VString have a max length, call this function.
			*		attention: unless for MaxLongInt, SetMaxLength preallocates the buffer to the given max length.
			*		returns false if the buffer could not be allocated for given max length.
			*	@param inNbChars	
			*/
			bool				SetMaxLength( VIndex inNbChars = kMAX_sLONG);

	
			/**
			*	@brief Gets this VString max length( MaxLongInt by default)
			*	@param 
			*	@discussion
			*/
			VIndex				GetMaxLength() const;

			// Accessors
			VIndex				GetLength() const							{ return fLength; }
			bool				IsEmpty() const								{ return (fLength == 0) || IsNull(); }
			void				SetEmpty()									{ Clear(); }
			
			const UniChar*		GetCPointer() const							{ return fString; }	// Always succeeds( never NULL)
			UniChar*			GetCPointerForWrite()						{ return _PrepareWriteKeepContent( fLength) ? fString : NULL; }	// MAY FAIL
			UniChar*			GetCPointerForWrite( VIndex inMaxNbChars)	{ return _PrepareWriteKeepContent( inMaxNbChars) ? fString : NULL; }	// MAY FAIL

			/** Content manipulation */

			// @brief Returns 0 if inIndex is out of bounds
			UniChar				GetUniChar( VIndex inIndex) const;
			
			// @brief VString must not be NULL, and inIndex must be valid. Returns false if failed.
			// Remember that because of reference counting, SetUniChar may induce a memory allocation that can fail.
			bool				SetUniChar( VIndex inIndex, UniChar inChar);

			// @brief Truncate() does not shrink the buffer and does not change the null flag.
			void				Truncate( VIndex inNbChars);

			void				Insert( UniChar inCharToInsert, VIndex inPlaceToInsert);
			void				Insert( const VString& inStringToInsert, VIndex inPlaceToInsert);

			void				Replace( const VString& inStringToInsert, VIndex inPlaceToInsert, VIndex inLenToReplace);
			
			/**
				@brief Exchange inStringToFind with inStringToInsert inCountToReplace times.
				if inCollator is NULL, use the one from default intl manager.
			**/
			void				Exchange( const VString& inStringToFind, const VString& inStringToInsert, VIndex inPlaceToStart = 1, VIndex inCountToReplace = 1, bool inDiacritical = false, VCollator *inCollator = NULL);
			void				ExchangeAll( const VString& inStringToFind, const VString& inStringToInsert, bool inDiacritical = false, VCollator *inCollator = NULL)	{ Exchange( inStringToFind, inStringToInsert, 1, XBOX::kMAX_VIndex, inDiacritical, inCollator);}

			void				Exchange( UniChar inCharToFind, UniChar inNewChar, VIndex inPlaceToStart = 1, VIndex inCountToReplace = 1);
			void				ExchangeAll( UniChar inCharToFind, UniChar inNewChar)	{ Exchange( inCharToFind, inNewChar, 1, XBOX::kMAX_VIndex);}

			void				Remove( VIndex inPlaceToRemove, VIndex inLenToRemove);
			void				Swap( VString& inOther);

			// template specialization for STL.
			// Sur vstudio 2002 std::sort appelle std::swap et donc notre swap n'est pas une specialization mais un overload ce qui est interdit.
			// pour faire marcher il faudrait injecter swap<vstring> dans le namespace std mais c'est aussi interdit...
			// Il parait que vstudio 2003 fixe le pb.
			// sur cw, std::sort appelle swap et non std::swap donc pas de soucis.
	friend	void				swap( VString& x, VString& y)	{x.Swap( y);}

			VIndex				Find( const VString& inStringToFind, VIndex inPlaceToStart = 1, bool inDiacritical = false) const;
			VIndex				FindUniChar( UniChar inChar, VIndex inPlaceToStart = 1, bool inIsReverseOrder = false) const;
			
			void				ToUpperCase( bool inStripDiacritics = true);
			void				ToLowerCase( bool inStripDiacritics = true);

			/** @brief compose or decompose unicode characters
				WARNING: most of the xtoolbox assumes characters are in composed mode.
			*/
			void				Decompose();
			void				Compose();
			
			/**
			* @brief EqualToString returns true if both strings are to be considered equal using current task VIntlMgr.
			* pass inDiacritical = false if 'a' should be equal to 'A'
			*/
			bool				EqualToString( const VString& inString, bool inDiacritical = false) const;
			bool				EqualToString_Like( const VString& inPattern, bool inDiacritical = false) const;

			/**
			* @brief EqualToStringRaw returns true if both strings are bitwise equal (using memcmp, don't check IsNull).
			*/
			bool				EqualToStringRaw( const VString& inString) const;
			sLONG				FindRawString( const VString& inString) const	{ return FindRawString( GetCPointer(), GetLength(), inString.GetCPointer(), inString.GetLength());}
	static	sLONG				FindRawString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize);
			void				ExchangeRawString( const VString& inStringToFind, const VString& inStringToInsert, VIndex inPlaceToStart = 1, VIndex inCountToReplace = 1);

			/**
			* @brief CompareToString compares two string using current task VIntlMgr.
			* pass inDiacritical = false if 'a' should be equal to 'A'
			*/
			CompareResult		CompareToString( const VString& inValue, bool inDiacritical = false) const;
			CompareResult		CompareToString_Like( const VString& inPattern, bool inDiacritical = false) const;
			
			/**
				EqualToUSASCIICString is for raw and fast testing for equality with a US-ASCII C string.
				It compares the 7-bit ascii code with the low byte of each UniChar.
				If the passed CString contains something else than 7-bit US-Ascii, the behavior is undefined.
				It's especially usefull for xml key comparisons.
			**/
			bool				EqualToUSASCIICString( const char *inUSASCIICString) const;
			
			bool				BeginsWith( const VString& inValue, bool inDiacritical = false) const;
			bool				EndsWith( const VString& inValue, bool inDiacritical = false) const;

			// Appending data( you can write str.Append(2).Append(" coucou ").Append(oneValue))
			VString&			AppendBlock( const void* inBuffer, VSize inCountBytes, CharSet inSet);
			VString&			AppendString( const VString& inString)						{ return AppendUniChars( inString.GetCPointer(), inString.GetLength()); }
			VString&			AppendChar( char inChar)									{ return AppendBlock(&inChar, 1, VTC_StdLib_char); }
			VString&			AppendCString( const char* inCString)						{ return AppendBlock(inCString,( VSize) ::strlen( inCString), VTC_StdLib_char); }
			VString&			AppendUniCString( const UniChar* inUniCString)				{ return AppendUniChars( inUniCString, UniCStringLength(inUniCString)); }
			VString&			AppendUniChar( UniChar inChar);
			VString&			AppendUniChars( const UniChar *inCharacters, VIndex inCountCharacters);
			VString&			AppendLong( sLONG inValue);
			VString&			AppendLong8( sLONG8 inValue);
			VString&			AppendULong8( uLONG8 inValue);
			VString&			AppendReal( Real inValue);
			VString&			AppendOsType( OsType inType);
			VString&			AppendValue( const VValueSingle& inValue);
			VString&			AppendPrintf( const char* inMessage, ...);	// Append a formatted string( like printf)

#if !WCHAR_IS_UNICHAR
			VString&			AppendUniCString( const wchar_t* inUniCString)				{ return AppendBlock((UniChar*) inUniCString,( VSize) ::wcslen(inUniCString) * sizeof(wchar_t), VTC_WCHAR); }
#endif

			/**
			* @brief Format the string with the elements of the VValueBag
			* Try to replace every item name surrounded by curly brackets that matches the element name of the VValueBag by its value.
			*/
			VString&			Format( const VValueBag *inParameters, bool inClearUnresolvedReferences = true);

			/**
			* @brief TranscodeToXML replaces all invalid xml characters by their entities.
			* &amp; &quot; &gt; &lt; &apos;
			*/
			void				TranscodeToXML(bool inEscapeControlChars = false);

			/**
			* @brief TranscodeFromXML replaces xml entities with their corresponding character.
			* &amp; &quot; &gt; &lt; &apos;
			*/
			void				TranscodeFromXML();
			
			// Converts this string into Base64 representation using specified charset (utf-8 by default). Conforms RFC2045
			// return true on success.
			bool				EncodeBase64( CharSet inSet = VTC_UTF_8);
			
			// Converts specified data into Base64 representation. Conforms RFC2045
			// return true on success.
			bool				EncodeBase64( const void *inDataPtr, size_t inDataSize);
			
			// Interprets this string as a Base64 encoded stream using specified charset (utf-8 by default). Conforms RFC2045
			// All whitespaces (tab, cr, lf, space) are silently stripped during decoding.
			// return true on success.
			bool				DecodeBase64( CharSet inSet = VTC_UTF_8);

			// Interprets this string as a Base64 encoded stream of bytes and return them.
			// return true on success.
			bool				DecodeBase64( VMemoryBuffer<>& outBuffer) const;

	// Content accessors( with convertion)
	virtual	void				GetValue( VValueSingle& inValue) const;
			
			// recognize minus '-' sign and digit characters.
			// stop analyse when current intl decimal separator is encountered.
			// ignores all other characters.
			// returns 0 in case of overflow.
			// exemple: "$54.42" gives 54
	virtual	sLONG				GetLong() const;
	virtual	sLONG8				GetLong8() const;
	virtual	sWORD				GetWord() const;
	
			// Always use '.' as decimal separator
	virtual	Real				GetReal() const;

	virtual	void				GetFloat( VFloat& outFloat) const;
	virtual	Boolean				GetBoolean() const;	// true if ToLong doesn't return 0
	virtual	void				GetString( VString& outString) const;	// Simple copy
	virtual	void				GetVUUID( VUUID& outValue) const;
	virtual	void				GetTime( VTime& outTime) const;	// Assumes format "YYYY-MM-DD HH:MM:SS:MS"
	virtual	void				GetDuration( VDuration& outDuration) const;	// Assumes 'DDDD:HH:MM:SS:MS'
	
	virtual uLONG				GetHashValue() const;

			OsType				GetOsType() const;	// Assumes the string is 4 char long

			// Reads as hexadeciomal notation using chars 0->9, a->z, A->Z
			uLONG8				GetHexLong() const;

			// if inPrettyFormatting is true, prefix with "0x" and enough '0' for a multiple of 4 or 8 digits
			void				FromHexLong( sLONG inValue, bool inPrettyFormatting = false);
			void				FromHexLong( uLONG inValue, bool inPrettyFormatting = false);
			void				FromHexLong( sLONG8 inValue, bool inPrettyFormatting = false);
			void				FromHexLong( uLONG8 inValue, bool inPrettyFormatting = false);
			void				FromHexLong( sBYTE inValue, bool inPrettyFormatting = false);
			void				FromHexLong( uBYTE inValue, bool inPrettyFormatting = false);

	virtual	void				FromValue( const VValueSingle& inValue);
	virtual	Boolean				FromValueSameKind( const VValue *inValue);	// inValue MUST be a VString: take care !
	virtual void				FromByte( sBYTE inValue );	// Sign aware
	virtual void				FromWord( sWORD inValue );	// Sign aware
	virtual	void				FromLong( sLONG inValue);	// Sign aware
	virtual	void				FromLong8( sLONG8 inValue);	// Sign aware
	virtual	void				FromULong8( uLONG8 inValue);
	virtual	void				FromFloat( const VFloat& inFloat);	// Sign aware
	virtual	void				FromReal( Real inValue);	// Uses current system settings for the delimiter
	virtual	void				FromBoolean( Boolean inValue);	// Produces "1" for true and "0" for false
	virtual	void				FromString( const VString& inString);
			void				FromString( const VInlineString& inString);
			void				FromUniChar( UniChar inChar);
	virtual	void				FromVUUID( const VUUID& inValue);
	virtual	void				FromTime( const VTime& inTime);	// Assumes "YYYY-MM-DD HH:MM:SS:MS"
	virtual	void				FromDuration( const VDuration& inDuration);	// Assumes "DDDD:HH:MM:SS:MS"

	virtual	void				GetXMLString( VString& outString, XMLStringOptions inOptions) const;
	virtual	bool				FromXMLString( const VString& inString);

			void				FromOsType( OsType inType);

			// The charset is deduced from the leading BOM if any.
			// If no BOM has been found, inDefaultSet is used.
			void				FromBlockWithOptionalBOM( const void* inBuffer, VSize inCountBytes, CharSet inDefaultSet);

			void				FromBlock( const void* inBuffer, VSize inCountBytes, CharSet inSet);
			VSize				ToBlock( void* inBuffer, VSize inBufferSize, CharSet inSet, bool inWithTrailingZero, bool inWithLengthPrefix) const;	// Returns number of bytes written
			
			void				ToUniCString( UniChar* outString, VSize inMaxBytes) const	{ ToBlock(outString, inMaxBytes, VTC_UTF_16, true, false); }	// inMaxBytes must includes the zero char( 2 bytes)
			void				FromUniCString( const UniChar* inString)					{ FromBlock(inString, UniCStringLength(inString) * sizeof(UniChar), VTC_UTF_16); }

			void				ToCString( char* outString, VSize inMaxBytes) const			{ ToBlock(outString, inMaxBytes, VTC_StdLib_char, true, false); }	// inMaxBytes must includes the zero char
			void				FromCString( const char* inString)							{ FromBlock(inString,( VSize) ::strlen(inString), VTC_StdLib_char); }
			
#if !WCHAR_IS_UNICHAR
			void				ToUniCString( wchar_t* outString, VSize inMaxBytes) const	{ ToBlock(outString, inMaxBytes, VTC_WCHAR, true, false); }
			void				FromUniCString( const wchar_t* inString)					{ FromBlock(inString,( VSize) ::wcslen(inString) * sizeof(wchar_t), VTC_WCHAR); }
#endif

#if VERSIONMAC
			void				MAC_ToMacPString( uBYTE *outString, VSize inMaxBytes) const	{ ToBlock(outString, inMaxBytes, VTC_MacOSAnsi, false, true); }
			void				MAC_FromMacPString( const uBYTE *inString)					{ FromBlock(&inString[1], inString[0], VTC_MacOSAnsi); }
			void				MAC_FromCFString( CFStringRef inString);

			CFStringRef			MAC_RetainCFStringCopy() const;
			CFStringEncoding	MAC_GetCFStringEncoding() const								{ return kCFStringEncodingUnicode; }
			TextEncoding		MAC_GetTextEncoding() const									{ return kUnicodeUTF16Format; }
#endif

#if VERSIONWIN
			void				WIN_ToWinCString( sBYTE *outString, VSize inMaxBytes) const	{ ToBlock(outString, inMaxBytes, VTC_Win32Ansi, true, false); }
			void				WIN_FromWinCString( const sBYTE* inString)					{ FromBlock(inString,( VSize) ::strlen( inString), VTC_Win32Ansi); }
#endif

	// Length utilities
	static	VIndex				UniCStringLength( const UniChar* inString);
	static	VSize				CStringLength( ConstCString inString)						{ return ::strlen(inString); }
	static	VSize 				PStringLength( ConstPString inString)						{ return inString[0]; }
	static	void				PStringCopy( ConstPString inString, PString outDest, VSize inDestSize);
	static	void				CStringCopy( ConstCString inString, CString outDest, VSize inDestSize)	{ ::strncpy(outDest, inString, inDestSize); }
	static	void				PStringToCString( ConstPString inString, CString outDest, VSize inDestSize);
	static	void				CStringToPString( ConstCString inString, PString outDest, VSize inDestSize);
	
	// Inherited from VValue
	virtual	const VValueInfo*	GetValueInfo() const;

	virtual	VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual	VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;
			VSize				GetSizeInStream() const;
	
	virtual	void*				LoadFromPtr( const void* inData, Boolean inRefOnly = false);
	virtual	void*				WriteToPtr( void* ioData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual	VSize				GetSpace(VSize inMax = 0) const;
	
	virtual	CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritical = false) const;
	virtual	CompareResult		CompareToSameKind( const VValue *inValue, Boolean inDiacritical = false) const;
	virtual	CompareResult		CompareBeginingToSameKind( const VValue *inValue, Boolean inDiacritical =false) const;
	virtual	Boolean				EqualToSameKind( const VValue *inValue, Boolean inDiacritical = false) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual	CompareResult		Swap_CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual	CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult		Swap_CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult		CompareBeginingToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult		IsTheBeginingToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult			CompareToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareBeginingToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	Boolean					EqualToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;

	virtual	CompareResult			CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult			Swap_CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			IsTheBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean					EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean					Swap_EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual VError					FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError					GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual	VError					FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError					GetJSONValue( VJSONValue& outJSONValue) const;

	virtual	VString*				Clone() const;

			// Allocate operators
			void*				operator new( size_t inSize)					{ return VValue::operator new( inSize); }
			void				operator delete( void* inAddr)					{ VValue::operator delete( inAddr); }

			void*				operator new( size_t /*inSize*/, void* inAddr)	{ return inAddr; }
			void				operator delete( void* /*inAddr*/, void* /*inAddr*/) {}

			// Operators( comparison operators are not diacritical)
			const UniChar&		operator[]( VIndex inIndex) const				{ xbox_assert( inIndex >= 0 && inIndex < fLength); return fString[inIndex]; }
			
			bool				operator ==( const VString& inString) const		{ return EqualToString(inString, false) ? true : false; }
			bool				operator ==( const UniChar* inUniCString) const	{ return EqualToString(VString( const_cast<UniChar*>( inUniCString), (VSize) -1), false) ? true : false; }

		#if !WCHAR_IS_UNICHAR
			bool 				operator ==( const wchar_t* inUniCString) const	{ return EqualToString(VString( const_cast<wchar_t*>( inUniCString), (VSize) -1), false) ? true : false; }
		#endif

			bool				operator !=( const VString &inString) const		{ return !EqualToString(inString, false); }
			bool				operator >( const VString &inString) const		{ return CompareToString(inString, false) == CR_BIGGER; }
			bool				operator <( const VString &inString) const		{ return CompareToString(inString, false) == CR_SMALLER; }
			bool				operator >=( const VString &inString) const		{ return CompareToString(inString, false) >= CR_EQUAL; }
			bool				operator <=( const VString &inString) const		{ return CompareToString(inString, false) <= CR_EQUAL; }
			
			VString&			operator =( const VString& inString)			{ FromString( inString); return *this; }
			VString&			operator +=( const VString& inString)			{ return AppendString(inString); }
			
			VString&			operator =( const UniChar* inUniCString)		{ FromBlock(inUniCString, UniCStringLength( inUniCString) * sizeof(UniChar), VTC_UTF_16); return *this; }
			VString&			operator +=( const UniChar* inUniCString)		{ return AppendUniCString(inUniCString); }

		#if !WCHAR_IS_UNICHAR
			VString&			operator =( const wchar_t* inUniCString)		{ FromBlock(inUniCString,( VSize) ::wcslen(inUniCString) * sizeof(wchar_t), VTC_WCHAR); return *this; }
			VString&			operator +=( const wchar_t* inUniCString)		{ return AppendUniCString(inUniCString); }
		#endif 

			VString&			operator =( const UniChar inUniChar)			{ FromUniChar( inUniChar); return *this; }
			VString&			operator +=( const UniChar inUniChar)			{ return AppendUniChar(inUniChar); }

			VString&			operator =( const char* inCString)				{ FromBlock(inCString,( VSize) ::strlen( inCString), VTC_StdLib_char); return *this; }
			VString&			operator +=( const char* inCString)				{ return AppendCString( inCString); }
			
			// Utilities
			void				Printf( const char* inMessage, ...);	// Formatted string as printf
			void				VPrintf( const char* inMessage, va_list argList);

			bool				IsStringValid() const;

			void				GetCleanJSName(VString& outJSName) const;

protected:
		#if !WCHAR_IS_UNICHAR
			// private constructor for VStr template and CVSTR macro
			// the goal is to quickly builds a vstring wrapper around a const string litteral.
								VString( size_t inLength, const wchar_t *inString);
		#endif
								
			// private utility for VInlineString
			UniChar*			RetainBuffer( VIndex *outLength, VIndex *outMaxLength) const;

			UniPtr				fString;	// Pointer to null-terminated string
			VIndex				fLength;	// Nb chars
			VIndex				fMaxLength;	// Max nb of chars fString can handle( not including the null char)
			VIndex				fMaxBufferLength;	// Max nb of chars fBuffer can handle( not including the null char)
			UniChar				fBuffer[1];	// Private buffer, may be extended using the VStr template

			// Inherited from VValue
	virtual	void				DoNullChanged();
			
			// Allocate operators( Let VString:NewString and the VStr<> template dynamically reserve space for the string in the same cpp block as this)
			void*				operator new( size_t inSize, VIndex inExtraChars)	{ return VValue::operator new(inSize + inExtraChars * sizeof(UniChar)); }
			void				operator delete( void* /*inAddress*/, sLONG /*inExtraChars*/) {};

			VCppMemMgr*			_GetBufferAllocator() const						{ return VObject::GetMainMemMgr();}
			
			// Call to publicize the extra buffer size( e.g. VStr<> template constructor or by VString::NewString)
			void				_AdjustPrivateBufferSize( VIndex inExtraChars)	{ fMaxBufferLength += inExtraChars; fMaxLength = fMaxBufferLength; }
			void				_Clear();

			// reverse meaning cause by default VString owns its buffer
			bool				_IsStringPointerOwner() const					{ return !GetFlag( Value_flag1); }
			void				_SetStringPointerOwner()						{ ClearFlag( Value_flag1); }
			void				_ClearStringPointerOwner()						{ SetFlag( Value_flag1); }

			bool				_IsFixedSize() const							{ return GetFlag( Value_flag2); }
			void				_SetFixedSize()									{ SetFlag( Value_flag2); }
			void				_ClearFixedSize()								{ ClearFlag( Value_flag2); }

			// reverse meaning cause by default VString is shared
			bool				_IsSharable() const								{ return !GetFlag( Value_flag3); }
			void				_SetSharable()									{ ClearFlag( Value_flag3); }
			void				_ClearSharable()								{ SetFlag( Value_flag3); }

			// the refcount is located at the very end of the buffer, over the last two UniChars above the fMaxLength limit and its null char.
			bool				_IsBufferRefCounted() const						{ return (fString != fBuffer) && _IsSharable(); }
			sLONG*				_GetBufferRefCountPtr() const					{ return reinterpret_cast<sLONG*>( fString + ((fMaxLength + 2) & 0xFFFFFFFE));}
			UniChar*			_RetainBuffer() const;
			void				_ReleaseBuffer() const;
			bool				_IsShared() const								{ return _IsBufferRefCounted() && (*_GetBufferRefCountPtr() > 1);}
	
private:
			void				_Printf( const char* inFormat, va_list inArgList);
			bool				_ReallocBuffer( VIndex inNbChars, bool inCopyCharacters);
			bool				_EnlargeBuffer( VIndex inNbChars, bool inCopyCharacters);
			bool				_PrepareWriteKeepContent( VIndex inNbChars)				{ return _IsShared() ? _ReallocBuffer( Max( inNbChars, fLength), true) : _EnsureSize( inNbChars, true); }
			bool				_PrepareWriteTrashContent( VIndex inNbChars)			{ return _IsShared() ? _ReallocBuffer( inNbChars, false) : _EnsureSize( inNbChars, false); }
			bool				_EnsureSize( VIndex inNbChars, bool inCopyCharacters)	{ return ( inNbChars <= fMaxLength) ? true : _EnlargeBuffer( inNbChars, inCopyCharacters); }
};


/*!
	@class	VStr<inNbChars>
	@abstract	Templated pre-allocated-buffer strings
	@discussion
		This class extend the buffer size of VString to the size specified as template
		See VString for more explanations.
*/

template <VIndex inNbChars>
class VStr : public VString
{
public:
								VStr()								{ _AdjustPrivateBufferSize( inNbChars); }
	explicit					VStr( const VString& inString)		{ _AdjustPrivateBufferSize( inNbChars); FromString( inString); }
	explicit					VStr( const UniChar* inUniCString)	{ _AdjustPrivateBufferSize( inNbChars); FromUniCString( inUniCString); }
	explicit					VStr( const UniChar inUniChar)		{ _AdjustPrivateBufferSize( inNbChars); FromBlock(&inUniChar, sizeof(UniChar), VTC_UTF_16); }
	explicit					VStr( const char* inCString)		{ _AdjustPrivateBufferSize( inNbChars); FromCString( inCString); }
	explicit					VStr( const void* inBuffer, sLONG inNbBytes, CharSet inCharSet)	{ _AdjustPrivateBufferSize( inNbChars); FromBlock(inBuffer, inNbBytes, inCharSet); }

	#if !WCHAR_IS_UNICHAR
			// private constructor for CVSTR macro
	explicit					VStr( VSize inLength, const wchar_t *inString):VString( inLength, inString)	{;}
	#endif
	
			// Operators
			VString&			operator=( const VString& inString)			{ FromString(inString); return *this; }
			VString&			operator+=( const VString& inString)		{ return AppendString(inString); }
			
			VString&			operator=( const UniChar* inUniCString)		{ FromBlock(inUniCString, UniCStringLength(inUniCString) * sizeof(UniChar), VTC_UTF_16); return *this; }
			VString&			operator+=( const UniChar* inUniCString)	{ return AppendUniCString(inUniCString); }

		#if !WCHAR_IS_UNICHAR
			VString&			operator=( const wchar_t* inUniCString)		{ FromBlock(inUniCString,( VSize) ::wcslen(inUniCString) * sizeof(wchar_t), VTC_WCHAR); return *this; }
			VString&			operator+=( const wchar_t* inUniCString)	{ return AppendUniCString(inUniCString); }
		#endif

			VString&			operator=( const UniChar inUniChar)			{ FromBlock(&inUniChar, sizeof(UniChar), VTC_UTF_16); return *this; }
			VString&			operator+=( const UniChar inUniChar)		{ return AppendUniChar(inUniChar); }

			VString&			operator=( const char* inCString)			{ FromBlock(inCString,( VSize) ::strlen(inCString), VTC_StdLib_char); return *this; }
			VString&			operator+=( const char* inCString)			{ return AppendCString(inCString); }

private:
	UniChar	fExtraBuffer[(uLONG)inNbChars];
};

typedef VStr<4>		VStr4;
typedef VStr<8>		VStr8;
typedef VStr<15>	VStr15;
typedef VStr<31>	VStr31;
typedef VStr<40>	VStr40;
typedef VStr<63>	VStr63;
typedef VStr<80>	VStr80;
typedef VStr<255>	VStr255;



/*!
	 @function 	CVSTR
	 @abstract	Macro to instantiate a const VString from a c++ string constant.
	 @discussion
	 You can use CVSTR whenever a function call expects a const VString&.
	 This is in hope to avoid the string copy and conversion.

	 Example:
	 	window->SetWindowTitle(CVSTR("hello"));
*/


// when available, we will use the 'u' suffix
#if WCHAR_IS_UNICHAR
#define CVSTR(_const_string_)( (const XBOX::VString&) XBOX::VString(const_cast<UniChar *>(L ## _const_string_ L""), sizeof( _const_string_) - 1, (XBOX::VSize) -1))
#else
#define CVSTR(_const_string_)( (const XBOX::VString&) XBOX::VStr<sizeof( _const_string_)>( sizeof( _const_string_) - 1, _const_string_ L""))
#endif

/*!
	@class	VInlineString
	@abstract	struct based String management class.
	@discussion
		This struct is a non-object, simplified, version of VString.
		It is intended for rare occasions such as the 'champvar' union in 4D where we can avoid the allocation of a VString object.
		You should not use this struct in new code.

		Although all fields are public because it is a struct, do not mess with them directly.

		fTag is used to detect a block copy: when comparing two VInlineString if fTag is equal then it is a block copy and not a copy by refcount.
		
		warning: 4D compiler duplicates this struct in comptype.h and assumes packing
*/

// always pack 2 to be stable regarding plugins and 4D Compiler and 32/64 bit modes
#pragma pack( push, 2)

XTOOLBOX_API extern uLONG __VInlineString_tag;

typedef struct VInlineString
{
	VIndex			fLength;
	UniChar*		fString;
	VIndex			fMaxLength;
	uLONG			fTag;
	
	void Init()
	{
		fString = NULL;
		fMaxLength = 0;
		fLength = 0;
		fTag = __VInlineString_tag++;
	}
	
	void InitWithString( const VInlineString& inString)
	{
		if (inString.fString != NULL)
		{
			xbox_assert( *inString.GetBufferRefCountPtr() > 0);
			VInterlocked::Increment( inString.GetBufferRefCountPtr());
		}
		fString = inString.fString;
		fMaxLength = inString.fMaxLength;
		fLength = inString.fLength;
		fTag = __VInlineString_tag++;
	}
	
	void InitWithString( const VString& inString)
	{
		fString = inString.RetainBuffer( &fLength, &fMaxLength);
		fTag = __VInlineString_tag++;
	}

	bool InitWithUniChars( const UniChar *inChars, VIndex inNbChars)
	{
		bool ok;
		if (inNbChars > 0)
		{
			VSize newSize = (inNbChars + 4L) & 0xFFFFFFFE;

			UniChar *buffer = (UniChar*) VObject::GetMainMemMgr()->NewPtr( newSize * sizeof(UniChar), false, 'str ');
			if (buffer != NULL)
			{
				::memcpy( buffer, inChars, inNbChars * sizeof(UniChar));
				buffer[inNbChars] = 0; // set null char
				*(sLONG*) (buffer + ((inNbChars + 2) & 0xFFFFFFFE)) = 1;	// set refcount
				fLength = fMaxLength = inNbChars;
				fString = buffer;
				ok = true;
			}
			else
			{
				fString = NULL;
				fLength = fMaxLength = 0;
				ok = false;
			}
		}
		else
		{
			fString = NULL;
			fMaxLength = 0;
			fLength = 0;
			ok = true;
		}

		fTag = __VInlineString_tag++;
		return ok;
	}
	
	void FromString( const VString& inString)
	{
		if (inString.GetCPointer() != fString)
		{
			ReleaseBuffer();
			fString = inString.RetainBuffer( &fLength, &fMaxLength);
		}
	}
	
	void FromString( const VInlineString& inString)
	{
		if (inString.fString != fString)
		{
			if (inString.fString != NULL)
			{
				xbox_assert( *inString.GetBufferRefCountPtr() > 0);
				VInterlocked::Increment( inString.GetBufferRefCountPtr());
			}
			ReleaseBuffer();
			fString = inString.fString;
			fMaxLength = inString.fMaxLength;
			fLength = inString.fLength;
		}
	}
	
	void FromUniChar( UniChar inChar)
	{
		ReleaseBuffer();

		// UniChar + zero + aligned refcount = 8 bytes
		UniChar *buffer = (UniChar*) VObject::GetMainMemMgr()->NewPtr( 8, false, 'str ');
		if (buffer != NULL)
		{
			buffer[0] = inChar;
			buffer[1] = 0;
			*(sLONG*) (buffer + 2) = 1;	// set refcount
			fLength = 1;
			fMaxLength = 1;
			fString = buffer;
		}
		else
		{
			fString = NULL;
			fLength = fMaxLength = 0;
		}
	}

	UniChar* RetainBuffer( VIndex *outLength, VIndex *outMaxLength) const
	{
		if (fString != NULL)
		{
			xbox_assert( *GetBufferRefCountPtr() > 0);
			VInterlocked::Increment( GetBufferRefCountPtr());
			*outLength = fLength;
			*outMaxLength = fMaxLength;
		}
		else
		{
			*outLength = *outMaxLength = 0;
		}
		return fString;
	}

	void RetainInPlace()
	{
		if (fString != NULL)
		{
			XBOX::VInterlocked::Increment( GetBufferRefCountPtr());
			fTag = __VInlineString_tag++;
		}
	}

	void ReleaseBuffer() const
	{
		if (fString != NULL)
		{
			xbox_assert( *GetBufferRefCountPtr() > 0);
			sLONG newCount = VInterlocked::Decrement( GetBufferRefCountPtr());
			if (newCount == 0)
			{
				VObject::GetMainMemMgr()->Free( fString);
			}
		}
	}
	
	// returns false if failed because of refcount != 1 or because of bad index.
	bool SetUniChar( VIndex inIndex, UniChar inChar)
	{
		if ( (inIndex > 0) && (inIndex <= fLength) && (*GetBufferRefCountPtr() == 1) )
		{
			fString[inIndex-1] = inChar;
			return true;
		}
		return false;
	}
	
	/** @brief make a copy if refcount > 1	**/
	bool PrepareForExport()
	{
		// fMaxLength may be lost that's why we force the copy if different than fLength.
		if ( (fString == NULL) || (*GetBufferRefCountPtr() <= 1 && (fMaxLength == fLength) ) )
			return true;
		
		return _Copy();
	}

	bool _Copy()
	{
		VSize newSize = (fLength + 4L) & 0xFFFFFFFE;

		UniChar *buffer = (UniChar*) VObject::GetMainMemMgr()->NewPtr( newSize * sizeof(UniChar), false, 'str ');
		if (buffer != NULL)
		{
			::memcpy( buffer, fString, (fLength + 1) * sizeof(UniChar));
			ReleaseBuffer(); // release fString
			*(sLONG*) (buffer + ((fLength + 2) & 0xFFFFFFFE)) = 1;	// set refcount
			fMaxLength = fLength;
			fString = buffer;
		}
		else
		{
			VInterlocked::Decrement( GetBufferRefCountPtr());
			fString = NULL;
			fLength = fMaxLength = 0;
		}

		fTag = __VInlineString_tag++;
		return buffer != NULL;
	}
	
	void				Dispose()							{ ReleaseBuffer(); }
	bool				IsEmpty() const						{ return fString == NULL;}
	VIndex				GetLength() const					{ return fLength;}
	const UniChar*		GetCPointer() const					{ return fString;}
	sLONG*				GetBufferRefCountPtr() const		{ return reinterpret_cast<sLONG*>( fString + ((fMaxLength + 2) & 0xFFFFFFFE)); }
	const UniChar&		operator[]( VIndex inIndex) const	{ xbox_assert( inIndex >= 0 && inIndex < fLength); return fString[inIndex]; }
	
} VInlineString;

#pragma pack( pop )


/*!
	@class	VOsTypeString
	@abstract	Utility VString for handling OSTypes
	@discussion
*/

#if VERSIONWIN
#pragma warning(push)
#pragma warning(disable:4275)
#endif

class XTOOLBOX_API VOsTypeString : public VString
{
public:
			VOsTypeString()								{ _AdjustPrivateBufferSize( 4); (void) fExtraBuffer; /*silence warning*/}
			VOsTypeString( OsType inType );
			VOsTypeString( const VString& inOriginal)	{ _AdjustPrivateBufferSize( 4); FromString( inOriginal);}
	virtual	~VOsTypeString() {};

private:
	UniChar	fExtraBuffer[4];
};

#if VERSIONWIN
#pragma warning(pop)
#endif


/*!
	@class	StStringConverter
	@abstract	String convertion utility class.
	@discussion
		A facility class to ease the writing of convertions to Ansi or Mac characters.
		Use eCRM_NATIVE if you want proper line feeds convertion( Windows: cr->crlf, Mac: crlf->cr)
	
	example 1:
	void DrawText(const VString& inString)
	 { 
		StStringConverter<char> buff(inString, VTC_MacOSAnsi);
		
		::DrawText( buff, buff.GetLength());
	 };
	 
	
	example 2:
	void DrawText(const VString& inString1, const VString& inString2)
	 { 
		StStringConverter<char> buff(VTC_MacOSAnsi);
		
		::DrawText(buff.ConvertString(inString1), buff.GetLength());
		::DrawText(buff.ConvertString(inString2), buff.GetLength());
	 };
	 
	 exemple 3:
	 void PrintText(const VString& inString)
	 {
		// wchar_t is utf_32 with gcc, but utf_16 with visual
		StStringConverter<wchar_t> buff(inString, VTC_WCHAR_T);
		
		// cast operator converts buff to wchar_t*
		::fwprintf( stdout, buff);
	 };
*/

class XTOOLBOX_API StStringConverterBase
{
protected:
									StStringConverterBase( CharSet inDestinationCharSet, ECarriageReturnMode inCRMode = eCRM_NONE);
									~StStringConverterBase();

			void					_ConvertString( const UniChar *inUniPtr, VIndex inLength, VSize inCharSize);

			void*					fBuffer;
			VSize					fStringSize;
			ECarriageReturnMode		fCRMode;	// Linefeeds convertion mode
			bool					fOK;

private:
			VSize					fBufferSize;
			void*					fSecondaryBuffer;			// Extra buffer
			VFromUnicodeConverter*	fConverter;
			char					fPrimaryBuffer[512];
};

template<typename T>
class StStringConverter : private StStringConverterBase
{
public:
									StStringConverter( CharSet inDestinationCharSet, ECarriageReturnMode inCRMode = eCRM_NONE)
									:StStringConverterBase( inDestinationCharSet, inCRMode)
									{
									}

									StStringConverter( const VString& inStringToConvert, CharSet inDestinationCharSet, ECarriageReturnMode inCRMode = eCRM_NONE)
									:StStringConverterBase( inDestinationCharSet, inCRMode)
									{
										ConvertString( inStringToConvert);
									}

									StStringConverter( const VInlineString& inStringToConvert, CharSet inDestinationCharSet, ECarriageReturnMode inCRMode = eCRM_NONE)
									:StStringConverterBase( inDestinationCharSet, inCRMode)
									{
										ConvertString( inStringToConvert);
									}
	
	// String convertion support
			const T*				ConvertString( const VString& inStringToConvert)
									{
										if (fCRMode != eCRM_NONE)
										{
											VString tempString( inStringToConvert);
											tempString.ConvertCarriageReturns( fCRMode);
											_ConvertString( tempString.GetCPointer(), tempString.GetLength(), sizeof( T));
										}
										else
										{
											_ConvertString( inStringToConvert.GetCPointer(), inStringToConvert.GetLength(), sizeof( T));
										}
										
										return GetCPointer();
									}

			const T*				ConvertString( const VInlineString& inStringToConvert)
									{
										if (fCRMode != eCRM_NONE)
										{
											VString tempString( inStringToConvert);
											tempString.ConvertCarriageReturns( fCRMode);
											_ConvertString( tempString.GetCPointer(), tempString.GetLength(), sizeof( T));
										}
										else
										{
											_ConvertString( inStringToConvert.GetCPointer(), inStringToConvert.GetLength(), sizeof( T));
										}
										
										return GetCPointer();
									}
	
	// Returns number of bytes produced by convertion (not including trailing zeros)
			VSize					GetSize() const				{ return fStringSize; }
		
	// Returns number of characters produced by convertion (not including trailing zeros)
	// WARNING: This may be meaningless in case of varying size character sets (VTC_UTF_8 or VTC_StdLib_char)
			VIndex					GetLength() const			{ return (VIndex) (fStringSize / sizeof( T)); }

			const T*				GetCPointer() const			{ return reinterpret_cast<const T*>( fBuffer); }

			bool					IsOK() const				{ return fOK; }
			
									operator const T*() const	{ return reinterpret_cast<const T*>( fBuffer); }
};

typedef StStringConverter<char> VStringConvertBuffer;

VString XTOOLBOX_API operator +(const VString& s1, const VString& s2);
VString XTOOLBOX_API ToString(sLONG n);
VString XTOOLBOX_API ToString(sLONG8 n);
#if ARCH_32 || COMPIL_GCC
VString XTOOLBOX_API ToString(uLONG8 n);
#endif

VString XTOOLBOX_API ToString(VSize n);

// Object to be used in maps, when the key is a VString and comparison must be case sensitive (ie: map of environment variables)
typedef struct
{
	inline bool operator() (const VString &inFirst, const VString &inSecond) const
	{
		return inFirst.CompareToString(inSecond, true) == CR_SMALLER;
	}
} VStringLessCompareStrict;

END_TOOLBOX_NAMESPACE

#endif

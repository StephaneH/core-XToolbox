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
#ifndef _ILEXER_H_
#define _ILEXER_H_

#include "ILexerInput.h"

BEGIN_TOOLBOX_NAMESPACE

class VString;

class XTOOLBOX_API ILexerToken {
public:
	enum TYPE {
		TT_WHITESPACE = 0,
		TT_NUMBER,
		TT_PUNCTUATION,
		TT_COMPARISON,
		TT_SQL_KEYWORD,
		TT_SQL_SCALAR_FUNCTION,
		TT_SQL_COLUMN_FUNCTION,
		TT_SQL_DEBUG,
		TT_SQL_BLOB,			/* Binary data in X'<hexadecimal>' form. */
		TT_NAME,
		TT_STRING,
		TT_COMMENT,
		TT_OPEN_COMMENT,
		TT_OPEN_STRING,
		TT_OPEN_NAME,
		TT_JAVASCRIPT_KEYWORD,
		TT_JAVASCRIPT_FUTURE_RESERVED_WORD,
		TT_JAVASCRIPT_REGEXP,
		TT_HTML_ENTITY,
		TT_SPECIAL_4D_COMMENT,
		TT_INVALID
	};

	virtual ILexerToken::TYPE GetType ( ) = 0;
	virtual VIndex GetPosition ( ) = 0; /* 0-based position index of a token within an input string */
	virtual VIndex GetLength ( ) = 0;
	virtual VString GetText( ) = 0;	/* Possibly NULL, such as in the case of whitespace or comments */
	virtual int GetValue( ) = 0;			/* The token value as defined by yytokentype enumeration */

	// Since tokens can reside in different components, we need to ensure consistent
	// memory management strategies.
	virtual void SelfDelete() = 0;

	VString GetTypeString()
	{
		VString type;

		switch ( GetType() )
		{
		case TT_WHITESPACE:							type = CVSTR("whitespace"); break;
		case TT_NUMBER:								type = CVSTR("number"); break;
		case TT_PUNCTUATION:						type = CVSTR("punctuation"); break;		
		case TT_COMPARISON:							type = CVSTR("comparaison"); break;
		case TT_SQL_KEYWORD:						type = CVSTR("sql_keyword"); break;
		case TT_SQL_SCALAR_FUNCTION:				type = CVSTR("sql_scalar_function"); break;
		case TT_SQL_COLUMN_FUNCTION:				type = CVSTR("sql_column_function"); break;
		case TT_SQL_DEBUG:							type = CVSTR("sql_debug"); break;
		case TT_SQL_BLOB:							type = CVSTR("sql_blob"); break;
		case TT_NAME:								type = CVSTR("name"); break;
		case TT_STRING:								type = CVSTR("string"); break;
		case TT_COMMENT:							type = CVSTR("comment"); break;
		case TT_OPEN_COMMENT:						type = CVSTR("open_comment"); break;
		case TT_OPEN_STRING:						type = CVSTR("open_string"); break;
		case TT_OPEN_NAME:							type = CVSTR("open_name"); break;
		case TT_JAVASCRIPT_KEYWORD:					type = CVSTR("javascript_keyword"); break;
		case TT_JAVASCRIPT_FUTURE_RESERVED_WORD:	type = CVSTR("javascript_future_reserved_keyword"); break;
		case TT_JAVASCRIPT_REGEXP:					type = CVSTR("javascript_regular_expression"); break;
		case TT_HTML_ENTITY:						type = CVSTR("html_entity"); break;
		case TT_SPECIAL_4D_COMMENT:					type = CVSTR("special_4D_comment"); break;
		case TT_INVALID:							type = CVSTR("invalid"); break;

		default:
			break;
		}

		return type;
	}
};

typedef std::vector< ILexerToken * > TokenList;

// This is the base interface for generically working with a lexer
// subclass.  However, lexer subclasses will generally inherit from 
// the common lexer base class: VLexerBase
class XTOOLBOX_API ILexer {
public:
	// This function call will advance to the next token, and store the token
	// value for future reference.
	virtual int GetNextTokenForParser() = 0;
	// This function will not advance to the next token.  Instead, it returns the
	// last token we succesfully lexed.
	virtual int GetLastToken() = 0;
	virtual VError Tokenize( TokenList &outTokens, bool inContinuationOfComment = false, sLONG inOpenStringMode  = 0) = 0;
};

// This is a common base class for lexers that takes care of typical
// lexer functionality, such as eating whitespaces, etc.  Generally
// speaking, any new lexers will want to be a subclass of this base class.
class XTOOLBOX_API VLexerBase : public VObject, public ILexer {
public:
	virtual int GetLastToken() { return fLastToken; }

protected:
	virtual void ReportLexerError( VError inError, const char *inMessage ) = 0;

	virtual bool IsWhitespace( UniChar inChar ) = 0;
	bool SkipWhitespaces( bool &outConsumedWhitespace, TokenList *ioTokens = NULL );

	// If the comment start or end requires more than one character of lookahead,
	// it is up to the subclass to peek at subsequent characters in the stream.
	// Different languages have different commenting needs, including multiple
	// ways of expressing a comment.  For instance, JavaScript can use /* */ as
	// well as <!-- --> for its comments.  So the "type" parameter is used to identify
	// what type of comment was found so that the appropriate match can be located.  Each
	// lexer should choose a unique type identifier (such as a four char code) for the language.
	const sLONG kCommentContinuationType;	// 'CmCt'
	virtual bool IsMultiLineCommentStart( UniChar inChar, sLONG &outType ) = 0;
	virtual bool IsMultiLineCommentEnd( UniChar inChar, sLONG &outCharsToConsume, sLONG inType ) = 0;
	virtual bool IsSingleLineCommentStart( UniChar inChar ) = 0;
	virtual bool IsLineEnding( UniChar inChar ) = 0;
	// The caller is responsible for the line ending they pass in, but this call will take care
	// of consuming any subsequent characters in the stream that are part of the original line
	// ending.  This allows a language to handle CRLF line endings transparently.
	virtual void ConsumeLineEnding( UniChar inChar );

	// It is assumed by this function that the current character is the start
	// of the comment stream.  This function will read until IsMultiLineCommentEnd
	// returns true (or an error occurs, such as the end of the stream), at which
	// point the current character will point to the character after the closing
	// comment marker.
	virtual bool ConsumeMultiLineComment( TokenList *ioTokens, sLONG inType );
	bool ConsumeSingleLineComment( TokenList *ioTokens );
	// This function can be overloaded by a lexer to handle comment documentation mechanisms,
	// like ScriptDoc.  It will be called whenever a comment has been consumed and contain the
	// entire contents of the comment, including the opener and closer.  However, this is not
	// something the implementor is required to provide, so we have a default no-op implementation.
	// Note that the inType will be 0 in the case of a single-line comment.
	virtual void CommentConsumed( const VString &/*inText*/, sLONG /*inType*/ ) { }

	// Similar to comments, strings can be enclosed in multiple ways within a single language.
	// For instance, in SQL, a string can either be quoted with '', or with ``.  So the "type"
	// parameter is used to distinguish what type of string is being lexed so that we can match
	// it appropriately.  The "value" passed around should be the token's type as it will be assigned
	// to the token added to the token list.  If there is an escape sequence, the subclass is responsible
	// for consuming the escape sequence and adding whatever characters to the buffer it wishes.  Note that
	// the subclass should be using AppendUniCharWithBuffer to add characters to the buffer if it must.
	virtual bool IsStringStart( UniChar inChar, sLONG &outType, sLONG &outValue ) = 0;
	virtual bool IsStringEnd( UniChar inChar, sLONG inType ) = 0;
	virtual bool IsStringEscapeSequence( UniChar inChar, sLONG inType ) = 0;
	virtual bool IsStringContinuationSequence( UniChar inChar, sLONG inType ) = 0;
	virtual bool ConsumeEscapeSequence( sLONG inType, UniChar &outCharacterToAdd ) = 0;
	bool ConsumeString( VString* vstrBuffer, TokenList *ioTokens, sLONG inType, sLONG inValue );

	virtual bool IsIdentifierStart( UniChar inChar ) = 0;
	virtual bool IsIdentifierCharacter( UniChar inChar ) = 0;
	virtual bool IsRegularExpressionStart( UniChar /*inChar*/ ) { return false; }

	// It is possible that the language has special requirements for their identifiers, so this
	// method can be overloaded.  For instance, in SQL, a name can be bracketed by [], which has
	// SQL-specific escape sequences, etc.  However, if the languages identifiers have no special
	// requirements, this simply consumes anything which is an identifier character up to the non-
	// identifier character encountered.
	virtual VString *ConsumeIdentifier();

	// We will also provide some simple mechansisms for languages to suppose hex, oct and bin numeric
	// entry in the cases where there's a prefix.  eg) &b10101010, the caller will know by the &b that
	// only binary numbers are legal values.
	enum ENumericType {
		eNumericUnknown = -1,
		eNumericInteger,
		eNumericReal,
		eNumericScientific,
		eNumericHex,
		eNumericOct,
		eNumericBin,
	};
	virtual int GetNumericTokenValueFromNumericType( ENumericType inType ) = 0;

	// Consume a string of decimal values (0-9) until a non-decimal value (including . and e) are
	// encountered.  This can be used to do more complex numerical lexing.
	virtual bool ConsumeDecimalNumbers( VString *ioNumber );

	// Consumes a string of hex, oct or binary numbers until a non-entity is encountered.
	virtual bool ConsumeHexNumber( VString *ioNumber );
	virtual bool ConsumeOctNumber( VString *ioNumber );
	virtual bool ConsumeBinNumber( VString *ioNumber );

	ILexerInput *fLexerInput; // will be allocated and released by implementation class
	bool	fCommentContinuationMode;
	sLONG	fOpenStringType;
	int fLastToken;

	void AppendUniCharWithBuffer ( UniChar inUChar, VString* inStr, UniChar* inBuffer, VIndex& ioBufferLength, VIndex inMaxBufferLength );

	explicit VLexerBase();
	virtual void SetLexerInput( ILexerInput *inInput );
	void SetLastToken( int token ) { fLastToken = token; }
};

END_TOOLBOX_NAMESPACE

#endif // _ILEXER_H_

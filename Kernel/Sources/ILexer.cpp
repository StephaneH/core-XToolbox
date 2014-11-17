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
#include "VKernelPrecompiled.h"

#include "VString.h"
#include "VKernelErrors.h"
#include "ILexer.h"

class VLexerToken : public VObject, 
	public ILexerToken {
public:
	VLexerToken ( ILexerToken::TYPE inType, VIndex inPosition, VIndex inLength, VString inText = "", int inValue = -1) :
												m_nType ( inType ), m_nPosition ( inPosition ), m_nLength ( inLength ),
												m_nTokenValue( inValue ), m_vstrText( inText )
												{}

	virtual ~VLexerToken() { }
	
	virtual ILexerToken::TYPE GetType ( ) { return m_nType; }
	virtual XBOX::VIndex GetPosition ( ) { return m_nPosition; }
	virtual XBOX::VIndex GetLength ( ) { return m_nLength; }
	virtual VString GetText( ) { return m_vstrText; }
	virtual int GetValue( ) { return m_nTokenValue; }

	virtual void SelfDelete() { delete this; }

private:
	ILexerToken::TYPE			m_nType;
	VIndex						m_nPosition;
	VIndex						m_nLength;
	VString 					m_vstrText;
	int							m_nTokenValue;
};

VLexerBase::VLexerBase() : 
	fCommentContinuationMode( false ),
	kCommentContinuationType( 'CmCt' ),
	fLastToken( -1 ),
	fOpenStringType( 0 ),
	fLexerInput( NULL )
{
}

void VLexerBase::SetLexerInput( ILexerInput *inInput )
{
	if (fLexerInput)	delete fLexerInput;
	fLexerInput = inInput;
}

void VLexerBase::AppendUniCharWithBuffer ( UniChar inUChar, VString* inStr, UniChar* inBuffer, VIndex& ioBufferLength, VIndex inMaxBufferLength )
{
	inBuffer [ ioBufferLength ] = inUChar;
	ioBufferLength++;
	if ( ioBufferLength == inMaxBufferLength )
	{
		inStr-> AppendUniChars ( inBuffer, inMaxBufferLength );
		ioBufferLength = 0;
	}
}

static void DrainBuffer( VString *str, UniChar buffer[], VIndex &ioSize )
{
	if (ioSize)	str->AppendUniChars( buffer, ioSize );
	ioSize = 0;
}

void VLexerBase::ConsumeLineEnding( UniChar inLineEnding )
{
	// The only multi-byte line ending we need to care about is a CRLF, in which
	// case we want to eat the LF.
	if (inLineEnding == CHAR_CONTROL_000D) {	// Carriage Return
		if (fLexerInput->HasMoreChars() && fLexerInput->PeekAtNextChar() == CHAR_CONTROL_000A)	{	// Line feed
			fLexerInput->MoveToNextChar();	// eat it!
		}
	}
}

bool VLexerBase::SkipWhitespaces( bool &outConsumedWhitespace, TokenList *ioTokens )
{
	sLONG		nLength = fLexerInput->GetLength();
	UniChar		uChar = 0;
	VIndex		nCount = 0;
	VIndex		nStart = 0;
	sLONG		commentID = 0;

	UniChar	szUCharBuffer [ 256 ];
	VIndex nBufferSize = 0;
	VString vstrBuffer;
	while (fLexerInput->HasMoreChars()) {
		uChar = fLexerInput->MoveToNextChar();
		if (!fCommentContinuationMode && IsWhitespace( uChar )) {
			outConsumedWhitespace = true;
			if (ioTokens != 0) {
				AppendUniCharWithBuffer ( uChar, &vstrBuffer, szUCharBuffer, nBufferSize, 256 );
				if (nCount == 0)	nStart = fLexerInput->GetCurrentPosition() - 1;
				nCount++;
			}
			continue;
		} else if (!fCommentContinuationMode && 
					fLexerInput->HasMoreChars() &&
					IsSingleLineCommentStart( uChar )) {
			outConsumedWhitespace = true;
			if (ioTokens != 0 && nCount > 0) {
				DrainBuffer( &vstrBuffer, szUCharBuffer, nBufferSize );
				ioTokens->push_back( new VLexerToken( ILexerToken::TT_WHITESPACE, nStart, nCount, vstrBuffer ) );
				nCount = 0;
			}

			// We have to back up a character in the stream because ConsumeSingleLineComment
			// assumes that it is starting from the comment opener when doing the consumption.
			fLexerInput->MoveToPreviousChar();

			if (!ConsumeSingleLineComment( ioTokens ))	return false;
			continue;
		} else if (fCommentContinuationMode ||
					IsMultiLineCommentStart( uChar, commentID )) {
			outConsumedWhitespace = true;
			if (fCommentContinuationMode) {
				commentID = kCommentContinuationType;
				fCommentContinuationMode = false;
			}

			// We have to back up a character in the stream because ConsumeMultiLineComment
			// assumes that it is starting from the comment opener when doing the consumption.
			// Also, if this is a continuation, then we need to back up to be able to determine
			// whether the first character was part of the terminus or not.
			fLexerInput->MoveToPreviousChar();

			int testStart = fLexerInput->GetCurrentPosition();

			if (ioTokens != 0 && nCount > 0) {
				DrainBuffer( &vstrBuffer, szUCharBuffer, nBufferSize );
				ioTokens->push_back( new VLexerToken( ILexerToken::TT_WHITESPACE, nStart, nCount, vstrBuffer ) );
				nCount = 0;
			}

			if (!ConsumeMultiLineComment( ioTokens, commentID ))	return false;
			continue;
		}
		
		if (ioTokens != 0 && nCount > 0) {
			DrainBuffer( &vstrBuffer, szUCharBuffer, nBufferSize );
			ioTokens->push_back( new VLexerToken( ILexerToken::TT_WHITESPACE, nStart, nCount, vstrBuffer ) );
		}

		// Roll back one char and return
		fLexerInput->MoveToPreviousChar();
		
		return true;
	}
	
	if (ioTokens != 0 && nCount > 0) {
		DrainBuffer( &vstrBuffer, szUCharBuffer, nBufferSize );
		ioTokens->push_back( new VLexerToken( ILexerToken::TT_WHITESPACE, nStart, nCount ) );
	}

	return true;
}

/*	Returns true if the comment was successfully consumed. Otherwise returns false.
Nested comments are not supported.

	Preconditions: g_nCurrentChar_LEX contains 0-based index of the opening 'star'.
	
	Postconditions: If method is executed successfully, then g_nCurrentChar_LEX
will contain 0-based index of the character following the trailing 'slash'.
*/
bool VLexerBase::ConsumeMultiLineComment( TokenList *ioTokens, sLONG inType )
{
	VIndex nStart = fLexerInput->GetCurrentPosition();
	UniChar	uChar = 0;
	while (fLexerInput->HasMoreChars()) 	{
		uChar = fLexerInput->MoveToNextChar();

		// If we're not at the end of the comment, we can continue to search
		sLONG consume = 0;
		if (!IsMultiLineCommentEnd( uChar, consume, inType ))	continue;

		// We've found the end of the comment, so it's time to finish consuming it.  The
		// previous call told us how many characters we need to consume to push to the
		// end of the comment.
		for (sLONG i = 0; i < consume; ++i)	fLexerInput->MoveToNextChar();

		// GetSubString takes a 1-based offset, while nStart comes from GetCurrentPosition,
		// which is a 0-based offset.
		VString commentText;
		fLexerInput->GetSubString( nStart + 1, fLexerInput->GetCurrentPosition() - nStart + 1, commentText );

		if (ioTokens != 0) {
			ioTokens->push_back( new VLexerToken( ILexerToken::TT_COMMENT, nStart, fLexerInput->GetCurrentPosition() - nStart, commentText ) );
		}

		CommentConsumed( commentText, inType );

		return true;
	}

	// The comment was not actually closed, so we want to alert the caller
	if (ioTokens != NULL) {
		ioTokens->push_back( new VLexerToken( ILexerToken::TT_OPEN_COMMENT, nStart, fLexerInput->GetCurrentPosition() - nStart ) );
	}
	return false;
}

bool VLexerBase::ConsumeSingleLineComment( TokenList *ioTokens )
{
	VIndex nStart = fLexerInput->GetCurrentPosition();

	while (fLexerInput->HasMoreChars()) {
		UniChar c = fLexerInput->MoveToNextChar();
		if (IsLineEnding( c )) {
			ConsumeLineEnding( c );
			break;
		}
	}

	// We subtract one from the length that we pass in because we know it's a newline character, and we don't want to
	// include that as part of our comment text.
	VString commentText;
	fLexerInput->GetSubString( nStart + 1, fLexerInput->GetCurrentPosition() - nStart - 1, commentText );

	if (ioTokens != 0) {
		ioTokens->push_back( new VLexerToken( ILexerToken::TT_COMMENT, nStart, fLexerInput->GetCurrentPosition() - nStart, commentText ) );
	}

	CommentConsumed( commentText, 0 );

	return true;
}

bool VLexerBase::ConsumeString( VString* vstrBuffer, TokenList *ioTokens, sLONG inType, sLONG inValue )
{
	UniChar					szUCharBuffer [ 256 ];
	VIndex					nBufferSize = 0;
	VIndex					nStartingPosition = fLexerInput->GetCurrentPosition();
	AppendUniCharWithBuffer ( fLexerInput-> MoveToNextChar ( ), vstrBuffer, szUCharBuffer, nBufferSize, 256 );

	// handle the line containing only the line continuation character
	if ( ! fLexerInput->HasMoreChars() && IsStringContinuationSequence(szUCharBuffer[0], inType) )
	{
		if (ioTokens)
			ioTokens->push_back( new VLexerToken( ILexerToken::TT_OPEN_STRING, 0, 0, CVSTR(""), inValue ) );
		
		return false;
	}

	while (fLexerInput->HasMoreChars())
	{
		UniChar c = fLexerInput->MoveToNextChar();
		
		// keep the zero test until we are sure that javascript does not need 
		// to consume escape sequence
		if ( fLexerInput->HasMoreChars() && IsStringEscapeSequence( c, inType ))
		{
			// Now let the subclass consume the escape sequence
			UniChar characterToAdd;

			ConsumeEscapeSequence( inType, characterToAdd );
			AppendUniCharWithBuffer( characterToAdd, vstrBuffer, szUCharBuffer, nBufferSize, 256 );
		}
		else if (IsStringEnd( c, inType ))
		{
			AppendUniCharWithBuffer ( c, vstrBuffer, szUCharBuffer, nBufferSize, 256 );
			if ( nBufferSize > 0 )
				vstrBuffer->AppendUniChars( szUCharBuffer, nBufferSize );
			
			// WAK0082246
			// We can't se the token lenght to vstrBuffer->GetLength() because we replace escapable characters by their value
			// directly in the stream (i.e. the two chars "\t" will be replaced by the value CHAR_CONTROL_0009
			// The token length, for strings, must computed using currentPosition - nStartingPosition
			XBOX::VIndex currentPosition = fLexerInput->GetCurrentPosition();
			XBOX::VIndex realLength = currentPosition - nStartingPosition;
			if (ioTokens)
				ioTokens->push_back( new VLexerToken (	ILexerToken::TT_STRING, nStartingPosition, realLength, *vstrBuffer, inValue ) );

			return true;
		}
		else if (IsStringContinuationSequence(c, inType) && ! fLexerInput->HasMoreChars() )
		{
			if (ioTokens)
				ioTokens->push_back( new VLexerToken( ILexerToken::TT_OPEN_STRING, nStartingPosition, nBufferSize, *vstrBuffer, inValue ) );
			
			return false;
		}
		else
		{
			// This is just part of the regular string, so add it to the buffer
			AppendUniCharWithBuffer ( c, vstrBuffer, szUCharBuffer, nBufferSize, 256 );
		}
	}
	
	if ( nBufferSize > 0 )
		vstrBuffer->AppendUniChars( szUCharBuffer, nBufferSize );

	ReportLexerError ( VE_LEXER_NON_TERMINATED_STRING, "Unterminated string" );
	
	return false;
}

VString *VLexerBase::ConsumeIdentifier()
{
	VString*		vstrNAME = new VString ( );

	vstrNAME-> AppendUniChar ( fLexerInput-> GetCurrentChar ( ) );

	while (	fLexerInput-> HasMoreChars ( ) && IsIdentifierCharacter( fLexerInput-> PeekAtNextChar ( ) ) ) {
		vstrNAME-> AppendUniChar ( fLexerInput-> MoveToNextChar ( ) );
	}
	
	return vstrNAME;
}

bool VLexerBase::ConsumeDecimalNumbers( VString *ioNumber )
{
	UniChar					szUCharBuffer [ 128 ];
	VIndex					nBufferSize = 0;
	VIndex					nStartingPosition = fLexerInput->GetCurrentPosition();
	bool					bConsumedNumber = false;

	// Lets consume all the digits
	while (fLexerInput-> HasMoreChars()) {
		UniChar c = fLexerInput->PeekAtNextChar();
		if (!(c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_NINE))
		{
			// What we have isn't 0-9, so it's not a decimal digit.  Bail out
			break;
		} else {
			// Add the character to our buffer
			bConsumedNumber = true;
			AppendUniCharWithBuffer ( fLexerInput->MoveToNextChar(), ioNumber, szUCharBuffer, nBufferSize, 128 );
		}
	}

	if ( nBufferSize > 0 )	ioNumber-> AppendUniChars ( szUCharBuffer, nBufferSize );

	return bConsumedNumber;
}

bool VLexerBase::ConsumeHexNumber( VString *ioNumber )
{
	UniChar					szUCharBuffer [ 128 ];
	VIndex					nBufferSize = 0;
	VIndex					nStartingPosition = fLexerInput->GetCurrentPosition();
	bool					bConsumedNumber = false;

	// Lets consume all the digits
	while (fLexerInput-> HasMoreChars()) {
		UniChar c = fLexerInput->PeekAtNextChar();
		if (!(c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_NINE) &&
			!(c >= CHAR_LATIN_CAPITAL_LETTER_A && c <= CHAR_LATIN_CAPITAL_LETTER_F) &&
			!(c >= CHAR_LATIN_SMALL_LETTER_A && c <= CHAR_LATIN_SMALL_LETTER_F))
		{
			// What we have isn't 0-9a-fA-F, so it's not a hex digit.  Bail out
			break;
		} else {
			// Add the character to our buffer
			bConsumedNumber = true;
			AppendUniCharWithBuffer ( fLexerInput->MoveToNextChar(), ioNumber, szUCharBuffer, nBufferSize, 128 );
		}
	}

	if ( nBufferSize > 0 )	ioNumber-> AppendUniChars ( szUCharBuffer, nBufferSize );

	return bConsumedNumber;
}

bool VLexerBase::ConsumeOctNumber( VString *ioNumber )
{
	UniChar					szUCharBuffer [ 128 ];
	VIndex					nBufferSize = 0;
	VIndex					nStartingPosition = fLexerInput->GetCurrentPosition();
	bool					bConsumedNumber = false;

	// Lets consume all the digits
	while (fLexerInput-> HasMoreChars()) {
		UniChar c = fLexerInput->PeekAtNextChar();
		if (!(c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_SEVEN))
		{
			// What we have isn't 0-7, so it's not an octal digit.  Bail out
			break;
		} else {
			// Add the character to our buffer
			bConsumedNumber = true;
			AppendUniCharWithBuffer ( fLexerInput->MoveToNextChar(), ioNumber, szUCharBuffer, nBufferSize, 128 );
		}
	}

	if ( nBufferSize > 0 )	ioNumber-> AppendUniChars ( szUCharBuffer, nBufferSize );

	return bConsumedNumber;
}

bool VLexerBase::ConsumeBinNumber( VString *ioNumber )
{
	UniChar					szUCharBuffer [ 128 ];
	VIndex					nBufferSize = 0;
	VIndex					nStartingPosition = fLexerInput->GetCurrentPosition();
	bool					bConsumedNumber = false;

	// Lets consume all the digits
	while (fLexerInput-> HasMoreChars()) {
		UniChar c = fLexerInput->PeekAtNextChar();
		if (c != CHAR_DIGIT_ZERO && c != CHAR_DIGIT_ONE) {
			// What we have isn't 0 or 1, so it's not a binary digit.  Bail out
			break;
		} else {
			// Add the character to our buffer
			bConsumedNumber = true;
			AppendUniCharWithBuffer ( fLexerInput->MoveToNextChar(), ioNumber, szUCharBuffer, nBufferSize, 128 );
		}
	}

	if ( nBufferSize > 0 )	ioNumber-> AppendUniChars ( szUCharBuffer, nBufferSize );

	return bConsumedNumber;
}

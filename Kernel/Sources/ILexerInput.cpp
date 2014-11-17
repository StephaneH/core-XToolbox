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
#include "VFileStream.h"
#include "ILexerInput.h"

ILexerInput::~ILexerInput()
{
}


VLexerStringInput::VLexerStringInput()
{
	Init ( NULL );
}

void VLexerStringInput::Init( VString *inStatement )
{
	// Notice that we do not take control of the input statement.  Instead, ownership
	// still resides with the caller
	fInput = inStatement;
	fCurrentChar = 0;
	fCommandStart = 0;
	fCurrentLine = 1;
	if( inStatement != NULL )
		fBookMarkedPositions.push_back(fCurrentChar);
}

VLexerStringInput::~VLexerStringInput()
{
	// No need to delete m_vstrInput here because this class is not its owner
}

VIndex VLexerStringInput::GetLength()
{
	if (fInput != NULL)	return fInput->GetLength();

	xbox_assert( false );

	return 0;
}

bool VLexerStringInput::HasMoreChars()
{
	if (fInput != NULL)	return fCurrentChar < fInput->GetLength();

	xbox_assert( false );

	return 0;
}

UniChar VLexerStringInput::GetCurrentChar()
{
	if (fInput != NULL)	return fInput->GetUniChar( fCurrentChar );

	xbox_assert( false );

	return 0;
}

UniChar VLexerStringInput::PeekAtNextChar()
{
	if (fInput != NULL)	return fInput->GetUniChar( fCurrentChar + 1 );

	xbox_assert( false );

	return 0;
}

UniChar VLexerStringInput::MoveToNextChar()
{
	if (fInput != NULL)
	{
		UniChar uchResult = fInput->GetUniChar( ++fCurrentChar );
		if( _IsLineFeed(uchResult) == true )
		{
			// line feed
			fCurrentLine++;
			if( (size_t) fCurrentLine > fBookMarkedPositions.size() )
				fBookMarkedPositions.push_back(fCurrentChar);
		}

		return uchResult;
	}

	xbox_assert( false );

	return 0;
}

void VLexerStringInput::MoveToPreviousChar()
{
	if (fInput != NULL)
	{
		UniChar uchResult = fInput->GetUniChar( fCurrentChar-- );
		if( _IsLineFeed(uchResult) == true )
		{
			// line feed
			fCurrentLine--;
		}
	}
}

VIndex VLexerStringInput::GetCurrentPosition(const bool& inAbsolutePosition/*=true*/)
{
	VIndex currentPosition = fCurrentChar;
	if( inAbsolutePosition == false )
	{
		if( (size_t) fCurrentLine <= fBookMarkedPositions.size() )
			currentPosition -= fBookMarkedPositions[fCurrentLine-1];
	}

	return currentPosition;
}

void VLexerStringInput::GetSubString( sLONG inStart, sLONG inCount, VString &outResult )
{
	if (fInput == NULL) {
		xbox_assert( false );
		return;
	}

	if (inStart > GetLength() || inCount <= 0)	return;

	if (inStart <= 0)	inStart = 1;
	if (inStart + inCount > GetLength() + 1)	inCount = GetLength() - inStart + 1;
	if (inCount <= 0)	inCount = 1;

	fInput->GetSubString( inStart, inCount, outResult );
}

void VLexerStringInput::GetLastCommand( VString &outResult )
{
	fInput->GetSubString( fCommandStart + 1, fCurrentChar - fCommandStart, outResult );
}

VIndex VLexerStringInput::GetCommandStart()
{
	return fCommandStart;
}

void VLexerStringInput::PrepareForNextCommand()
{
	fCommandStart = fCurrentChar;
}

VIndex VLexerStringInput::GetCurrentLineNumber()
{
	return fCurrentLine;
}

bool VLexerStringInput::_IsLineFeed(const UniChar& inChar) const
{
	bool isLineFeed = false;
	
	if( CHAR_CONTROL_000A	== inChar ||
	   CHAR_CONTROL_000D	== inChar ||
	   (UniChar)0x2028		== inChar ||
	   (UniChar)0x2029		== inChar )
	{
		isLineFeed = true;
	}
	
	return isLineFeed;
}





VLexerFileInput::VLexerFileInput() : fPageSize( 102400 )
{
	fCurrentChar = 0;
	fCurrentPageChar = 0;

	fFileStream = NULL;
}

VLexerFileInput::~VLexerFileInput()
{
	DeInit();
}

VError VLexerFileInput::Init( const VFilePath &inPath )
{
	fCurrentChar = 0;
	fCurrentPageChar = 0;
	fCommandStart = 0;
	fCurrentLine = 0;

	xbox_assert ( fFileStream == NULL );

	VFile *vFile = new VFile( inPath );
	fFileStream = new VFileStream( vFile ); // VFileStream retains the input VFile
	vFile->Release();
	
	VError vError = fFileStream->OpenReading();
	if (vError != VE_OK) {
		delete fFileStream;
		fFileStream = NULL;

		return vError;
	}

	vError = fFileStream->GuessCharSetFromLeadingBytes( XBOX::VTC_UTF_8 );
	fFileStream->SetCarriageReturnMode( XBOX::eCRM_NATIVE );

	if (vError == VE_OK)
		vError = FillBuffer();

	return vError;
}

VError VLexerFileInput::DeInit()
{
	if (fFileStream == NULL)	return VE_OK;

	delete fFileStream;
	fFileStream = NULL;

	fCurrentChar = 0;
	fCommandStart = 0;

	return VE_OK;
}

VIndex VLexerFileInput::GetLength()
{
	if (fFileStream != NULL)
		return static_cast<VIndex>( fFileStream->GetSize());

	xbox_assert ( false );

	return 0;
}

bool VLexerFileInput::HasMoreChars()
{
	if (fFileStream != NULL) {
		if (fCurrentPageChar < fBuffer.GetLength())	return true;

		VError vError = FillBuffer();
		xbox_assert( vError == VE_OK );

		return fBuffer.GetLength() > 1; // one character is always left over from the previous buffer
	}

	xbox_assert( false );

	return false;
}

UniChar VLexerFileInput::GetCurrentChar()
{
	if (fFileStream != NULL)	return fBuffer.GetUniChar( fCurrentPageChar );

	xbox_assert( false );

	return 0;
}

UniChar VLexerFileInput::PeekAtNextChar()
{
	if (fFileStream == NULL) {
		xbox_assert( false );
		return 0;
	}

	VError vError = VE_OK;
	if (fCurrentPageChar >= fBuffer.GetLength())	vError = FillBuffer();

	xbox_assert( vError == VE_OK || vError == VE_STREAM_EOF );

	if (vError == VE_OK )	return fBuffer.GetUniChar( fCurrentPageChar + 1 );

	return 0;
}

UniChar VLexerFileInput::MoveToNextChar()
{
	if (fFileStream == NULL) {
		xbox_assert( false );
		return 0;
	}

	VError vError = VE_OK;
	if (fCurrentPageChar >= fBuffer.GetLength())	vError = FillBuffer( );

	xbox_assert( vError == VE_OK );

	if (vError == VE_OK) {
		fCurrentPageChar++;
		fCurrentChar++;

		UniChar uchResult = fBuffer.GetUniChar( fCurrentPageChar );
		if (CHAR_CONTROL_000A == uchResult) {
			// line feed
			fCurrentLine++;
		}

		return uchResult;
	}

	return 0;
}

void VLexerFileInput::MoveToPreviousChar()
{
	if (fFileStream == NULL) {
		xbox_assert( false );
		return;
	}

	fCurrentPageChar--;
	fCurrentChar--;

	xbox_assert( fCurrentPageChar >= 0 );
}

VIndex VLexerFileInput::GetCurrentPosition(const bool& /*inAbsolutePosition*/ /*=true*/)
{
	if (fFileStream != NULL)	return fCurrentChar;

	xbox_assert( false );

	return 0;
}

VError VLexerFileInput::FillBuffer()
{
	xbox_assert( fFileStream != NULL );

	/* Keep the last char of the previous buffer as the first char in the new buffer */
	if (fBuffer.GetLength() > 0 ) {
		fBuffer.SetUniChar( 1, fBuffer.GetUniChar( fBuffer.GetLength() ) );
		fBuffer.Truncate( 1 );
		fCurrentPageChar = 1;
	} else {
		fCurrentPageChar = 0;
	}

	VError vError = fFileStream->GetText( fBuffer, fPageSize, true );
	if (vError == VE_STREAM_EOF) vError = VE_OK;

	return vError;
}

void VLexerFileInput::GetSubString ( sLONG inStart, sLONG inCount, VString &outResult )
{
	if (fFileStream == NULL) {
		xbox_assert( false );
		return;
	}

	/* For now let's look within the current page to avoid storing a copy of the whole file in memory. */
	sLONG nDelta = fCurrentChar - fCurrentPageChar;
	inStart = (inStart <= nDelta) ? 1 : inStart - nDelta + 1;
	if (inStart + inCount > fBuffer. GetLength() + 1 ) {
		inCount = fBuffer.GetLength() - inStart + 1;
	}

	fBuffer.GetSubString( inStart, inCount, outResult );
}

void VLexerFileInput::GetLastCommand( VString & /*outResult*/ )
{
	xbox_assert( false ); // For now let's do nothing.
}

VIndex VLexerFileInput::GetCommandStart()
{
	return fCommandStart;
}

void VLexerFileInput::PrepareForNextCommand()
{
	fCommandStart = fCurrentChar;
}

uLONG VLexerFileInput::GetLastCommandApproximateLength()
{
	return (fCurrentChar >= fCommandStart) ? (fCurrentChar - fCommandStart) : (fCommandStart - fCurrentChar);
}

uLONG VLexerFileInput::FindCommandStart()
{
	uLONG nSkipped = 0;
	UniChar nChar;
	while (HasMoreChars()) {
		nSkipped++;
		nChar = MoveToNextChar();
		if (nChar == CHAR_SEMICOLON)	break;
	}
	fCommandStart = fCurrentChar;

	return nSkipped;
}

VIndex VLexerFileInput::GetCurrentLineNumber()
{
	return fCurrentLine + 1;
}

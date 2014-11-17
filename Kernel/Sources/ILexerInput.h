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
#ifndef _ILEXERINPUT_H_
#define _ILEXERINPUT_H_

BEGIN_TOOLBOX_NAMESPACE

class VString;
class VFileStream;
class VFilePath;

class XTOOLBOX_API ILexerInput {
public:
	virtual ~ILexerInput();

	virtual VIndex GetLength() = 0;
	virtual bool HasMoreChars() = 0;
	virtual UniChar GetCurrentChar() = 0;
	virtual UniChar PeekAtNextChar() = 0;
	virtual UniChar MoveToNextChar() = 0;
	virtual void MoveToPreviousChar() = 0;
	virtual VIndex GetCurrentPosition(const bool& inAbsolutePosition = true) = 0;

	// These functions are only used by the SQL lexer.  However, there
	// is not a clean way to abstact this design due to the way C++ handles
	// multiple inheritence, even with abstract classes.
	virtual void GetLastCommand( VString &outResult ) = 0;
	virtual VIndex GetCommandStart() = 0;
	virtual void PrepareForNextCommand() = 0;
	virtual void GetSubString( sLONG inStart, sLONG inCount, VString &outResult ) = 0;
};

class XTOOLBOX_API VLexerStringInput : public ILexerInput {
public:
	explicit VLexerStringInput();
	virtual ~VLexerStringInput();

	// Implementation of the ISQLLexerInput interface
	virtual VIndex GetLength();
	virtual bool HasMoreChars();
	virtual UniChar GetCurrentChar();
	virtual UniChar PeekAtNextChar();
	virtual UniChar MoveToNextChar();
	virtual void MoveToPreviousChar();
	virtual VIndex GetCurrentPosition(const bool& inAbsolutePosition = true);

	virtual void GetSubString( sLONG inStart, sLONG inCount, VString &outResult );
	virtual void GetLastCommand( VString &outResult );
	virtual VIndex GetCommandStart();
	virtual void PrepareForNextCommand();

	virtual void Init( VString *inStatement );

	VIndex GetCurrentLineNumber();

protected:
	VString *fInput;

	// Contains 1-based index of the last character already read from the input string.
	VIndex fCurrentChar;
	VIndex fCommandStart;
	VIndex fCurrentLine;
	
	bool _IsLineFeed(const UniChar& inChar) const;
	
	std::vector<VIndex> fBookMarkedPositions;
};

class XTOOLBOX_API VLexerFileInput : public ILexerInput {
public:
	explicit VLexerFileInput();
	virtual ~VLexerFileInput();

	// Implementation of the ISQLLexerInput interface
	virtual VIndex GetLength();
	virtual bool HasMoreChars();
	virtual UniChar GetCurrentChar();
	virtual UniChar PeekAtNextChar();
	virtual UniChar MoveToNextChar();
	virtual void MoveToPreviousChar();
	virtual VIndex GetCurrentPosition(const bool& inAbsolutePosition = true);

	virtual void GetSubString( sLONG inStart, sLONG inCount, VString &outResult );
	virtual void GetLastCommand( VString &outResult );
	virtual VIndex GetCommandStart();
	virtual void PrepareForNextCommand();

	uLONG GetLastCommandApproximateLength();
	VIndex GetCurrentLineNumber();

	/* Looks for the first semi-colon and returns the amount of chars skipped. */
	uLONG FindCommandStart();

	virtual VError Init( const VFilePath &inPath );

protected:
	// Contains 1-based index of the last character already read from the input string.
	VIndex fCurrentChar;
	VIndex fCurrentPageChar;
	VIndex fCurrentLine;
	VIndex fCommandStart;

	VFileStream *fFileStream;
	VString fBuffer;
	const VIndex fPageSize;

	VError FillBuffer(); // Loads the next chunk from the file stream
	virtual VError DeInit();
};

END_TOOLBOX_NAMESPACE

#endif // _ILEXERINPUT_H_
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
#include "VXMLPrecompiled.h"
#include "VCSSParser.h"

BEGIN_TOOLBOX_NAMESPACE

const VString kCSS_UNIT_PERCENT			= CVSTR("%");

//predefined colors

const VString kCSS_XML_COLOR_ALICEBLUE				= CVSTR("ALICEBLUE");
const VString kCSS_XML_COLOR_ANTIQUEWHITE			= CVSTR("ANTIQUEWHITE");
const VString kCSS_XML_COLOR_AQUA					= CVSTR("AQUA");
const VString kCSS_XML_COLOR_AQUAMARINE				= CVSTR("AQUAMARINE");
const VString kCSS_XML_COLOR_AZURE					= CVSTR("AZURE");
const VString kCSS_XML_COLOR_BEIGE					= CVSTR("BEIGE");
const VString kCSS_XML_COLOR_BISQUE					= CVSTR("BISQUE");
const VString kCSS_XML_COLOR_BLACK					= CVSTR("BLACK");
const VString kCSS_XML_COLOR_BLANCHEDALMOND			= CVSTR("BLANCHEDALMOND");
const VString kCSS_XML_COLOR_BLUE					= CVSTR("BLUE");
const VString kCSS_XML_COLOR_BLUEVIOLET				= CVSTR("BLUEVIOLET");
const VString kCSS_XML_COLOR_BROWN					= CVSTR("BROWN");
const VString kCSS_XML_COLOR_BURLYWOOD				= CVSTR("BURLYWOOD");
const VString kCSS_XML_COLOR_CADETBLUE				= CVSTR("CADETBLUE");
const VString kCSS_XML_COLOR_CHARTREUSE				= CVSTR("CHARTREUSE");
const VString kCSS_XML_COLOR_CHOCOLATE				= CVSTR("CHOCOLATE");
const VString kCSS_XML_COLOR_CORAL					= CVSTR("CORAL");
const VString kCSS_XML_COLOR_CORNFLOWERBLUE			= CVSTR("CORNFLOWERBLUE");
const VString kCSS_XML_COLOR_CORNSILK				= CVSTR("CORNSILK");
const VString kCSS_XML_COLOR_CRIMSON				= CVSTR("CRIMSON");
const VString kCSS_XML_COLOR_CYAN					= CVSTR("CYAN");
const VString kCSS_XML_COLOR_DARKBLUE				= CVSTR("DARKBLUE");
const VString kCSS_XML_COLOR_DARKCYAN				= CVSTR("DARKCYAN");
const VString kCSS_XML_COLOR_DARKGOLDENROD			= CVSTR("DARKGOLDENROD");
const VString kCSS_XML_COLOR_DARKGRAY				= CVSTR("DARKGRAY");
const VString kCSS_XML_COLOR_DARKGREEN				= CVSTR("DARKGREEN");
const VString kCSS_XML_COLOR_DARKGREY				= CVSTR("DARKGREY");
const VString kCSS_XML_COLOR_DARKKHAKI				= CVSTR("DARKKHAKI");
const VString kCSS_XML_COLOR_DARKMAGENTA			= CVSTR("DARKMAGENTA");
const VString kCSS_XML_COLOR_DARKOLIVEGREEN			= CVSTR("DARKOLIVEGREEN");
const VString kCSS_XML_COLOR_DARKORANGE				= CVSTR("DARKORANGE");
const VString kCSS_XML_COLOR_DARKORCHID				= CVSTR("DARKORCHID");
const VString kCSS_XML_COLOR_DARKRED				= CVSTR("DARKRED");
const VString kCSS_XML_COLOR_DARKSALMON				= CVSTR("DARKSALMON");
const VString kCSS_XML_COLOR_DARKSEAGREEN			= CVSTR("DARKSEAGREEN");
const VString kCSS_XML_COLOR_DARKSLATEBLUE			= CVSTR("DARKSLATEBLUE");
const VString kCSS_XML_COLOR_DARKSLATEGRAY			= CVSTR("DARKSLATEGRAY");
const VString kCSS_XML_COLOR_DARKSLATEGREY			= CVSTR("DARKSLATEGREY");
const VString kCSS_XML_COLOR_DARKTURQUOISE			= CVSTR("DARKTURQUOISE");
const VString kCSS_XML_COLOR_DARKVIOLET				= CVSTR("DARKVIOLET");
const VString kCSS_XML_COLOR_DEEPPINK				= CVSTR("DEEPPINK");
const VString kCSS_XML_COLOR_DEEPSKYBLUE			= CVSTR("DEEPSKYBLUE");
const VString kCSS_XML_COLOR_DIMGRAY				= CVSTR("DIMGRAY");
const VString kCSS_XML_COLOR_DIMGREY				= CVSTR("DIMGREY");
const VString kCSS_XML_COLOR_DODGERBLUE				= CVSTR("DODGERBLUE");
const VString kCSS_XML_COLOR_FIREBRICK				= CVSTR("FIREBRICK");
const VString kCSS_XML_COLOR_FLORALWHITE			= CVSTR("FLORALWHITE");
const VString kCSS_XML_COLOR_FORESTGREEN			= CVSTR("FORESTGREEN");
const VString kCSS_XML_COLOR_FUCHSIA				= CVSTR("FUCHSIA");
const VString kCSS_XML_COLOR_GAINSBORO				= CVSTR("GAINSBORO");
const VString kCSS_XML_COLOR_GHOSTWHITE				= CVSTR("GHOSTWHITE");
const VString kCSS_XML_COLOR_GOLD					= CVSTR("GOLD");
const VString kCSS_XML_COLOR_GOLDENROD				= CVSTR("GOLDENROD");
const VString kCSS_XML_COLOR_GRAY					= CVSTR("GRAY");
const VString kCSS_XML_COLOR_GREY					= CVSTR("GREY");
const VString kCSS_XML_COLOR_GREEN					= CVSTR("GREEN");
const VString kCSS_XML_COLOR_GREENYELLOW			= CVSTR("GREENYELLOW");
const VString kCSS_XML_COLOR_HONEYDEW				= CVSTR("HONEYDEW");
const VString kCSS_XML_COLOR_HOTPINK				= CVSTR("HOTPINK");
const VString kCSS_XML_COLOR_INDIANRED				= CVSTR("INDIANRED");
const VString kCSS_XML_COLOR_INDIGO					= CVSTR("INDIGO");
const VString kCSS_XML_COLOR_IVORY					= CVSTR("IVORY");
const VString kCSS_XML_COLOR_KHAKI					= CVSTR("KHAKI");
const VString kCSS_XML_COLOR_LAVENDER				= CVSTR("LAVENDER");
const VString kCSS_XML_COLOR_LAVENDERBLUSH			= CVSTR("LAVENDERBLUSH");
const VString kCSS_XML_COLOR_LAWNGREEN				= CVSTR("LAWNGREEN");
const VString kCSS_XML_COLOR_LEMONCHIFFON			= CVSTR("LEMONCHIFFON");
const VString kCSS_XML_COLOR_LIGHTBLUE				= CVSTR("LIGHTBLUE");
const VString kCSS_XML_COLOR_LIGHTCORAL				= CVSTR("LIGHTCORAL");
const VString kCSS_XML_COLOR_LIGHTCYAN				= CVSTR("LIGHTCYAN");
const VString kCSS_XML_COLOR_LIGHTGOLDENRODYELLOW	= CVSTR("LIGHTGOLDENRODYELLOW");
const VString kCSS_XML_COLOR_LIGHTGRAY				= CVSTR("LIGHTGRAY");
const VString kCSS_XML_COLOR_LIGHTGREEN				= CVSTR("LIGHTGREEN");
const VString kCSS_XML_COLOR_LIGHTGREY				= CVSTR("LIGHTGREY");
const VString kCSS_XML_COLOR_LIGHTPINK				= CVSTR("LIGHTPINK");
const VString kCSS_XML_COLOR_LIGHTSALMON			= CVSTR("LIGHTSALMON");
const VString kCSS_XML_COLOR_LIGHTSEAGREEN			= CVSTR("LIGHTSEAGREEN");
const VString kCSS_XML_COLOR_LIGHTSKYBLUE			= CVSTR("LIGHTSKYBLUE");
const VString kCSS_XML_COLOR_LIGHTSLATEGRAY			= CVSTR("LIGHTSLATEGRAY");
const VString kCSS_XML_COLOR_LIGHTSLATEGREY			= CVSTR("LIGHTSLATEGREY");
const VString kCSS_XML_COLOR_LIGHTSTEELBLUE			= CVSTR("LIGHTSTEELBLUE");
const VString kCSS_XML_COLOR_LIGHTYELLOW			= CVSTR("LIGHTYELLOW");
const VString kCSS_XML_COLOR_LIME					= CVSTR("LIME");
const VString kCSS_XML_COLOR_LIMEGREEN				= CVSTR("LIMEGREEN");
const VString kCSS_XML_COLOR_LINEN					= CVSTR("LINEN");
const VString kCSS_XML_COLOR_MAGENTA				= CVSTR("MAGENTA");
const VString kCSS_XML_COLOR_MAROON					= CVSTR("MAROON");
const VString kCSS_XML_COLOR_MEDIUMAQUAMARINE		= CVSTR("MEDIUMAQUAMARINE");
const VString kCSS_XML_COLOR_MEDIUMBLUE				= CVSTR("MEDIUMBLUE");
const VString kCSS_XML_COLOR_MEDIUMORCHID			= CVSTR("MEDIUMORCHID");
const VString kCSS_XML_COLOR_MEDIUMPURPLE			= CVSTR("MEDIUMPURPLE");
const VString kCSS_XML_COLOR_MEDIUMSEAGREEN			= CVSTR("MEDIUMSEAGREEN");
const VString kCSS_XML_COLOR_MEDIUMSLATEBLUE		= CVSTR("MEDIUMSLATEBLUE");
const VString kCSS_XML_COLOR_MEDIUMSPRINGGREEN		= CVSTR("MEDIUMSPRINGGREEN");
const VString kCSS_XML_COLOR_MEDIUMTURQUOISE		= CVSTR("MEDIUMTURQUOISE");
const VString kCSS_XML_COLOR_MEDIUMVIOLETRED		= CVSTR("MEDIUMVIOLETRED");
const VString kCSS_XML_COLOR_MIDNIGHTBLUE			= CVSTR("MIDNIGHTBLUE");
const VString kCSS_XML_COLOR_MINTCREAM				= CVSTR("MINTCREAM");
const VString kCSS_XML_COLOR_MISTYROSE				= CVSTR("MISTYROSE");
const VString kCSS_XML_COLOR_MOCCASIN				= CVSTR("MOCCASIN");
const VString kCSS_XML_COLOR_NAVAJOWHITE			= CVSTR("NAVAJOWHITE");
const VString kCSS_XML_COLOR_NAVY					= CVSTR("NAVY");
const VString kCSS_XML_COLOR_OLDLACE				= CVSTR("OLDLACE");
const VString kCSS_XML_COLOR_OLIVE					= CVSTR("OLIVE");
const VString kCSS_XML_COLOR_OLIVEDRAB				= CVSTR("OLIVEDRAB");
const VString kCSS_XML_COLOR_ORANGE					= CVSTR("ORANGE");
const VString kCSS_XML_COLOR_ORANGERED				= CVSTR("ORANGERED");
const VString kCSS_XML_COLOR_ORCHID					= CVSTR("ORCHID");
const VString kCSS_XML_COLOR_PALEGOLDENROD			= CVSTR("PALEGOLDENROD");
const VString kCSS_XML_COLOR_PALEGREEN				= CVSTR("PALEGREEN");
const VString kCSS_XML_COLOR_PALETURQUOISE			= CVSTR("PALETURQUOISE");
const VString kCSS_XML_COLOR_PALEVIOLETRED			= CVSTR("PALEVIOLETRED");
const VString kCSS_XML_COLOR_PAPAYAWHIP				= CVSTR("PAPAYAWHIP");
const VString kCSS_XML_COLOR_PEACHPUFF				= CVSTR("PEACHPUFF");
const VString kCSS_XML_COLOR_PERU					= CVSTR("PERU");
const VString kCSS_XML_COLOR_PINK					= CVSTR("PINK");
const VString kCSS_XML_COLOR_PLUM					= CVSTR("PLUM");
const VString kCSS_XML_COLOR_POWDERBLUE				= CVSTR("POWDERBLUE");
const VString kCSS_XML_COLOR_PURPLE					= CVSTR("PURPLE");
const VString kCSS_XML_COLOR_RED					= CVSTR("RED");
const VString kCSS_XML_COLOR_ROSYBROWN				= CVSTR("ROSYBROWN");
const VString kCSS_XML_COLOR_ROYALBLUE				= CVSTR("ROYALBLUE");
const VString kCSS_XML_COLOR_SADDLEBROWN			= CVSTR("SADDLEBROWN");
const VString kCSS_XML_COLOR_SALMON					= CVSTR("SALMON");
const VString kCSS_XML_COLOR_SANDYBROWN				= CVSTR("SANDYBROWN");
const VString kCSS_XML_COLOR_SEAGREEN				= CVSTR("SEAGREEN");
const VString kCSS_XML_COLOR_SEASHELL				= CVSTR("SEASHELL");
const VString kCSS_XML_COLOR_SIENNA					= CVSTR("SIENNA");
const VString kCSS_XML_COLOR_SILVER					= CVSTR("SILVER");
const VString kCSS_XML_COLOR_SKYBLUE				= CVSTR("SKYBLUE");
const VString kCSS_XML_COLOR_SLATEBLUE				= CVSTR("SLATEBLUE");
const VString kCSS_XML_COLOR_SLATEGRAY				= CVSTR("SLATEGRAY");
const VString kCSS_XML_COLOR_SLATEGREY				= CVSTR("SLATEGREY");
const VString kCSS_XML_COLOR_SNOW					= CVSTR("SNOW");
const VString kCSS_XML_COLOR_SPRINGGREEN			= CVSTR("SPRINGGREEN");
const VString kCSS_XML_COLOR_STEELBLUE				= CVSTR("STEELBLUE");
const VString kCSS_XML_COLOR_TAN					= CVSTR("TAN");
const VString kCSS_XML_COLOR_TEAL					= CVSTR("TEAL");
const VString kCSS_XML_COLOR_THISTLE				= CVSTR("THISTLE");
const VString kCSS_XML_COLOR_TOMATO					= CVSTR("TOMATO");
const VString kCSS_XML_COLOR_TURQUOISE				= CVSTR("TURQUOISE");
const VString kCSS_XML_COLOR_VIOLET					= CVSTR("VIOLET");
const VString kCSS_XML_COLOR_WHEAT					= CVSTR("WHEAT");
const VString kCSS_XML_COLOR_WHITE					= CVSTR("WHITE");
const VString kCSS_XML_COLOR_WHITESMOKE				= CVSTR("WHITESMOKE");
const VString kCSS_XML_COLOR_YELLOW					= CVSTR("YELLOW");
const VString kCSS_XML_COLOR_YELLOWGREEN			= CVSTR("YELLOWGREEN");

END_TOOLBOX_NAMESPACE

USING_TOOLBOX_NAMESPACE

/** start lexical parsing */
void VCSSLexParser::Start(  const UniChar *inSource)
{
	fSource = inSource;

	fLine = 1;
	fColumn = 0;

	fCurTokenLine = 1;
	fCurTokenColumn = 1;

	fEOF = fSource == NULL;
	fCurPos = fSource;

	fCurTokenValue.EnsureSize(2048);
	fCurTokenValue.Clear();
	fCurTokenDumpValue.EnsureSize(2048);
	fCurToken = CSSToken::S;

	fCurTokenValueNumber = 0.0;
	fCurTokenValueIdent.EnsureSize( 16);

	fParsePseudoClass = true;

	_NextChar();
}

/** save parser context */
void VCSSLexParser::SaveContext()
{
	fContext.fCurTokenLine=fCurTokenLine;
	fContext.fCurTokenColumn=fCurTokenColumn;
	fContext.fLine=fLine;
	fContext.fColumn=fColumn;
	fContext.fTabSize=fTabSize;
	fContext.fEOF=fEOF;
	fContext.fCurPos=fCurPos;
	fContext.fCurChar=fCurChar;
	fContext.fCurToken=fCurToken;
	fContext.fCurTokenValue=fCurTokenValue;
	fContext.fCurTokenValueNumber=fCurTokenValueNumber;
	fContext.fCurTokenValueIdent=fCurTokenValueIdent;
	fContext.fCurTokenString1=fCurTokenString1;
	fContext.fCurTokenString2=fCurTokenString2;
	fContext.fCurTokenDumpValue=fCurTokenDumpValue;
	fContext.fHasCurTokenLineEnding=fHasCurTokenLineEnding;
	fContext.fParsePseudoClass=fParsePseudoClass;
}

/** restore parser context */
void VCSSLexParser::RestoreContext()
{
	fCurTokenLine=			fContext.fCurTokenLine;
	fCurTokenColumn=		fContext.fCurTokenColumn;
	fLine=					fContext.fLine;
	fColumn=				fContext.fColumn;
	fTabSize=				fContext.fTabSize;
	fEOF=					fContext.fEOF;
	fCurPos=				fContext.fCurPos;
	fCurChar=				fContext.fCurChar;
	fCurToken=				fContext.fCurToken;
	fCurTokenValue=			fContext.fCurTokenValue;
	fCurTokenValueNumber=	fContext.fCurTokenValueNumber;
	fCurTokenValueIdent=	fContext.fCurTokenValueIdent;
	fCurTokenString1=		fContext.fCurTokenString1;
	fCurTokenString2=		fContext.fCurTokenString2;
	fCurTokenDumpValue=		fContext.fCurTokenDumpValue;
	fHasCurTokenLineEnding=	fContext.fHasCurTokenLineEnding;
	fParsePseudoClass=		fContext.fParsePseudoClass;
}


/** return next character or 0 for EOF 
@remark
	parse any platform line endings and return line endings as a single '\n' character 
*/
UniChar VCSSLexParser::_NextChar()
{
	do
	{
		if (fEOF)
		{
			fCurChar = 0;
			return fCurChar;
		}
		if (*fCurPos == 0)
		{
			fCurChar = 0;
			fEOF = true;
			return fCurChar;
		}
		if (*fCurPos == '\t')
		{
			fColumn += fTabSize;
			break;
		}
		else if (*fCurPos == '\n')
		{
			//Windows or Unix OS line ending: end of line
			fLine++;
			fColumn = 0;
			break;
		}
		else if (*fCurPos == '\r')
		{
			if (*(fCurPos+1) == '\n')
				//Windows OS line endings first char: skip to next char
				fCurPos++;
			else
			{
				//Mac OS line ending: end of line
				fLine++;
				fColumn = 0;
				fCurChar = '\n';
				fCurPos++;
				return fCurChar;
			}
		}
		else
		{
			fColumn++;
			break;
		}
	}	while (true);

	fCurChar = *fCurPos++;
	return fCurChar;
}



/** step to next token and return token	*/
CSSToken::eCSSToken VCSSLexParser::Next(bool inSkipSpacesComment)
{
 do
 {
	 fCurTokenString1 = false;
	 fCurTokenString2 = false;
	 fCurTokenValue.Clear();
	 fCurTokenDumpValue.Clear();
	 if (fCurChar != 0)
		fCurTokenValue.AppendUniChar( fCurChar);
	 fCurTokenLine	= fLine;
	 fCurTokenColumn = fColumn != 0 ? fColumn : 1;
	 if (fCurChar >= '0' && fCurChar <= '9')
	 {
		fCurTokenValue.Clear();
		_parseNumber();
	 }
	 else switch ( fCurChar )
	 {
		case '-':
			{
				_NextChar();
				const UniChar *oldPos = fCurPos;
				UniChar oldChar = fCurChar;
				if( fCurChar == '-' )
				{
				   _NextChar();
				   if( fCurChar == '>' )
				   {
					  _NextChar();
					  fCurToken = CSSToken::CDC;
					  fCurTokenValue = "-->";
					  break;
				   }
				}
				fCurPos = oldPos;
				fCurChar = oldChar;
				if ((fCurChar >= '0' && fCurChar <= '9')
					||
					(fCurChar == '.'))
				{
					fCurTokenValue.Clear();
					fCurTokenValue.AppendUniChar( '-');
					_parseNumber();
				}
				else if (VCSSUtil::isName( fCurChar ))
				{
					//parse ident|function
					while ( VCSSUtil::isName( fCurChar ) )
					{
						if( fCurChar == '\\' )
						{
							fCurTokenValue.AppendUniChar( _ParseEscape());
						}
						else
						{
							fCurTokenValue.AppendUniChar( fCurChar);
							_NextChar();
						}
					}
					if (fCurChar == '(')
					{
						fCurToken = CSSToken::FUNCTION;
						_NextChar();
					}
					else
						fCurToken = CSSToken::IDENT;
				}
				else
					fCurToken = CSSToken::MINUS;
			}
			break;
		case 'u':
		case 'U':
			{
			fCurToken = CSSToken::IDENT;
			_NextChar();
			switch ( fCurChar )
			{
			  case 'r':
			  case 'R':
				 fCurTokenValue.AppendUniChar( fCurChar);
				 _NextChar();
				 switch ( fCurChar )
				 {
					case 'l':
					case 'L':
					   fCurTokenValue.AppendUniChar( fCurChar);
					   _NextChar();
					   switch ( fCurChar )
					   {
						  case '(':
							  {
								 _NextChar();
								 _skipSpaces(true);
								 switch ( fCurChar )
								 {
									case '\'':
									   _parseString('\'');
									   _skipSpaces(true);
									   if( fCurChar == 0 )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   if( fCurChar != ')' )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   _NextChar();
									   fCurToken = CSSToken::URI;
									   break;
									case '"':
									   _parseString('"');
									   _skipSpaces(true);
									   if( fCurChar == 0 )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   if( fCurChar != ')' )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   _NextChar();
									   fCurToken = CSSToken::URI;
									   break;
									case ')':
									   throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									default:
									   {
									   if( !VCSSUtil::isUri( fCurChar ) )
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   fCurTokenValue.Clear();
									   do
									   {
										   fCurTokenValue.AppendUniChar( fCurChar);
										  _NextChar();
									   } while ( VCSSUtil::isUri( fCurChar ) );
									   _skipSpaces(true);
									   if( fCurChar == 0 )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   if( fCurChar != ')' )
									   {
										  throw VCSSException( VE_CSS_LEX_PARSER_BAD_URI, fCurTokenLine, fCurTokenColumn );
									   }
									   _NextChar();
									   fCurToken = CSSToken::URI;
									   }
									   break;
								 }
							 }
						  default:
							 break;
					   }
				 }
			}
			//parse identifier
			if (fCurToken != CSSToken::URI)
			{
				while ( VCSSUtil::isName( fCurChar ) )
				{
					if( fCurChar == '\\' )
					{
						fCurTokenValue.AppendUniChar( _ParseEscape());
					}
					else
					{
						fCurTokenValue.AppendUniChar( fCurChar);
						_NextChar();
					}
				}
				if (fCurChar == '(')
				{
					fCurToken = CSSToken::FUNCTION;
					_NextChar();
				}
				else
					fCurToken = CSSToken::IDENT;
			}
			}
			break;	
		default:
			{
				if( VCSSUtil::isIdentifierStart( fCurChar ) )
				{
					//parse identifier
					fCurTokenValue.Clear();
					do
					{
						if( fCurChar == '\\' )
						{
							fCurTokenValue.AppendUniChar( _ParseEscape());
						}
						else
						{
							fCurTokenValue.AppendUniChar( fCurChar);
							_NextChar();
						}
					} while ( VCSSUtil::isName( fCurChar ) );
					if (fCurChar == '(')
					{
						fCurToken = CSSToken::FUNCTION;
						_NextChar();
					}
					else
						fCurToken = CSSToken::IDENT;
				}
				else
				{
					switch(fCurChar)
					{
						case ' ':
						case '\t':
						case '\n':
						case '\f':
							{
								fCurTokenValue.SetEmpty();
								fHasCurTokenLineEnding = false;
								do
								{
									if (fCurChar == '\n')
									{
										//convert to platform-specific line endings
				#if VERSIONMAC
										fCurTokenValue.AppendUniChar( '\r');
				#else
				#if VERSIONWIN
										fCurTokenValue.AppendUniChar( '\r');
										fCurTokenValue.AppendUniChar( '\n');
				#else
										fCurTokenValue.AppendUniChar( '\n');
				#endif
				#endif
										fHasCurTokenLineEnding = true;
									}
									else
										fCurTokenValue.AppendUniChar( fCurChar);
									_NextChar();
								} while ( VCSSUtil::isSpace( fCurChar) );
								fCurToken = CSSToken::S;
							}
							break;
						case ',':
							{
								_NextChar();
								fCurToken = CSSToken::COMMA;
							}
							break;
						case ';':
							{
								_NextChar();
								fCurToken = CSSToken::SEMI_COLON;
							}
							break;
						case 0:
							fCurToken = CSSToken::END;
							break;
						case '#':
							{
								_NextChar();
								if( VCSSUtil::isName( fCurChar ))
								{
								  fCurToken = CSSToken::HASH;
								  fCurTokenValue.Clear();
								  do
								  {
									 if( fCurChar == '\\' )
									 {
										fCurTokenValue.AppendUniChar( _ParseEscape());
									 }
									 else
									 {
										fCurTokenValue.AppendUniChar( fCurChar);
										_NextChar();
									 }
								  } while ( VCSSUtil::isName( fCurChar ) );
								}
								else
									fCurToken = CSSToken::DELIM;
							}
							break;
						case '"':
							_ParseString1();
							break;
						case '\'':
							_ParseString2();
							break;
						case '+':
							{
								_NextChar();
								if ((fCurChar >= '0' && fCurChar <= '9')
									||
									(fCurChar == '.'))
								{
									fCurTokenValue.Clear();
									fCurTokenValue.AppendUniChar( '+');
									_parseNumber();
								}
								else
									fCurToken = CSSToken::PLUS;
							}
							break;
						case '{':
							{
								_NextChar();
								fCurToken = CSSToken::LEFT_CURLY_BRACE;
							}
							break;
						case '}':
							{
								_NextChar();
								fCurToken = CSSToken::RIGHT_CURLY_BRACE;
							}
							break;
						case '=':
							{
								_NextChar();
								fCurToken = CSSToken::EQUAL;
							}
							break;
						case '>':
							{
								_NextChar();
								fCurToken = CSSToken::GREATER;
							}
							break;
						case '[':
							{
								_NextChar();
								fCurToken = CSSToken::LEFT_BRACKET;
							}
							break;
						case ']':
							{
								_NextChar();
								fCurToken = CSSToken::RIGHT_BRACKET;
							}
							break;
						case '(':
							{
								_NextChar();
								fCurToken = CSSToken::LEFT_BRACE;
							}
							break;
						case ')':
							{
								_NextChar();
								fCurToken = CSSToken::RIGHT_BRACE;
							}
							break;
						case '*':
							{
								_NextChar();
								fCurToken = CSSToken::MUL;
							}
							break;
						case '/':
							{
								_NextChar();
								if( fCurChar != '*' )
								{
									fCurToken = CSSToken::DIV;
								}
								else
								{
									_NextChar();
									_parseComment();
								}
							}
							break;
						case '<':
							{
								_NextChar();
								if( fCurChar == '!' )
								{
								   _NextChar();
								   if( fCurChar == '-' )
								   {
									  _NextChar();
									  if( fCurChar == '-' )
									  {
										 _NextChar();
										 fCurToken = CSSToken::CDO;
										 fCurTokenValue = "<!--";
										 break;
									  }
								   }
								   throw VCSSException( VE_CSS_LEX_PARSER_BAD_CDO, fCurTokenLine, fCurTokenColumn );
								}
								fCurToken = CSSToken::DELIM;
							}
							break;
						case '|':
							{
								_NextChar();
								if( fCurChar == '=' )
								{
								  _NextChar();
								  fCurToken = CSSToken::DASHMATCH;
								  fCurTokenValue = "|=";
								}
								else
									fCurToken = CSSToken::DELIM;
							}
							break;
						case '~':
							{
								_NextChar();
								if( fCurChar == '=' )
								{
								  _NextChar();
								  fCurToken = CSSToken::INCLUDES;
								  fCurTokenValue = "~=";
								}
								else
									fCurToken = CSSToken::DELIM;
							}
							break;
						case '@':
							{
								_NextChar();
								if( VCSSUtil::isIdentifierStart( fCurChar ) )
								{
								  //parse identifier
								  fCurToken = CSSToken::ATKEYWORD;
								  fCurTokenValue.Clear();
								  do
								  {
									 if( fCurChar == '\\' )
									 {
										fCurTokenValue.AppendUniChar( _ParseEscape());
									 }
									 else
									 {
										fCurTokenValue.AppendUniChar( fCurChar);
										_NextChar();
									 }
								  } while ( VCSSUtil::isName( fCurChar ) );
								}
								else
									fCurToken = CSSToken::DELIM;
							}
							break;
						case '.':
							{
								_NextChar();
								if( VCSSUtil::isIdentifierStart( fCurChar ) )
								{
								  //parse identifier
								  fCurToken = CSSToken::CLASS;
								  fCurTokenValue.Clear();
								  do
								  {
									 if( fCurChar == '\\' )
									 {
										fCurTokenValue.AppendUniChar( _ParseEscape());
									 }
									 else
									 {
										fCurTokenValue.AppendUniChar( fCurChar);
										_NextChar();
									 }
								  } while ( VCSSUtil::isName( fCurChar ) );
								}
								else if (fCurChar >= '0' && fCurChar <= '9')
								{
									fCurTokenValue.Clear();
									fCurTokenValue.AppendUniChar( '.');
									_parseNumber();
								}
								else
									fCurToken = CSSToken::DOT;
							}
							break;
						case ':':
							{
								_NextChar();
								if( VCSSUtil::isIdentifierStart( fCurChar ) && fParsePseudoClass)
								{
								  //parse identifier
								  fCurToken = CSSToken::PSEUDO_CLASS;
								  fCurTokenValue.Clear();
								  do
								  {
									 if( fCurChar == '\\' )
									 {
										fCurTokenValue.AppendUniChar( _ParseEscape());
									 }
									 else
									 {
										fCurTokenValue.AppendUniChar( fCurChar);
										_NextChar();
									 }
								  } while ( VCSSUtil::isName( fCurChar ) );
								}
								else
									fCurToken = CSSToken::COLON;
							}
							break;
						case '!':
							{
								const UniChar *oldPos = fCurPos;
								do
								{
								  _NextChar();
								} while ( fCurChar != 0 && VCSSUtil::isSpace( fCurChar ) );
								if(	 VCSSUtil::isEqualNoCase( fCurChar, 'i' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'm' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'p' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'o' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'r' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 't' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'a' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 'n' ) &&
									 VCSSUtil::isEqualNoCase( _NextChar(), 't' ) )
								{
								  _NextChar();
								  fCurToken = CSSToken::IMPORTANT_SYMBOL;
								  fCurTokenValue = "!important";
								  break;
								}

								fCurToken = CSSToken::DELIM;
								fCurPos = oldPos;
								fEOF = false;
								_NextChar();
							}
							break;
						default:
							{
								fCurToken = CSSToken::DELIM;
								_NextChar();
							}
							break;
					}
				}
			}
			break;
	 }
	} while (inSkipSpacesComment && (fCurToken == CSSToken::S || fCurToken == CSSToken::COMMENT));

	return fCurToken;
}


/** return current token */
CSSToken::eCSSToken VCSSLexParser::GetCurToken() const
{
	return fCurToken;
}

/** return current token value 
@remark
	return meaningful string value so for instance:
	for CSSToken::HASH it is the hash name without the # character
	for CSSToken::ATKEYWORD it is the keyword without the @ character
	for CSSToken::STRING it is the string without brackets if any
	etc...
*/
const VString& VCSSLexParser::GetCurTokenValue() const
{
	return fCurTokenValue;
}



/** parse escape sequence */
UniChar VCSSLexParser::_ParseEscape()
{
	xbox_assert(fCurChar == '\\');
	_NextChar();

	UniChar c = 0;
	if( VCSSUtil::isHexadecimal( fCurChar ) )
	{
		c = VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		if( !VCSSUtil::isHexadecimal( fCurChar ) )
		{
			if( VCSSUtil::isSpace( fCurChar ) )
			   _NextChar();
			return c;
		}
		c *= 16;
		c += VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		if( !VCSSUtil::isHexadecimal( fCurChar ) )
		{
			if( VCSSUtil::isSpace( fCurChar ) )
			   _NextChar();
			return c;
		}
		c *= 16;
		c += VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		if( !VCSSUtil::isHexadecimal( fCurChar ) )
		{
			if( VCSSUtil::isSpace( fCurChar ) )
			   _NextChar();
			return c;
		}
		c *= 16;
		c += VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		if( !VCSSUtil::isHexadecimal( fCurChar ) )
		{
			if( VCSSUtil::isSpace( fCurChar ) )
			   _NextChar();
			return c;
		}
		c *= 16;
		c += VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		if( !VCSSUtil::isHexadecimal( fCurChar ) )
		{
			if( VCSSUtil::isSpace( fCurChar ) )
			   _NextChar();
			return c;
		}
		c *= 16;
		c += VCSSUtil::HexaToDecimal( fCurChar);
		_NextChar();
		return c;
	}
	if( ( fCurChar >= ' ' && fCurChar <= '~' ) || fCurChar >= 128 )
	{
		c = fCurChar;
		_NextChar();
		return c;
	}
	throw VCSSException( VE_CSS_LEX_PARSER_BAD_ESCAPE, fCurTokenLine, fCurTokenColumn );
}

/** parse string delimited by inCharBracket character */
void VCSSLexParser::_parseString(UniChar inCharBracket)
{
	xbox_assert(fCurChar == inCharBracket);
	
	fCurToken = CSSToken::STRING;
	if (inCharBracket == '"')
		fCurTokenString1 = true;
	else
		fCurTokenString2 = true;
	fCurTokenValue.Clear();

	_NextChar();
	while (true)
	{
	 switch ( fCurChar )
	 {
		case 0:
			throw VCSSException( VE_CSS_LEX_PARSER_BAD_STRING, fCurTokenLine, fCurTokenColumn );
		case '"':
		case '\'':
			{
				if (fCurChar == inCharBracket)
				{
					_NextChar();
					return;
				}
				fCurTokenValue.AppendUniChar( fCurChar);
				_NextChar();
			}
			break;
		case '\\':
			{
				_NextChar();
				switch ( fCurChar )
				{
					case '\n':
					case '\f':
						_NextChar();
						break;
					default:
						{
							fCurPos--;
							fColumn--;
							fCurChar = *(fCurPos-1);
							fCurTokenValue.AppendUniChar( _ParseEscape());
						}
						break;
				}
			}
			break;
		default:
			{
				if( VCSSUtil::isString( fCurChar ) )
					fCurTokenValue.AppendUniChar( fCurChar);
				else			
					throw VCSSException( VE_CSS_LEX_PARSER_BAD_STRING, fCurTokenLine, fCurTokenColumn );
				_NextChar();
			}
			break;
	 }
	}
}


/** get current token dump value 
@remark 
	return string value as it is parsed so for instance:
	for CSSToken::HASH it is #{name} including # character
	for CSSToken::ATKEYWORD it is @{string} including @ character
	for CSSToken::STRING it is {string} including brackets
*/
const VString& VCSSLexParser::GetCurTokenDumpValue()
{
	if (!fCurTokenDumpValue.IsEmpty())
		return fCurTokenDumpValue;
	
	switch (fCurToken)
	{
		case CSSToken::ATKEYWORD:			//@{ident}  
			{
			fCurTokenDumpValue.AppendUniChar('@');
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			}
			break;
		case CSSToken::STRING:			//{string} 
			{
			if (fCurTokenString1)
			{
				fCurTokenDumpValue.AppendUniChar('"');
				fCurTokenDumpValue.AppendString(fCurTokenValue);
				fCurTokenDumpValue.AppendUniChar('"');
			}
			else
			{
				fCurTokenDumpValue.AppendUniChar('\'');
				fCurTokenDumpValue.AppendString(fCurTokenValue);
				fCurTokenDumpValue.AppendUniChar('\'');
			}
			}
			break;
		case CSSToken::HASH:				//#{name}  
			{
			fCurTokenDumpValue.AppendUniChar('#');
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			}
			break;
		case CSSToken::CLASS:				//.{ident}
			{
			fCurTokenDumpValue.AppendUniChar('.');
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			}
			break;
		case CSSToken::PSEUDO_CLASS:		//:{ident}
			{
			fCurTokenDumpValue.AppendUniChar(':');
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			}
			break;
		case CSSToken::URI:				//url\({w}{string}{w}\) | url\({w}([!#$%&*-~]|{nonascii}|{escape})*{w}\)
			{
			if (fCurTokenString1)
			{
				fCurTokenDumpValue.AppendString("url(\"");
				fCurTokenDumpValue.AppendString(fCurTokenValue);
				fCurTokenDumpValue.AppendString("\")");
			}
			else if (fCurTokenString2)
			{
				fCurTokenDumpValue.AppendString("url('");
				fCurTokenDumpValue.AppendString(fCurTokenValue);
				fCurTokenDumpValue.AppendString("')");
			}
			else
			{
				fCurTokenDumpValue.AppendString("url(");
				fCurTokenDumpValue.AppendString(fCurTokenValue);
				fCurTokenDumpValue.AppendString(")");
			}
			}
			break;
		case CSSToken::FUNCTION:				//{ident}(
			{
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			fCurTokenDumpValue.AppendUniChar('(');
			}
			break;
		case CSSToken::COMMENT:			//\/\*[^*]*\*+([^/][^*]*\*+)*\/  
			{
			fCurTokenDumpValue.AppendString("/*");
			fCurTokenDumpValue.AppendString(fCurTokenValue);
			fCurTokenDumpValue.AppendString("*/");
			}
			break;
		case CSSToken::END:
			break;
		default:
			return fCurTokenValue;
			break;
	}
	return fCurTokenDumpValue;
}


/** parse number */
void VCSSLexParser::_parseNumber()
{
	fCurToken = CSSToken::NUMBER;
	bool isNegative = false;
	bool hasDot = false;
	bool hasExposant = false;
	int exposant = 0;
	bool exposantIsNegative = false;
	if (!fCurTokenValue.IsEmpty())
	{
		isNegative = fCurTokenValue.GetUniChar(1) == '-';
		hasDot = fCurTokenValue.GetUniChar(1) == '.';
	}
	fCurTokenValue.AppendUniChar( fCurChar);
	Real frac = 1;
	if (fCurChar == '.')
	{
		fCurTokenValueNumber = 0.0;
		hasDot = true;
	}
	else
	{
		if (hasDot)
		{
			frac *= 0.1;
			fCurTokenValueNumber = (fCurChar-'0')*frac;
		}
		else
			fCurTokenValueNumber  = fCurChar-'0';
	}
	bool bContinue = true;
	while(bContinue)
	{
		switch( _NextChar())
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
					fCurTokenValue.AppendUniChar( fCurChar);
					if (hasExposant)
						exposant = exposant*10 + (fCurChar-'0');
					else if (hasDot)
					{
						frac *= 0.1;
						fCurTokenValueNumber = fCurTokenValueNumber + (fCurChar-'0')*frac;
					}
					else
						fCurTokenValueNumber  = fCurTokenValueNumber*10 + (fCurChar-'0');
				}
				break;
			case '.':
				{
					if (!hasDot && !hasExposant)
					{
						hasDot = true;
						fCurTokenValue.AppendUniChar( fCurChar);
					}
					else
						bContinue = false;
				}
				break;
			case '%':
				{
					fCurTokenValue.AppendUniChar( fCurChar);
					fCurToken = CSSToken::PERCENTAGE;
					fCurTokenValueIdent = "%";
					_NextChar();
					bContinue = false;
				}
				break;
			case 'e':
			case 'E':
				/** scientific notation */
				if (!hasExposant)
				{
					const UniChar *oldPos = fCurPos;
					UniChar oldChar = fCurChar;
					// WAK0082509
					int oldColumn = fColumn;
					// WAK0082509

					_NextChar();

					if (fCurChar == '+')
					{
						fCurTokenValue.AppendUniChar( oldChar);
						fCurTokenValue.AppendUniChar( fCurChar);
						hasExposant = true;
						break;
					}
					else if (fCurChar == '-')
					{
						fCurTokenValue.AppendUniChar( oldChar);
						fCurTokenValue.AppendUniChar( fCurChar);
						exposantIsNegative = true;
						hasExposant = true;
						break;
					}
					else if (fCurChar >= '0' && fCurChar <= '9')
					{
						fCurTokenValue.AppendUniChar( oldChar);
						fCurTokenValue.AppendUniChar( fCurChar);
						exposant = fCurChar-'0';
						hasExposant = true;
						break;
					}
					else
					{
						fCurPos = oldPos;
						fCurChar = oldChar;
						// WAK0082509
						fColumn = oldColumn;
						// WAK0082509
					}
				}
				/** please do not add a break here ! */
			default:
				if (VCSSUtil::isIdentifierStart( fCurChar))
				{
				  //parse identifier
				  fCurTokenValueIdent.SetEmpty();
				  do
				  {
					 if( fCurChar == '\\' )
					 {
						fCurTokenValueIdent.AppendUniChar( _ParseEscape());
					 }
					 else
					 {
						fCurTokenValueIdent.AppendUniChar( fCurChar);
						_NextChar();
					 }
				  } while ( fCurChar != 0 && VCSSUtil::isName( fCurChar ) );
				  fCurTokenValue.AppendString( fCurTokenValueIdent);
				  fCurToken = CSSToken::DIMENSION;
				}
				bContinue = false;
				break;
		}
	}
	if (hasExposant && exposant)
	{
		if (exposantIsNegative)
			fCurTokenValueNumber *= ::pow(10.0, -exposant);
		else
			fCurTokenValueNumber *= ::pow(10.0, exposant);
	}
	if (isNegative)
		fCurTokenValueNumber = -fCurTokenValueNumber;
}


//** parse comment (current char is char after starting /*)
void VCSSLexParser::_parseComment( bool skip)
{
   // Comment
   if (!skip)
	fCurTokenValue.Clear();
   do
   {
	  while( fCurChar != 0 && fCurChar != '*' )
	  {
		 if (!skip)
			 fCurTokenValue.AppendUniChar( fCurChar);
		 _NextChar();
	  }
	  if (fCurChar == 0)
		  break;
	  do
	  {
		 _NextChar();
	  } while ( fCurChar != 0  && fCurChar == '*' );
   } while ( fCurChar != 0 && fCurChar != '/' );

   if( fCurChar == 0 )
	  throw VCSSException( VE_CSS_LEX_PARSER_BAD_COMMENT, fCurTokenLine, fCurTokenColumn );

   _NextChar();

   if (!skip)
	   fCurToken = CSSToken::COMMENT;
}


/** skip spaces and comment */
void VCSSLexParser::_skipSpaces( bool inSkipComment)
{
   do
   {
	   while (VCSSUtil::isSpace( fCurChar))
		  _NextChar();
	   if (inSkipComment)
		   if (fCurChar == '/')
		   {
			   _NextChar();
			   if (fCurChar == '*')
			   {
				   _NextChar();
				   _parseComment(true);
			   }
			   fCurPos--;
			   fColumn--;
			   fCurChar = *(fCurPos-1);
		   }
   } while (VCSSUtil::isSpace( fCurChar));
}



/** parse CSS document from URL */
void VCSSParser::ParseURL( const VString& inBaseURLPath, const VString& inURLPath, CSSRuleSet::MediaRules *inMediaRules, CSSMedia::Set *inMedias)
{
	VString fullURL;
	VString baseURL;

	//build full url path
	if (inURLPath.FindUniChar( ':') != 0)
		fullURL = inURLPath;
	else
		fullURL = inBaseURLPath+inURLPath;

	//determine base url path
	VIndex last = fullURL.FindUniChar( '/', fullURL.GetLength(), true);
	if (last != 0)
		fullURL.GetSubString(1, last, baseURL);

	VCSSException::SetDocURL( fullURL);

	//build VURL from URL string
	VURL *urlPath	= new VURL( fullURL, true);
	xbox_assert(urlPath);

	//build VFile from VURL
	VFile *file = new VFile( *urlPath);
	delete urlPath;
	xbox_assert(file);
	if (file->Exists())
	{
		bool ioError = false;
		VString textCSS;
		try
		{
			//read from stream as UTF16 (standard unicode) string
			VFileStream stream( file );
			stream.OpenReading();
			stream.GuessCharSetFromLeadingBytes( VTC_UTF_8 );
			stream.SetCarriageReturnMode( eCRM_NATIVE );
			stream.GetText( textCSS);
			stream.CloseReading();
		}
		catch(...)
		{
			ioError = true;
		}
		if (!ioError)
		{
			//finally parse text
			ReleaseRefCountable(&file);
			_Parse( textCSS, baseURL, inMediaRules, inMedias);
			return;
		}
	}
	ReleaseRefCountable(&file);
	throw VCSSException( VE_CSS_PARSER_URL_FAILED);
}


/** parse CSS text */
void VCSSParser::_Parse(  const VString& inSource, const VString& inBaseURLPath, CSSRuleSet::MediaRules *inMediaRules, CSSMedia::Set *inMedias)
{
	_Parse( inSource.GetCPointer(), inBaseURLPath, inMediaRules, inMedias);
}


/** parse CSS document from UniChar * ref */
void VCSSParser::_Parse(  const UniChar *inSource, const VString& inBaseURLPath, CSSRuleSet::MediaRules *inMediaRules, CSSMedia::Set *inMedias)
{
	fBaseURL = inBaseURLPath;
	fLexParser.Start( inSource);

	fMedia.clear();
	fMedia.reserve(10);
	if (inMedias)
		//inherit media from caller
		fMedia.push_back( *inMedias);
	else
	{
		//set default media filter to 'all'
		CSSMedia::Set medias;
		medias.insert( (int)CSSMedia::ALL);
		fMedia.push_back( medias);
	}

	//prepare first <media set,rules> pair
	xbox_assert(inMediaRules);
	fMediaRules = inMediaRules;
	fMediaRules->push_back( CSSRuleSet::MediaRulesPair( fMedia.back(),CSSRuleSet::Rules()));
	fCurRules = &(fMediaRules->back().second);
	fCurRules->reserve(10);
#if VERSIONDEBUG
	try
	{
#endif
		fBlockCounter = 0;
		fHasRules = false;
		_ParseBlock( false);
		if (fCurRules->size() == 0)
			fMediaRules->pop_back();
#if VERSIONDEBUG
	}
	catch( VCSSException e)
	{
		VString message = e.GetErrorPath();
		xbox_assert(e.GetError() != XBOX::VE_OK);
		throw;
	}
#endif

	if (fContinueParseAfterError) {
		// We aggregated all of the errors together from this parsing pass, so now
		// we need to see if any errors actually happened.  If they did, we can throw
		// the aggregate list to the caller.
		if (fExceptionList.HasExceptions()) throw fExceptionList;
	}
}


/** parse block */
void VCSSParser::_ParseBlock(bool bSkip)
{
	fBlockCounter++;
	fLexParser.Next( true);
	do
	{
		try {
			switch( fLexParser.GetCurToken())
			{
			case CSSToken::ATKEYWORD:
				{
				if (!bSkip)
					_ParseAtRule( fLexParser.GetCurTokenValue());
				else 
					fLexParser.Next( true);
				}
				break;
			case CSSToken::LEFT_CURLY_BRACE:
				{
				_ParseBlock(bSkip);
				fLexParser.Next( true);
				}
				break;
			case CSSToken::RIGHT_CURLY_BRACE:
				{
				if (fBlockCounter > 1)
				{
					fBlockCounter--;
					return;
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_BLOCK_END, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				break;
			case CSSToken::SEMI_COLON:
				fLexParser.Next( true);
				break;
			default:
				if (fLexParser.GetCurToken() != CSSToken::END)
				{
					if (bSkip)
						fLexParser.Next( true);
					else
						_ParseRuleSet();
				}
				break;
			}
		} catch (VCSSException err) {
			if (fContinueParseAfterError) {
				// If we were asked to parse the entire document instead of stopping at the first
				// sign of troubles, we need to add this exception to our list
				fExceptionList.AddCSSException( err );
				do 
				{
					//line ending: resume parsing 
					if (fLexParser.HasCurTokenLineEnding())
					{
						fLexParser.Next( true );
						break;
					}

					//starting new block: resume parsing
					if (fLexParser.GetCurToken() == CSSToken::LEFT_CURLY_BRACE)
						break;

					//at-rule: resume parsing
					if (fLexParser.GetCurToken() == CSSToken::ATKEYWORD)
						break;

					//end of current block: finish parsing block
					if (fLexParser.GetCurToken() == CSSToken::RIGHT_CURLY_BRACE 
						&&
						fBlockCounter > 1
						)
					{
						fBlockCounter--;
						return;
					}
					//end of current statement: resume parsing 
					if (fLexParser.GetCurToken() == CSSToken::SEMI_COLON) {
						// We're done with recovering, so nab one more token to put us at the
						// start of the next statement.
						fLexParser.Next( true );
						break;
					}
				} while (fLexParser.Next( false ) != CSSToken::END);
			} else {
				// We are supposed to stop when we hit the first error, so just rethrow the exception
				throw err;
			}
		}
	} while (fLexParser.GetCurToken() != CSSToken::END);
	fBlockCounter--;
}

/** parse at-rule */
void VCSSParser::_ParseAtRule( const VString& inAtName)
{
	if (inAtName.EqualToString("import",false) 
		&& fBlockCounter == 1 && (!fHasRules) //CSS2 compliant: @import at-rule valid only in main block 
											  //				and if no rule declared yet
		) 
																						
	{
		//import rule

		_ParseImportRule();
		return;
	}
	else if (inAtName.EqualToString( "media", false))
	{
		//media rule

		_ParseMediaRule();
		return;
	}

	//skip un-supported or invalid at-rules
	do
	{
		CSSToken::eCSSToken token = fLexParser.Next( true);
		switch( token)
		{
		case CSSToken::SEMI_COLON:
			return;
			break;
		case CSSToken::LEFT_CURLY_BRACE:
			{
			_ParseBlock(true);
			fLexParser.Next( true);
			}
			return;
			break;
		default:
			break;
		}
	} while (fLexParser.GetCurToken() != CSSToken::END);
}


/** parse import rule */
void VCSSParser::_ParseImportRule()
{
	VString url;
	CSSMedia::Set medias;
	bool bMediaTypeIdent = false;
	do
	{
		CSSToken::eCSSToken token = fLexParser.Next( true);
		switch( token)
		{
		case CSSToken::STRING:
			if (url.IsEmpty())
				url = fLexParser.GetCurTokenValue();
			else
				throw VCSSException( VE_CSS_PARSER_BAD_IMPORT_RULE, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			break;
		case CSSToken::URI:
			if (url.IsEmpty())
				url = fLexParser.GetCurTokenValue();
			else
				throw VCSSException( VE_CSS_PARSER_BAD_IMPORT_RULE, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			break;
		case CSSToken::IDENT:
			{
				if (url.IsEmpty())
					throw VCSSException( VE_CSS_PARSER_BAD_IMPORT_RULE, fLexParser.GetCurLine(), fLexParser.GetCurColumn());

				if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kALL, false))
				{
					medias = fMedia.back();
				}
				else if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kSCREEN, false))
				{
					if (_MediaSetCurContains(CSSMedia::SCREEN))
						medias.insert( (int)CSSMedia::SCREEN);
				}
				else if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kPRINT, false))
				{
					if (_MediaSetCurContains(CSSMedia::PRINT))
						medias.insert( (int)CSSMedia::PRINT);
				}
				bMediaTypeIdent = true;
			}
			break;
		case CSSToken::SEMI_COLON:
			{
				if (!bMediaTypeIdent)
					medias = fMedia.back();
				if (!medias.empty())
				{
					VCSSParser *parser = new VCSSParser();
					try
					{
						VString urlDoc = VCSSException::GetDocURL();

						if (fCurRules->size() == 0)
							fMediaRules->pop_back();

						parser->ParseURL( fBaseURL, url, fMediaRules, &medias);

						fMediaRules->push_back( CSSRuleSet::MediaRulesPair( fMedia.back(),CSSRuleSet::Rules()));
						fCurRules = &(fMediaRules->back().second);
						fCurRules->reserve(10);
						
						VCSSException::SetDocURL( urlDoc);
					}
					catch( VCSSException e)
					{
						delete parser; //ensure we release parser if parser throw error
						throw;
					}
					delete parser;
				}
			}
			return;
			break;
		case CSSToken::LEFT_CURLY_BRACE:
			throw VCSSException( VE_CSS_PARSER_BAD_IMPORT_RULE, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			break;
		default:
			break;
		}
	} while (fLexParser.GetCurToken() != CSSToken::END);
}


/** return true if media set contains specified media type */
bool VCSSParser::_MediaSetContains(const CSSMedia::Set& inMediaTypeSet, CSSMedia::eMediaType inMediaType)
{
	return inMediaTypeSet.find( (int)inMediaType) != inMediaTypeSet.end() 
		   || 
		   inMediaTypeSet.find( (int)CSSMedia::ALL) != inMediaTypeSet.end(); 
}

/** return true if fMedia.back() contains specified media type */
bool VCSSParser::_MediaSetCurContains(CSSMedia::eMediaType inMediaType)
{
	return _MediaSetContains(fMedia.back(), inMediaType);
}

/** return true if media set contains CSSMedia::ALL media type */
bool VCSSParser::_MediaSetIsAll(const CSSMedia::Set& inMediaTypeSet)
{
	return inMediaTypeSet.find( (int)CSSMedia::ALL) != inMediaTypeSet.end(); 
}


/** parse media rule */
void VCSSParser::_ParseMediaRule()
{
	CSSMedia::Set medias;
	do
	{
		CSSToken::eCSSToken token = fLexParser.Next( true);
		switch( token)
		{
		case CSSToken::IDENT:
			{
				if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kALL, false))
				{
					medias = fMedia.back();
				}
				if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kSCREEN, false))
				{
					if (_MediaSetCurContains(CSSMedia::SCREEN))
						medias.insert( (int)CSSMedia::SCREEN);
				}
				else if (fLexParser.GetCurTokenValue().EqualToString( CSSMedia::kPRINT, false))
				{
					if (_MediaSetCurContains(CSSMedia::PRINT))
						medias.insert( (int)CSSMedia::PRINT);
				}
			}
			break;
		case CSSToken::SEMI_COLON:
			fLexParser.Next( true);
			return;
			break;
		case CSSToken::LEFT_CURLY_BRACE:
			{
			if (!medias.empty())
			{
				bool bNewMediaSet = medias != fMedia.back();
				if (bNewMediaSet)
				{
					//stack new media set
					fMedia.push_back( medias);
					
					//add new <media set,rules> pair
					if (fCurRules->size() == 0)
						fMediaRules->pop_back();
					fMediaRules->push_back( CSSRuleSet::MediaRulesPair( medias,CSSRuleSet::Rules()));
					fCurRules = &(fMediaRules->back().second);
					fCurRules->reserve(10);
				}

				_ParseBlock(false);

				if (bNewMediaSet)
				{
					//restore previous media set
					fMedia.pop_back();

					//add new <media set,rules> pair
					//@remark
					//	we cannot re-use previous pair because rules must be applied sequentially
					fMediaRules->push_back( CSSRuleSet::MediaRulesPair( fMedia.back(),CSSRuleSet::Rules()));
					fCurRules = &(fMediaRules->back().second);
					fCurRules->reserve(10);
				}
			}
			else //skip block if media is unknown
				_ParseBlock(true);
			fLexParser.Next( true);
			}
			return;
			break;
		default:
			break;
		}
	} while (fLexParser.GetCurToken() != CSSToken::END);
}


/** parse rule-set */
void VCSSParser::_ParseRuleSet()
{
	fCurRule = NULL;
	fCurSelector = NULL;
	fCurSimpleSelector = NULL;
	fCurCombinator = CSSRuleSet::DESCENDANT;
	fCurDeclaration = NULL;
	while (fLexParser.GetCurToken() != CSSToken::END)
	{
		switch( fLexParser.GetCurToken())
		{
		case CSSToken::MUL:
		case CSSToken::IDENT:
		case CSSToken::HASH:
		case CSSToken::CLASS:
		case CSSToken::PSEUDO_CLASS:
		case CSSToken::LEFT_BRACKET:
		case CSSToken::PLUS:
		case CSSToken::GREATER:
			_ParseSelector();
			break;

		case CSSToken::LEFT_CURLY_BRACE:
			{
			_ParseDeclarations();
			fLexParser.Next( true);

			fCurRule = NULL;
			fCurSelector = NULL;
			fCurSimpleSelector = NULL;
			fCurCombinator = CSSRuleSet::DESCENDANT;
			fCurDeclaration = NULL;
			}
			return;
		case CSSToken::COMMA:
			fLexParser.Next( true);
			break;
		case CSSToken::RIGHT_CURLY_BRACE:
		case CSSToken::ATKEYWORD:
			{
			fCurRule = NULL;
			fCurSelector = NULL;
			fCurSimpleSelector = NULL;
			fCurCombinator = CSSRuleSet::DESCENDANT;
			fCurDeclaration = NULL;
			}
			return;
			break;
		case CSSToken::S:
		case CSSToken::COMMENT:
			fLexParser.Next( true);
			break;
		default:
			throw VCSSException( VE_CSS_PARSER_BAD_RULESET, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			break;
		}
	} 
}


/** parse selector */
void VCSSParser::_ParseSelector()
{
	if (fCurRule == NULL)
	{
		//add new rule
		fCurRules->push_back( CSSRuleSet::Rule( CSSRuleSet::Selectors(), CSSRuleSet::Declarations()));
		fCurRule = &(fCurRules->back());
		fCurRule->first.reserve(4);
		fCurRule->second.reserve(10);
	}

	//add new selector
	fCurRule->first.push_back( CSSRuleSet::Selector());
	fCurSelector = &(fCurRule->first.back());
	fCurSelector->reserve(4);
	fCurSimpleSelector = NULL;
	fCurCombinator = CSSRuleSet::DESCENDANT;

	while (fLexParser.GetCurToken() != CSSToken::END)
	{
		switch( fLexParser.GetCurToken())
		{
		case CSSToken::MUL:
		case CSSToken::IDENT:
		case CSSToken::HASH:
		case CSSToken::CLASS:
		case CSSToken::PSEUDO_CLASS:
		case CSSToken::LEFT_BRACKET:
			{
			_ParseSimpleSelector();
			fCurCombinator = CSSRuleSet::DESCENDANT;
			}
			break;
		case CSSToken::PLUS:
			{
			if (fCurSimpleSelector == NULL)
			{
				//add ANY simple selector with current combinator (because 'A + + F' <=> 'A + * + F' or '> A' <=> '* > A')
				fCurSelector->push_back( CSSRuleSet::SimpleSelector( "", CSSRuleSet::CondVector(), fCurCombinator));
			}
			fCurCombinator = CSSRuleSet::ADJACENT;
			fCurSimpleSelector = NULL;
			fLexParser.Next( true);
			}
			break;
		case CSSToken::GREATER:
			{
			if (fCurSimpleSelector == NULL)
			{
				//add ANY simple selector with current combinator (because 'A > > F' <=> 'A > * > F')
				fCurSelector->push_back( CSSRuleSet::SimpleSelector( "", CSSRuleSet::CondVector(), fCurCombinator));
			}
			fCurCombinator = CSSRuleSet::CHILD;
			fCurSimpleSelector = NULL;
			fLexParser.Next( true);
			}
			break;
		case CSSToken::S:
		case CSSToken::COMMENT:
			fLexParser.Next( true);
			break;
		default:
			{
			if (fCurCombinator != CSSRuleSet::DESCENDANT)
			{
				//add ANY simple selector with current combinator (because 'A >' => 'A > *')
				fCurSelector->push_back( CSSRuleSet::SimpleSelector( "", CSSRuleSet::CondVector(), fCurCombinator));
			}
			}
			return;
			break;
		}
	} 
}


/** add condition to current condition list */
void VCSSParser::_AddCondition( const VString& inNameAttribute, const VString& inValue, bool inInclude, bool inBeginsWith, bool inIsClass)
{
	CSSRuleSet::CondVector::iterator it = fCurConditions->begin();
	CSSRuleSet::CondVector::iterator end = fCurConditions->end();
	for (;it != end; it++)
	{
		if ((*it).fName.EqualToString( inNameAttribute, false))
		{
			if (inInclude && (*it).fInclude)
			{
				if (!inValue.IsEmpty())
					(*it).fValues.push_back( inValue);
			}
			else 
			{
				(*it).fValues.clear();
				if (!inValue.IsEmpty())
					(*it).fValues.push_back( inValue);
				(*it).fInclude = inInclude;
				(*it).fBeginsWith = inBeginsWith;
				(*it).fIsClass = inIsClass;
			}
			return;
		}
	}

	std::vector<VString> values;
	values.push_back( inValue);
	fCurConditions->push_back( CSSRuleSet::Condition( inNameAttribute, values, inInclude, inBeginsWith, inIsClass));
}

/** parse simple selector */
void VCSSParser::_ParseSimpleSelector()
{
	//add new simple selector
	fCurSelector->push_back( CSSRuleSet::SimpleSelector( "", CSSRuleSet::CondVector(), fCurCombinator));
	fCurSimpleSelector = &(fCurSelector->back());
	fCurConditions = &(fCurSimpleSelector->fConditions);
	fCurConditions->reserve(4);

	while (fLexParser.GetCurToken() != CSSToken::END)
	{
		switch( fLexParser.GetCurToken())
		{
		case CSSToken::MUL:
			fCurSimpleSelector->fNameElem = "";
			break;
		case CSSToken::IDENT:
			fCurSimpleSelector->fNameElem = fLexParser.GetCurTokenValue();
			break;
		case CSSToken::HASH:
			_AddCondition( "id", fLexParser.GetCurTokenValue());
			fCurSimpleSelector->fHasCondID = true;
			break;
		case CSSToken::CLASS:
			_AddCondition( "class", fLexParser.GetCurTokenValue(), true, false, true);
			break;
		case CSSToken::PSEUDO_CLASS:
			{
			if (fLexParser.GetCurTokenValue().EqualToString( "first-child", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_FIRST_CHILD);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "link", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_LINK);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "visited", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_VISITED);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "active", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_ACTIVE);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "hover", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_HOVER);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "focus", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_FOCUS);
			}
			else if (fLexParser.GetCurTokenValue().EqualToString( "lang", false))
			{
				fCurSimpleSelector->fPseudos.insert((int)CSSRuleSet::Pseudo::PC_LANG);
				fLexParser.Next(false);
				if (fLexParser.GetCurToken() != CSSToken::LEFT_BRACE)
					throw VCSSException( VE_CSS_PARSER_BAD_LANG_PSEUDO_CLASS, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				fLexParser.Next(false);
				if (fLexParser.GetCurToken() != CSSToken::IDENT)
					throw VCSSException( VE_CSS_PARSER_BAD_LANG_PSEUDO_CLASS, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				fCurSimpleSelector->fLangType = fLexParser.GetCurTokenValue();
				fLexParser.Next(false);
				if (fLexParser.GetCurToken() != CSSToken::RIGHT_BRACE)
					throw VCSSException( VE_CSS_PARSER_BAD_LANG_PSEUDO_CLASS, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			}
			}
			break;
		case CSSToken::LEFT_BRACKET:
			_ParseAttributeCondition();
			break;
		case CSSToken::COMMENT:
			//skip comments
			break;
		default:
			return; //close simple selector if any other character including space
			break;
		}
		fLexParser.Next( false); //do not skip spaces because it is used as simple selector separator
	} 
}

/** parse selector attribute condition */
void VCSSParser::_ParseAttributeCondition()
{
	bool bParseValue = false;
	VString name;
	VString value;
	bool include = false;
	bool beginsWith = false;
	fLexParser.Next( true);
	while (fLexParser.GetCurToken() != CSSToken::END)
	{
		switch( fLexParser.GetCurToken())
		{
		case CSSToken::IDENT:
			{
				if (!bParseValue)
				{
					if (name.IsEmpty())
						name = fLexParser.GetCurTokenValue();
					else
						throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				else
				{
					if (value.IsEmpty())
						value = fLexParser.GetCurTokenValue();
					else
						throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
			}
			break;
		case CSSToken::STRING:
			{
				if (bParseValue)
				{
					if (value.IsEmpty())
						value = fLexParser.GetCurTokenValue();
					else
						throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			}
			break;
		case CSSToken::EQUAL:
			{
				if (!bParseValue)
				{
					bParseValue = true;
					include = false;
					beginsWith = false;
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			}
			break;
		case CSSToken::DASHMATCH:
			{
				if (!bParseValue)
				{
					bParseValue = true;
					include = false;
					beginsWith = true;
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			}
			break;
		case CSSToken::INCLUDES:
			{
				if (!bParseValue)
				{
					bParseValue = true;
					include = true;
					beginsWith = false;
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			}
			break;
		case CSSToken::RIGHT_BRACKET:
			{
				if (!name.IsEmpty())
					_AddCondition( name, value, include, beginsWith);
				else
					throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				return;
			}
			break;
		default:
			throw VCSSException( VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
			break;
		}
		fLexParser.Next( true);
	} 
}

/** concat current token dump value with input value */
void VCSSLexParser::ConcatCurTokenDumpValue( VString& ioValue)
{
	if (ioValue.IsEmpty())
		ioValue = GetCurTokenDumpValue();
	else
	{
		if (GetCurToken() == CSSToken::LEFT_BRACE 
			|| 
			GetCurToken() == CSSToken::RIGHT_BRACE)
			ioValue += GetCurTokenDumpValue();
		else if (GetCurToken() != CSSToken::COMMA)
			ioValue += " "+GetCurTokenDumpValue();
		else
			ioValue += GetCurTokenDumpValue();
	}
}

/** concat current token value with input value */
void VCSSParser::_ConcatCurTokenDumpValue( VString& ioValue)
{
	fLexParser.ConcatCurTokenDumpValue( ioValue);
}


//CSS idents
const VString kCSS_IN							= CVSTR("in");
const VString kCSS_CM							= CVSTR("cm");
const VString kCSS_MM							= CVSTR("mm");
const VString kCSS_PT							= CVSTR("pt");
const VString kCSS_PC							= CVSTR("pc");
const VString kCSS_PX							= CVSTR("px");
const VString kCSS_EM							= CVSTR("em");
const VString kCSS_EX							= CVSTR("ex");
const VString kCSS_PERCENT						= CVSTR("%");

const VString kCSS_RGB							= CVSTR("rgb");	

const VString kCSS_INHERIT						= CVSTR("inherit");	
const VString kCSS_LENGTH_AUTO					= CVSTR("auto");
const VString kCSS_LENGTH_NORMAL				= CVSTR("normal");
const VString kCSS_TRANSPARENT					= CVSTR("transparent");	
const VString kCSS_INVERT						= CVSTR("invert");	

const VString kCSS_VISIBLE						= CVSTR("visible");
const VString kCSS_HIDDEN						= CVSTR("hidden");
const VString kCSS_COLLAPSE						= CVSTR("collapse");

const VString kCSS_NONE							= CVSTR("none");
const VString kCSS_CURRENT_COLOR				= CVSTR("currentColor");	

const VString kCSS_DEG							= CVSTR("deg");
const VString kCSS_RAD							= CVSTR("rad");
const VString kCSS_GRAD							= CVSTR("grad");

const VString kCSS_ANGLE_AUTO					= CVSTR("auto");

const VString kCSS_FILLRULE_NONZERO				= CVSTR("nonzero");	
const VString kCSS_FILLRULE_EVENODD				= CVSTR("evenodd");	

const VString kCSS_STROKE_LINE_CAP_BUTT			= CVSTR("butt");	
const VString kCSS_STROKE_LINE_CAP_ROUND		= CVSTR("round");	
const VString kCSS_STROKE_LINE_CAP_SQUARE		= CVSTR("square");	

const VString kCSS_STROKE_LINE_JOIN_MITER		= CVSTR("miter");	
const VString kCSS_STROKE_LINE_JOIN_ROUND		= CVSTR("round");	
const VString kCSS_STROKE_LINE_JOIN_BEVEL		= CVSTR("bevel");	

const VString kCSS_FONT_STYLE_NORMAL			= CVSTR("normal");	
const VString kCSS_FONT_STYLE_ITALIC			= CVSTR("italic");	
const VString kCSS_FONT_STYLE_OBLIQUE			= CVSTR("oblique");	

const VString kCSS_FONT_STYLE_UNDERLINE			= CVSTR("underline");	
const VString kCSS_FONT_STYLE_OVERLINE	 		= CVSTR("overline");	
const VString kCSS_FONT_STYLE_LINE_THROUGH		= CVSTR("line-through");	
const VString kCSS_FONT_STYLE_BLINK				= CVSTR("blink");	

const VString kCSS_FONT_WEIGHT_NORMAL			= CVSTR("normal");	
const VString kCSS_FONT_WEIGHT_BOLD		 		= CVSTR("bold");	
const VString kCSS_FONT_WEIGHT_BOLDER			= CVSTR("bolder");	
const VString kCSS_FONT_WEIGHT_LIGHTER			= CVSTR("lighter");	

const VString kCSS_FONT_SIZE_XX_SMALL			= CVSTR("xx-small");	
const VString kCSS_FONT_SIZE_X_SMALL			= CVSTR("x-small");	
const VString kCSS_FONT_SIZE_SMALL		 		= CVSTR("small");	
const VString kCSS_FONT_SIZE_MEDIUM		 		= CVSTR("medium");	
const VString kCSS_FONT_SIZE_LARGE				= CVSTR("large");	
const VString kCSS_FONT_SIZE_X_LARGE			= CVSTR("x-large");	
const VString kCSS_FONT_SIZE_XX_LARGE			= CVSTR("xx-large");

const VString kCSS_FONT_SIZE_LARGER				= CVSTR("larger");	
const VString kCSS_FONT_SIZE_SMALLER			= CVSTR("smaller");	

const VString kCSS_TEXTPROP_ANCHOR_START		= CVSTR("start");	
const VString kCSS_TEXTPROP_ANCHOR_MIDDLE		= CVSTR("middle");	
const VString kCSS_TEXTPROP_ANCHOR_END			= CVSTR("end");	

const VString kCSS_DIRECTION_LTR				= CVSTR("ltr");	
const VString kCSS_DIRECTION_RTL				= CVSTR("rtl");	

const VString kCSS_WRITING_MODE_LR				= CVSTR("lr");	
const VString kCSS_WRITING_MODE_LR_TB			= CVSTR("lr-tb");	
const VString kCSS_WRITING_MODE_RL				= CVSTR("rl");	
const VString kCSS_WRITING_MODE_RL_TB			= CVSTR("rl-tb");	
const VString kCSS_WRITING_MODE_TB				= CVSTR("tb");	
const VString kCSS_WRITING_MODE_TB_RL			= CVSTR("tb-rl");	

const VString kCSS_TEXT_ALIGN_START				= CVSTR("start");	
const VString kCSS_TEXT_ALIGN_CENTER			= CVSTR("center");	
const VString kCSS_TEXT_ALIGN_END				= CVSTR("end");	
const VString kCSS_TEXT_ALIGN_JUSTIFY			= CVSTR("justify");	
const VString kCSS_TEXT_ALIGN_LEFT				= CVSTR("left");	
const VString kCSS_TEXT_ALIGN_RIGHT				= CVSTR("right");	

const VString kCSS_TEXT_ALIGN_AUTO				= CVSTR("auto");	
const VString kCSS_TEXT_ALIGN_BEFORE			= CVSTR("before");	
const VString kCSS_TEXT_ALIGN_AFTER				= CVSTR("after");	

const VString kCSS_VERTICAL_ALIGN_BASELINE		= CVSTR("baseline");	
const VString kCSS_VERTICAL_ALIGN_SUB			= CVSTR("sub");	
const VString kCSS_VERTICAL_ALIGN_SUPER			= CVSTR("super");	
const VString kCSS_VERTICAL_ALIGN_TOP			= CVSTR("top");	
const VString kCSS_VERTICAL_ALIGN_TEXT_TOP		= CVSTR("text-top");	
const VString kCSS_VERTICAL_ALIGN_MIDDLE		= CVSTR("middle");	
const VString kCSS_VERTICAL_ALIGN_CENTER		= CVSTR("center");	
const VString kCSS_VERTICAL_ALIGN_BOTTOM		= CVSTR("bottom");	
const VString kCSS_VERTICAL_ALIGN_TEXT_BOTTOM	= CVSTR("text-bottom");	

const VString kCSS_BACKGROUND_REPEAT			= CVSTR("repeat");	
const VString kCSS_BACKGROUND_REPEAT_X			= CVSTR("repeat-x");	
const VString kCSS_BACKGROUND_REPEAT_Y			= CVSTR("repeat-y");	
const VString kCSS_BACKGROUND_NO_REPEAT			= CVSTR("no-repeat");	

const VString kCSS_BACKGROUND_BORDER_BOX		= CVSTR("border-box");	
const VString kCSS_BACKGROUND_PADDING_BOX		= CVSTR("padding-box");	
const VString kCSS_BACKGROUND_CONTENT_BOX		= CVSTR("content-box");	

const VString kCSS_BACKGROUND_SIZE_COVER		= CVSTR("cover");	
const VString kCSS_BACKGROUND_SIZE_CONTAIN		= CVSTR("contain");	


const VString kCSS_BORDER_STYLE_NONE			= CVSTR("none"); 	
const VString kCSS_BORDER_STYLE_HIDDEN			= CVSTR("hidden");
const VString kCSS_BORDER_STYLE_DOTTED			= CVSTR("dotted");
const VString kCSS_BORDER_STYLE_DASHED			= CVSTR("dashed");
const VString kCSS_BORDER_STYLE_SOLID			= CVSTR("solid");
const VString kCSS_BORDER_STYLE_DOUBLE			= CVSTR("double");
const VString kCSS_BORDER_STYLE_GROOVE			= CVSTR("groove");
const VString kCSS_BORDER_STYLE_RIDGE			= CVSTR("ridge");
const VString kCSS_BORDER_STYLE_INSET			= CVSTR("inset");
const VString kCSS_BORDER_STYLE_OUTSET			= CVSTR("outset");

const VString kCSS_TEXT_BORDER_WIDTH_THIN		= CVSTR("thin"); 	
const VString kCSS_TEXT_BORDER_WIDTH_MEDIUM		= CVSTR("medium");
const VString kCSS_TEXT_BORDER_WIDTH_THICK		= CVSTR("thick");

const VString kCSS_RENDER_AUTO					= CVSTR("auto");	
const VString kCSS_RENDER_OPTIMIZE_SPEED		= CVSTR("optimizeSpeed");	
const VString kCSS_RENDER_CRISP_EDGES			= CVSTR("crispEdges");	
const VString kCSS_RENDER_GEOMETRIC_PRECISION	= CVSTR("geometricPrecision");	
const VString kCSS_RENDER_OPTIMIZE_LEGIBILITY	= CVSTR("optimizeLegibility");	

const VString kCSS_TAB_STOP_TYPE_LEFT			= CVSTR("left");	
const VString kCSS_TAB_STOP_TYPE_CENTER			= CVSTR("center");	
const VString kCSS_TAB_STOP_TYPE_RIGHT			= CVSTR("right");	
const VString kCSS_TAB_STOP_TYPE_DECIMAL		= CVSTR("decimal");	
const VString kCSS_TAB_STOP_TYPE_BAR			= CVSTR("bar");	

const VString kCSS_LIST_STYLE_TYPE_DISC						= CVSTR("disc");	
const VString kCSS_LIST_STYLE_TYPE_CIRCLE					= CVSTR("circle");	
const VString kCSS_LIST_STYLE_TYPE_SQUARE					= CVSTR("square");	
const VString kCSS_LIST_STYLE_TYPE_HOLLOW_SQUARE			= CVSTR("hollow-square");	
const VString kCSS_LIST_STYLE_TYPE_DIAMOND					= CVSTR("diamond");	
const VString kCSS_LIST_STYLE_TYPE_CLUB						= CVSTR("club");	
const VString kCSS_LIST_STYLE_TYPE_CUSTOM					= CVSTR("custom");	
const VString kCSS_LIST_STYLE_TYPE_DECIMAL					= CVSTR("decimal");	
const VString kCSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO		= CVSTR("decimal-leading-zero");	
const VString kCSS_LIST_STYLE_TYPE_LOWER_LATIN				= CVSTR("lower-latin");	
const VString kCSS_LIST_STYLE_TYPE_LOWER_ALPHA				= CVSTR("lower-alpha");	
const VString kCSS_LIST_STYLE_TYPE_LOWER_ROMAN				= CVSTR("lower-roman");	
const VString kCSS_LIST_STYLE_TYPE_LOWER_GREEK				= CVSTR("lower-greek");	
const VString kCSS_LIST_STYLE_TYPE_UPPER_LATIN				= CVSTR("upper-latin");	
const VString kCSS_LIST_STYLE_TYPE_UPPER_ALPHA				= CVSTR("upper-alpha");	
const VString kCSS_LIST_STYLE_TYPE_UPPER_ROMAN				= CVSTR("upper-roman");	
const VString kCSS_LIST_STYLE_TYPE_ARMENIAN					= CVSTR("armenian");	
const VString kCSS_LIST_STYLE_TYPE_GEORGIAN					= CVSTR("georgian");	
const VString kCSS_LIST_STYLE_TYPE_HEBREW					= CVSTR("hebrew");	
const VString kCSS_LIST_STYLE_TYPE_HIRAGANA					= CVSTR("hiragana");	
const VString kCSS_LIST_STYLE_TYPE_KATAKANA					= CVSTR("katakana");	
const VString kCSS_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC			= CVSTR("cjk-ideographic");	
const VString kCSS_LIST_STYLE_TYPE_DECIMAL_GREEK			= CVSTR("decimal-greek");	


/** parse SVG URI */
bool VCSSParser::ParseSVGURI( VCSSLexParser *inLexParser, VString& outURI, VString *ioValue)
{
	if (inLexParser->GetCurToken() == CSSToken::URI)
	{
		//extract URI reference from URL reference (ie url(<URI reference>)')

		outURI = inLexParser->GetCurTokenValue();
		if (outURI.GetLength() > 0
			&&
			outURI.GetUniChar(1) == '#')
			outURI.SubString( 2, outURI.GetLength()-1);
		if (ioValue)
			inLexParser->ConcatCurTokenDumpValue( *ioValue);
		return true;
	}
	else if (inLexParser->GetCurToken() == CSSToken::HASH)
	{
		outURI = inLexParser->GetCurTokenValue();
		if (outURI.EqualToString(CVSTR("xpointer"), false))
		{
			//extract long URI reference (ie '#xpointer(id(<elementID>))')

			inLexParser->SaveContext();
			if (ioValue)
				inLexParser->ConcatCurTokenDumpValue( *ioValue);
			inLexParser->Next( true);
			if (inLexParser->GetCurToken() == CSSToken::LEFT_BRACE)
			{
				if (ioValue)
					inLexParser->ConcatCurTokenDumpValue( *ioValue);
				inLexParser->Next( true);
				if (inLexParser->GetCurToken() == CSSToken::FUNCTION
					&&
					inLexParser->GetCurTokenValue().EqualToString( "id", false))
				{
					if (ioValue)
						inLexParser->ConcatCurTokenDumpValue( *ioValue);
					inLexParser->Next( true);
					if (inLexParser->GetCurToken() == CSSToken::IDENT)
					{
						outURI = inLexParser->GetCurTokenValue();
						if (ioValue)
							inLexParser->ConcatCurTokenDumpValue( *ioValue);
						inLexParser->Next( true);
						if (inLexParser->GetCurToken() == CSSToken::RIGHT_BRACE)
						{
							if (ioValue)
								inLexParser->ConcatCurTokenDumpValue( *ioValue);
							inLexParser->Next( true);
							if (inLexParser->GetCurToken() == CSSToken::RIGHT_BRACE)
							{
								if (ioValue)
									inLexParser->ConcatCurTokenDumpValue( *ioValue);
								return true;
							}
						}
					}
				}
			}
			else 
			{
				//extract short URI reference (ie '#<elementID>')
				inLexParser->RestoreContext();
				return true;
			}
			if (ioValue)
				ioValue->SetEmpty();
			inLexParser->RestoreContext();
			return false;
		}
		else
		{
			//extract short URI reference (ie '#<elementID>')
			if (ioValue)
				inLexParser->ConcatCurTokenDumpValue( *ioValue);
			return true;
		}
	}
	else
		return false;
}

/** parse color as RGB function */
bool VCSSParser::ParseRGBFunc( VCSSLexParser *inLexParser, uLONG& outColor, VString* ioValue)
{
	if (!inLexParser->GetCurTokenValue().EqualToString( kCSS_RGB, false))
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);
	uLONG red, green, blue;

	/** parse red color */
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() == CSSToken::NUMBER)
		red = (uLONG)inLexParser->GetCurTokenValueNumber();
	else if (inLexParser->GetCurToken() == CSSToken::PERCENTAGE)
		red = (uLONG)(inLexParser->GetCurTokenValueNumber()*(255.0/100.0));
	else
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() != CSSToken::COMMA)
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);

	/** parse green color */
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() == CSSToken::NUMBER)
		green = (uLONG)inLexParser->GetCurTokenValueNumber();
	else if (inLexParser->GetCurToken() == CSSToken::PERCENTAGE)
		green = (uLONG)(inLexParser->GetCurTokenValueNumber()*(255.0/100.0));
	else
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() != CSSToken::COMMA)
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);

	/** parse blue color */
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() == CSSToken::NUMBER)
		blue = (uLONG)inLexParser->GetCurTokenValueNumber();
	else if (inLexParser->GetCurToken() == CSSToken::PERCENTAGE)
		blue = (uLONG)(inLexParser->GetCurTokenValueNumber()*(255.0/100.0));
	else
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);
	inLexParser->Next( true);
	if (inLexParser->GetCurToken() != CSSToken::RIGHT_BRACE)
		return false;
	if (ioValue)
		inLexParser->ConcatCurTokenDumpValue( *ioValue);
	outColor = CSSMakeARGB( red, green, blue);
	return true;
}


/** parse value according to the specified value type */ 
CSSProperty::Value *VCSSParser::ParseValue( VCSSLexParser *inLexParser, CSSProperty::eType inType, VString* ioValue, bool inIgnoreSVG, bool inParseFirstTokenOnly)
{
	CSSProperty::Value *value = new CSSProperty::Value();
	value->fType = inType;
	value->fInherit = false;

	if (ioValue)
		ioValue->SetEmpty();
	bool done = false;
	if (!inIgnoreSVG)
		switch( inType)
		{
		case CSSProperty::SVGLENGTH:
		case CSSProperty::SVGCOORD:
		case CSSProperty::SVGFONTSIZE:
		{
			bool isLength = inType != CSSProperty::SVGCOORD;
			bool isFontSize = inType == CSSProperty::SVGFONTSIZE;

			value->v.svg.fLength.fAuto = false;
			value->v.svg.fLength.fNumber = 0.0;
			value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PX;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::NUMBER:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::PERCENTAGE:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber()*0.01;
						value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PERCENT;
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::DIMENSION:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;

						if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PT, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_IN, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_IN;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_CM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_CM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_MM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_MM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PC, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PC;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PERCENT, false))
						{
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PERCENT;
							value->v.svg.fLength.fNumber *= 0.01;
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}

						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::IDENT:
					{
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
						value->fInherit = true;
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LENGTH_AUTO, false))
					{
						if (!isFontSize)
							value->v.svg.fLength.fAuto = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
					}
					else if (isFontSize)
					{
						value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_MEDIUM, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_MEDIUM;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_XX_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_XX_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_X_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_X_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_X_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_X_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_XX_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_XX_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_LARGER, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_FONTSIZE_LARGER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_SMALLER, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_FONTSIZE_SMALLER;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}

						if (value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
					}
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}

					done = true;
					if (ioValue)
						*ioValue = inLexParser->GetCurTokenValue();
					}
					break;
				case CSSToken::COMMA:
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::SVGNUMBER:
		case CSSProperty::SVGOPACITY:
		case CSSProperty::SVGOFFSET:
		case CSSProperty::SVGANGLE:
			{
			value->v.svg.fNumber.fAuto = false;
			value->v.svg.fNumber.fNumber = 0.0;

			bool isAngle = inType == CSSProperty::SVGANGLE;
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::NUMBER:
				case CSSToken::PERCENTAGE:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurToken() == CSSToken::PERCENTAGE)
							value->v.svg.fNumber.fNumber = inLexParser->GetCurTokenValueNumber()*0.01;
						else
							value->v.svg.fNumber.fNumber = inLexParser->GetCurTokenValueNumber();

						if (inType == CSSProperty::SVGOPACITY || inType == CSSProperty::SVGOFFSET)
						{
							if (value->v.svg.fNumber.fNumber < 0.0)
								value->v.svg.fNumber.fNumber = 0.0;
							else if (value->v.svg.fNumber.fNumber > 1.0)
								value->v.svg.fNumber.fNumber = 1.0;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::DIMENSION:
					if (done || (!isAngle))
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();

						if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_RAD, false))
							value->v.svg.fLength.fNumber = value->v.svg.fLength.fNumber*180.0/XBOX::PI;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_GRAD, false))
							value->v.svg.fLength.fNumber = value->v.svg.fLength.fNumber*360.0/400.0;
						else if (!inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_DEG, false))
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
					{
						value->fInherit = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					else if (isAngle
							 &&
							 inLexParser->GetCurTokenValue().EqualToString( kCSS_ANGLE_AUTO, false))
					{
						value->v.svg.fNumber.fAuto = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::SVGPAINT:
		case CSSProperty::SVGCOLOR:
			{
			bool isSVGPAINT = inType == CSSProperty::SVGPAINT;

			//reset to BLACK
			value->v.svg.fPaint.fURI = NULL;
			value->v.svg.fPaint.fNone = false;
			value->v.svg.fPaint.fCurrentColor = false;
			value->v.svg.fPaint.fColor = 0;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::HASH:
					if (!done)
					{
						//check first if it is a hexadecimal color
						if (VCSSUtil::ParseColorHexa( inLexParser->GetCurTokenValue(), &value->v.svg.fPaint.fColor))
						{
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenDumpValue();
							done = true;
						}
						else if (isSVGPAINT)
						{
							//otherwise it is probably a URI
							value->v.svg.fPaint.fURI = new VString();
							if (VCSSParser::ParseSVGURI( inLexParser, *(value->v.svg.fPaint.fURI), ioValue))
								done = true;
							else
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
					}
					break;
				case CSSToken::IDENT:
					if (!done)
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
						{
							value->fInherit = true;
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
							done = true;
							break;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_NONE, false))
						{
							value->v.svg.fPaint.fNone = true;
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
							done = true;
							break;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_CURRENT_COLOR, false))
						{
							value->v.svg.fPaint.fCurrentColor = true;
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
							done = true;
							break;
						}
						else if (VCSSUtil::ParseColorIdent( inLexParser->GetCurTokenValue(), &value->v.svg.fPaint.fColor))
						{
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
							done = true;
							break;
						}
						else 
						{
							value->v.svg.fPaint.fURI = new VString( inLexParser->GetCurTokenValue());
							done = true;
							break;
						}
					}
					break;
				case CSSToken::FUNCTION:
					/** parse 'rgb(red,green,blue)' color */
					{
						uLONG color = 0;
						if (!VCSSParser::ParseRGBFunc( inLexParser, color, done ? NULL : ioValue))
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (!done)
							value->v.svg.fPaint.fColor = color;
						done = true;
					}
					break;
				case CSSToken::URI:
					if (!isSVGPAINT)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else 
					{
						bool success;
						if (done)
						{
							VString dummy;
							success = VCSSParser::ParseSVGURI( inLexParser, dummy, NULL);
						}
						else
						{
							value->v.svg.fPaint.fURI = new VString();
							success = VCSSParser::ParseSVGURI( inLexParser, *(value->v.svg.fPaint.fURI), ioValue);
						}
						if (success)
							done = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::SVGURI:
			{
			value->v.svg.fURI.fNone = false;
			value->v.svg.fURI.fURI = NULL;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
					{
						value->fInherit = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_NONE, false))
					{
						value->v.svg.fURI.fNone = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else 
					{
						value->v.svg.fURI.fURI = new VString( inLexParser->GetCurTokenValue());
						done = true;
					}
					break;
				case CSSToken::URI:
				case CSSToken::HASH:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.svg.fURI.fURI = new VString();
						if (VCSSParser::ParseSVGURI( inLexParser, *(value->v.svg.fURI.fURI), ioValue))
							done = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::SVGTEXTANCHOR:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXTPROP_ANCHOR_START, false))
							value->v.svg.fTextAnchor = CSSProperty::kTEXT_ANCHOR_START;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXTPROP_ANCHOR_MIDDLE, false))
							value->v.svg.fTextAnchor = CSSProperty::kTEXT_ANCHOR_MIDDLE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXTPROP_ANCHOR_END, false))
							value->v.svg.fTextAnchor = CSSProperty::kTEXT_ANCHOR_END;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGTEXTALIGN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_AUTO, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_START;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_START, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_START;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_CENTER, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_CENTER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_END, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_END;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_JUSTIFY, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_JUSTIFY;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_LEFT, false)) //for compat with CSS style
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_START; 
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_RIGHT, false)) //for compat with CSS style
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_END;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value); 
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::DISPLAYALIGN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_AUTO, false))
							value->v.css.fDisplayAlign = CSSProperty::kTEXT_ALIGN_START;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_BEFORE, false))
							value->v.css.fDisplayAlign = CSSProperty::kTEXT_ALIGN_START;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_CENTER, false))
							value->v.css.fDisplayAlign = CSSProperty::kTEXT_ALIGN_CENTER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_AFTER, false))
							value->v.css.fDisplayAlign = CSSProperty::kTEXT_ALIGN_END;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGFILLRULE:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FILLRULE_NONZERO, false))
							value->v.svg.fFillRuleEvenOdd = false;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FILLRULE_EVENODD, false))
							value->v.svg.fFillRuleEvenOdd = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGSTROKELINECAP:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_CAP_BUTT, false))
							value->v.svg.fStrokeLineCap = CSSProperty::kStrokeLineCapButt;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_CAP_ROUND, false))
							value->v.svg.fStrokeLineCap = CSSProperty::kStrokeLineCapRound;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_CAP_SQUARE, false))
							value->v.svg.fStrokeLineCap = CSSProperty::kStrokeLineCapSquare;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGSTROKELINEJOIN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_JOIN_MITER, false))
							value->v.svg.fStrokeLineJoin = CSSProperty::kStrokeLineJoinMiter;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_JOIN_ROUND, false))
							value->v.svg.fStrokeLineJoin = CSSProperty::kStrokeLineJoinRound;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_STROKE_LINE_JOIN_BEVEL, false))
							value->v.svg.fStrokeLineJoin = CSSProperty::kStrokeLineJoinBevel;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGTEXTRENDERING:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_AUTO, false))
							value->v.svg.fTextRenderingMode = CSSProperty::kTRM_AUTO;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_OPTIMIZE_SPEED, false))
							value->v.svg.fTextRenderingMode = CSSProperty::kTRM_OPTIMIZE_SPEED;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_OPTIMIZE_LEGIBILITY, false))
							value->v.svg.fTextRenderingMode = CSSProperty::kTRM_OPTIMIZE_LEGIBILITY;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_GEOMETRIC_PRECISION, false))
							value->v.svg.fTextRenderingMode = CSSProperty::kTRM_GEOMETRIC_PRECISION;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::SVGSHAPERENDERING:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_AUTO, false))
							value->v.svg.fShapeRenderingMode = CSSProperty::kSRM_AUTO;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_OPTIMIZE_SPEED, false))
							value->v.svg.fShapeRenderingMode = CSSProperty::kSRM_OPTIMIZE_SPEED;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_CRISP_EDGES, false))
							value->v.svg.fShapeRenderingMode = CSSProperty::kSRM_CRISP_EDGES;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_RENDER_GEOMETRIC_PRECISION, false))
							value->v.svg.fShapeRenderingMode = CSSProperty::kSRM_GEOMETRIC_PRECISION;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				inLexParser->Next( true); 
			} 
			break;

		default:
			break;
		}

	if (!done)
		switch( inType)
		{
		case CSSProperty::LENGTH:
		case CSSProperty::COORD:
		case CSSProperty::FONTSIZE:
		case CSSProperty::LINEHEIGHT:
		{
			//@remark: note that because of union v.svg.fLength = v.css.fLength
			//		   so we can factorize code for CSS2 & SVG LENGTH+COORD+FONTSIZE types

			bool isLength = inType != CSSProperty::COORD;
			bool isFontSize = inType == CSSProperty::FONTSIZE;
			bool isLineHeight = inType == CSSProperty::LINEHEIGHT;

			value->v.svg.fLength.fAuto = false;
			value->v.svg.fLength.fNumber = 0.0;
			value->v.svg.fLength.fUnit = isLineHeight ?  CSSProperty::LENGTH_TYPE_PERCENT : CSSProperty::LENGTH_TYPE_PX;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::NUMBER:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::PERCENTAGE:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber()*0.01;
						value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PERCENT;
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::DIMENSION:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (isLength && value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;

						if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PT, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_IN, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_IN;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_CM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_CM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_MM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_MM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PC, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PC;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PERCENT, false))
						{
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PERCENT;
							value->v.svg.fLength.fNumber *= 0.01;
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}

						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::IDENT:
					{
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
						value->fInherit = true;
					else if (!isFontSize && !isLineHeight && inLexParser->GetCurTokenValue().EqualToString( kCSS_LENGTH_AUTO, false))
						value->v.svg.fLength.fAuto = true;
					else if (isLineHeight && inLexParser->GetCurTokenValue().EqualToString( kCSS_LENGTH_NORMAL, false))
						value->v.svg.fLength.fAuto = true;
					else if (isFontSize)
					{
						value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_MEDIUM, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_MEDIUM;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_XX_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_XX_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_X_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_X_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_SMALL, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_SMALL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_X_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_X_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_XX_LARGE, false))
							value->v.svg.fLength.fNumber = kFONT_SIZE_XX_LARGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_LARGER, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_FONTSIZE_LARGER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_SIZE_SMALLER, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_FONTSIZE_SMALLER;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}

						if (value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
					}
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}

					done = true;
					if (ioValue)
						*ioValue = inLexParser->GetCurTokenValue();
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

	
		case CSSProperty::FONTFAMILY:
			{
			value->v.css.fFontFamily = NULL;
			VString fontFamily;
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::STRING:
				case CSSToken::IDENT:
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else 
						{
							if (!fontFamily.IsEmpty())
								fontFamily.AppendUniChar(' ');
							fontFamily.AppendString(inLexParser->GetCurTokenValue());
						}
						if (ioValue)
							inLexParser->ConcatCurTokenDumpValue( *ioValue);
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
					{
						if (!fontFamily.IsEmpty())
						{
							if (value->v.css.fFontFamily == NULL)
								value->v.css.fFontFamily = new VectorOfVString();
							value->v.css.fFontFamily->push_back( fontFamily);
							fontFamily.SetEmpty();
						}
						
						return value;
					}
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				case CSSToken::COMMA:
					{
					if (!fontFamily.IsEmpty())
					{
						if (value->v.css.fFontFamily == NULL)
							value->v.css.fFontFamily = new VectorOfVString();
						value->v.css.fFontFamily->push_back( fontFamily);
						fontFamily.SetEmpty();
					}
					if (ioValue)
						inLexParser->ConcatCurTokenDumpValue( *ioValue);
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (!inParseFirstTokenOnly)
					inLexParser->Next( true); 
				else
					break;
			} 
			if (done)
			{
				if (!fontFamily.IsEmpty())
				{
					if (value->v.css.fFontFamily == NULL)
						value->v.css.fFontFamily = new VectorOfVString();
					value->v.css.fFontFamily->push_back( fontFamily);
				}
				
				return value;
			}
			else
			{
				XBOX::ReleaseRefCountable(&value);
				return NULL;
			}
			}
			break;

		case CSSProperty::FONTWEIGHT:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_WEIGHT_NORMAL, false))
							value->v.css.fFontWeight = CSSProperty::kFONT_WEIGHT_NORMAL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_WEIGHT_BOLD, false))
							value->v.css.fFontWeight = CSSProperty::kFONT_WEIGHT_BOLD;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_WEIGHT_BOLDER, false))
							value->v.css.fFontWeight = CSSProperty::kFONT_WEIGHT_BOLDER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_WEIGHT_LIGHTER, false))
							value->v.css.fFontWeight = CSSProperty::kFONT_WEIGHT_LIGHTER;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::NUMBER:
					{
						if (done)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else
						{
							int weight = (int)inLexParser->GetCurTokenValueNumber();
							if (((weight % 100) == 0)
								&&
								(weight >= CSSProperty::kFONT_WEIGHT_MIN)
								&&
								(weight <= CSSProperty::kFONT_WEIGHT_MAX))
							{
								value->v.css.fFontWeight = (CSSProperty::eFontWeight)weight;
								done = true;
							}
							else
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
						}
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::FONTSTYLE:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_NORMAL, false))
							value->v.css.fFontStyle = CSSProperty::kFONT_STYLE_NORMAL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_ITALIC, false))
							value->v.css.fFontStyle = CSSProperty::kFONT_STYLE_ITALIC;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_OBLIQUE, false))
							value->v.css.fFontStyle = CSSProperty::kFONT_STYLE_OBLIQUE;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::TEXTDECORATION:
			{
			value->v.css.fTextDecoration = 0;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_NONE, false))
							value->v.css.fTextDecoration = 0;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_UNDERLINE, false))
							value->v.css.fTextDecoration |= CSSProperty::kFONT_STYLE_UNDERLINE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_OVERLINE, false))
							value->v.css.fTextDecoration |= CSSProperty::kFONT_STYLE_OVERLINE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_LINE_THROUGH, false))
							value->v.css.fTextDecoration |= CSSProperty::kFONT_STYLE_LINETHROUGH;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_FONT_STYLE_BLINK, false))
							value->v.css.fTextDecoration |= CSSProperty::kFONT_STYLE_BLINK;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							inLexParser->ConcatCurTokenDumpValue( *ioValue);
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				case CSSToken::COMMA:
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::STRING:
			{
			value->v.css.fString = NULL;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::STRING:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.css.fString = new VString( inLexParser->GetCurTokenValue());
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::DIRECTION:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_DIRECTION_LTR, false))
							value->v.css.fDirection = CSSProperty::kDIRECTION_LTR;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_DIRECTION_RTL, false))
							value->v.css.fDirection = CSSProperty::kDIRECTION_RTL;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::WRITINGMODE:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_LR, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_LR;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_LR_TB, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_LR;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_RL, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_RL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_RL_TB, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_RL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_TB, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_TB;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_WRITING_MODE_TB_RL, false))
							value->v.css.fWritingMode = CSSProperty::kWRITING_MODE_TB;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::TEXTALIGN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_LEFT, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_LEFT;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_CENTER, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_CENTER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_RIGHT, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_RIGHT;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_JUSTIFY, false))
							value->v.css.fTextAlign = CSSProperty::kTEXT_ALIGN_JUSTIFY;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value); 
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::VERTICALALIGN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_BASELINE, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_BASELINE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_SUB, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_SUB;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_SUPER, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_SUPER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_TOP, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_TOP;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_TEXT_TOP, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_TEXT_TOP;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_MIDDLE, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_MIDDLE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_BOTTOM, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_BOTTOM;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_TEXT_BOTTOM, false))
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_TEXT_BOTTOM;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::NUMBER:
				case CSSToken::DIMENSION:
				case CSSToken::PERCENTAGE:
					{
						//not supported: we set to default CSS alignment (baseline)
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else
							value->v.css.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_BASELINE;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::BACKGROUNDPOSITION:
			{
				sLONG count = 0;
				while (inLexParser->GetCurToken() != CSSToken::END)
				{
					switch( inLexParser->GetCurToken())
					{
					case CSSToken::IDENT:
						{
							if (done == true && count >= 2)
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
							if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							{
								if (count > 0)
								{
									XBOX::ReleaseRefCountable(&value);
									return NULL;
								}
								value->fInherit = true;
								return value;
							}
							else if (count == 0)
							{
								if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_LEFT, false))
									value->v.css.fBackgroundPosition.fTextAlign = CSSProperty::kTEXT_ALIGN_LEFT;
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_CENTER, false))
									value->v.css.fBackgroundPosition.fTextAlign = CSSProperty::kTEXT_ALIGN_CENTER;
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_ALIGN_RIGHT, false))
									value->v.css.fBackgroundPosition.fTextAlign = CSSProperty::kTEXT_ALIGN_RIGHT;
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_TOP, false))
								{
									value->v.css.fBackgroundPosition.fTextAlign = CSSProperty::kTEXT_ALIGN_CENTER;
									value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_TOP;
									count = 1;
								}
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_BOTTOM, false))
								{
									value->v.css.fBackgroundPosition.fTextAlign = CSSProperty::kTEXT_ALIGN_CENTER;
									value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_BOTTOM;
									count = 1;
								}
								else 
								{
									XBOX::ReleaseRefCountable(&value);
									return NULL;
								}
							}
							else
							{
								if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_TOP, false))
									value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_TOP;
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_CENTER, false)) 
									value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_MIDDLE;
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VERTICAL_ALIGN_BOTTOM, false))
									value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_BOTTOM;
								else 
								{
									XBOX::ReleaseRefCountable(&value);
									return NULL;
								}
							}
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
						}
						break;
					case CSSToken::SEMI_COLON:
					case CSSToken::RIGHT_CURLY_BRACE:
					case CSSToken::IMPORTANT_SYMBOL:
						{
						if (done)
						{
							if (count < 2)
								value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_MIDDLE;
							return value;
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						}
						break;
					default:
						XBOX::ReleaseRefCountable(&value); 
						return NULL;
						break;
					}
					if (inParseFirstTokenOnly)
						return value;
					else
						inLexParser->Next( true); 
					count++;
					done = true;
				} 
				if (done)
				{
					if (count < 2)
						value->v.css.fBackgroundPosition.fVerticalAlign = CSSProperty::kVERTICAL_ALIGN_MIDDLE;
				}
			}
			break;
		case CSSProperty::BACKGROUNDREPEAT:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_REPEAT, false))
							value->v.css.fBackgroundRepeat = CSSProperty::kBR_REPEAT;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_REPEAT_X, false))
							value->v.css.fBackgroundRepeat = CSSProperty::kBR_REPEAT_X;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_REPEAT_Y, false))
							value->v.css.fBackgroundRepeat = CSSProperty::kBR_REPEAT_Y;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_NO_REPEAT, false))
							value->v.css.fBackgroundRepeat = CSSProperty::kBR_NO_REPEAT;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value); 
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::BACKGROUNDCLIP:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_BORDER_BOX, false))
							value->v.css.fBackgroundClip = CSSProperty::kBB_BORDER_BOX;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_PADDING_BOX, false))
							value->v.css.fBackgroundClip = CSSProperty::kBB_PADDING_BOX;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_CONTENT_BOX, false))
							value->v.css.fBackgroundClip = CSSProperty::kBB_CONTENT_BOX;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value); 
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::BACKGROUNDORIGIN:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_BORDER_BOX, false))
							value->v.css.fBackgroundOrigin = CSSProperty::kBB_BORDER_BOX;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_PADDING_BOX, false))
							value->v.css.fBackgroundOrigin = CSSProperty::kBB_PADDING_BOX;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_CONTENT_BOX, false))
							value->v.css.fBackgroundOrigin = CSSProperty::kBB_CONTENT_BOX;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value); 
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::BACKGROUNDSIZE:
			{
				sLONG count = 0;
				value->v.css.fBackgroundSize.fPreset = CSSProperty::kBS_AUTO;
				while (inLexParser->GetCurToken() != CSSToken::END)
				{
					switch( inLexParser->GetCurToken())
					{
					case CSSToken::IDENT:
						{
							if (done == true && count >= 2)
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
							if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							{
								if (count > 0)
								{
									XBOX::ReleaseRefCountable(&value);
									return NULL;
								}
								value->fInherit = true;
								return value;
							}
							else 
							{
								if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_SIZE_COVER, false))
								{
									value->v.css.fBackgroundSize.fPreset = CSSProperty::kBS_COVER;
									count = 1;
								}
								else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BACKGROUND_SIZE_CONTAIN, false))
								{
									value->v.css.fBackgroundSize.fPreset = CSSProperty::kBS_CONTAIN;
									count = 1;
								}
								else 
								{
									CSSProperty::Value *subValue = VCSSParser::ParseValue( inLexParser, CSSProperty::LENGTH, ioValue, true, true);
									if (subValue)
									{
										if (count == 0)
											value->v.css.fBackgroundSize.fWidth = subValue->v.css.fLength;
										else
											value->v.css.fBackgroundSize.fHeight = subValue->v.css.fLength;
										ReleaseRefCountable(&subValue);
									}
									else
									{
										XBOX::ReleaseRefCountable(&value);
										return NULL;
									}
								}
							}
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenValue();
						}
						break;
					case CSSToken::NUMBER:
					case CSSToken::PERCENTAGE:
					case CSSToken::DIMENSION:
						{
							CSSProperty::Value *subValue = VCSSParser::ParseValue( inLexParser, CSSProperty::LENGTH, ioValue, true, true);
							if (subValue)
							{
								if (count == 0)
									value->v.css.fBackgroundSize.fWidth = subValue->v.css.fLength;
								else
									value->v.css.fBackgroundSize.fHeight = subValue->v.css.fLength;
								ReleaseRefCountable(&subValue);
							}
							else
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
						}
						break;
					case CSSToken::SEMI_COLON:
					case CSSToken::RIGHT_CURLY_BRACE:
					case CSSToken::IMPORTANT_SYMBOL:
						{
						if (done)
						{
							if (count < 2)
								value->v.css.fBackgroundSize.fHeight.fAuto = true;
							return value;
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						}
						break;
					default:
						XBOX::ReleaseRefCountable(&value); 
						return NULL;
						break;
					}
					if (inParseFirstTokenOnly)
						return value;
					else
						inLexParser->Next( true); 
					count++;
					done = true;
				} 
				if (done)
				{
					if (count < 2)
						value->v.css.fBackgroundSize.fHeight.fAuto = true;
				}
			}
			break;

		case CSSProperty::BORDERSTYLE:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_SOLID, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_SOLID;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_DOTTED, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_DOTTED;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_DASHED, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_DASHED;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_DOUBLE, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_DOUBLE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_NONE, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_NONE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_HIDDEN, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_HIDDEN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_GROOVE, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_GROOVE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_RIDGE, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_RIDGE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_INSET, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_INSET;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_BORDER_STYLE_OUTSET, false))
							value->v.css.fBorderStyle = CSSProperty::kBORDER_STYLE_OUTSET;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::BORDERWIDTH:
			{
			value->v.svg.fLength.fAuto = false;
			value->v.svg.fLength.fNumber = 0.0;
			value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PX;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					{
						if (done == true)
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_BORDER_WIDTH_THIN, false))
						{
							value->v.css.fLength.fNumber = 1;
							value->v.css.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_BORDER_WIDTH_MEDIUM, false))
						{
							value->v.css.fLength.fNumber = 2;
							value->v.css.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TEXT_BORDER_WIDTH_THICK, false))
						{
							value->v.css.fLength.fNumber = 4;
							value->v.css.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						}
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else 
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::NUMBER:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::DIMENSION:
					if (!done)
					{
						value->v.svg.fLength.fNumber = inLexParser->GetCurTokenValueNumber();
						if (value->v.svg.fLength.fNumber < 0.0)
							value->v.svg.fLength.fNumber = 0.0;

						if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PT, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PT;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EX, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_EX;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_IN, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_IN;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_CM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_CM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_MM, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_MM;
						else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PC, false))
							value->v.svg.fLength.fUnit = CSSProperty::LENGTH_TYPE_PC;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}

						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::NUMBER:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::NUMBER:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.css.fNumber = inLexParser->GetCurTokenValueNumber();
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::URI:
			{
			value->v.css.fURI = NULL;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::URI:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.css.fURI = new VString( inLexParser->GetCurTokenValue());
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::DIMENSION:
			{
			value->v.css.fDimension.fIdent = NULL;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::DIMENSION:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.css.fDimension.fNumber = inLexParser->GetCurTokenValueNumber();
						value->v.css.fDimension.fIdent = new VString( inLexParser->GetCurTokenValueIdent());
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		case CSSProperty::PERCENTAGE:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::PERCENTAGE:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						value->v.css.fPercentage = inLexParser->GetCurTokenValueNumber()*0.01;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::DISPLAY:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_NONE, false))
							value->v.css.fDisplay = false;
						else
							value->v.css.fDisplay = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::VISIBILITY:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_HIDDEN, false))
							value->v.css.fVisibility = false;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_COLLAPSE, false))
							value->v.css.fVisibility = false;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_VISIBLE, false))
							value->v.css.fVisibility = true;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::COLOR:
			{
			value->v.css.fColor.fTransparent = false;
			value->v.css.fColor.fInvert = false;

			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::HASH:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (VCSSUtil::ParseColorHexa( inLexParser->GetCurTokenValue(), &value->v.css.fColor.fColor))
						{
							if (ioValue)
								*ioValue = inLexParser->GetCurTokenDumpValue();
							done = true;
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
					}
					break;
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
					{
						value->fInherit = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TRANSPARENT, false))
					{
						value->v.css.fColor.fTransparent = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INVERT, false))
					{
						value->v.css.fColor.fInvert = true;
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else if (VCSSUtil::ParseColorIdent( inLexParser->GetCurTokenValue(), &(value->v.css.fColor.fColor)))
					{
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenValue();
						done = true;
					}
					else 
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				case CSSToken::FUNCTION:
					/** parse 'rgb(red,green,blue)' color */
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						uLONG color = 0;
						if (!VCSSParser::ParseRGBFunc( inLexParser, color, ioValue))
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						value->v.css.fColor.fColor = color;
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			}
			break;

		//4D CSS reserved 
		case CSSProperty::TABSTOPTYPE4D:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TAB_STOP_TYPE_LEFT, false))
							value->v.css.fTabStopType = CSSProperty::kTST_LEFT;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TAB_STOP_TYPE_RIGHT, false))
							value->v.css.fTabStopType = CSSProperty::kTST_RIGHT;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TAB_STOP_TYPE_CENTER, false))
							value->v.css.fTabStopType = CSSProperty::kTST_CENTER;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TAB_STOP_TYPE_DECIMAL, false))
							value->v.css.fTabStopType = CSSProperty::kTST_DECIMAL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_TAB_STOP_TYPE_BAR, false))
							value->v.css.fTabStopType = CSSProperty::kTST_BAR;
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		case CSSProperty::LISTSTYLETYPE:
		case CSSProperty::LISTSTYLETYPE4D:
			while (inLexParser->GetCurToken() != CSSToken::END)
			{
				switch( inLexParser->GetCurToken())
				{
				case CSSToken::IDENT:
					if (done)
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					else
					{
						if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
							value->fInherit = true;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_DISC, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_DISC;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_CIRCLE, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_CIRCLE;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_SQUARE, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_SQUARE;

						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_DECIMAL, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_DECIMAL;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_DECIMAL_LEADING_ZERO;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_LOWER_LATIN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_LOWER_LATIN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_LOWER_ROMAN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_LOWER_ROMAN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_LOWER_GREEK, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_LOWER_GREEK;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_UPPER_LATIN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_UPPER_LATIN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_UPPER_ROMAN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_UPPER_ROMAN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_ARMENIAN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_ARMENIAN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_GEORGIAN, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_GEORGIAN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_HEBREW, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_HEBREW;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_HIRAGANA, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_HIRAGANA;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_KATAKANA, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_KATAKANA;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_CJK_IDEOGRAPHIC;

						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_LOWER_ALPHA, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_LOWER_LATIN;
						else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_UPPER_ALPHA, false))
							value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_UPPER_LATIN;

						else if (inType == CSSProperty::LISTSTYLETYPE4D)
						{
							if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_HOLLOW_SQUARE, false))
								value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_HOLLOW_SQUARE;
							else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_DIAMOND, false))
								value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_DIAMOND;
							else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_CLUB, false))
								value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_CLUB;
							else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_CUSTOM, false))
								value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_CUSTOM;
							else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_LIST_STYLE_TYPE_DECIMAL_GREEK, false))
								value->v.css.fListStyleType = CSSProperty::kLIST_STYLE_TYPE_DECIMAL_GREEK;
							else
							{
								XBOX::ReleaseRefCountable(&value);
								return NULL;
							}
						}
						else
						{
							XBOX::ReleaseRefCountable(&value);
							return NULL;
						}
						if (ioValue)
							*ioValue = inLexParser->GetCurTokenDumpValue();
						done = true;
					}
					break;
				case CSSToken::SEMI_COLON:
				case CSSToken::RIGHT_CURLY_BRACE:
				case CSSToken::IMPORTANT_SYMBOL:
					if (done)
						return value;
					else
					{
						XBOX::ReleaseRefCountable(&value);
						return NULL;
					}
					break;
				default:
					XBOX::ReleaseRefCountable(&value);
					return NULL;
					break;
				}
				if (inParseFirstTokenOnly)
					return value;
				else
					inLexParser->Next( true); 
			} 
			break;

		default:
			xbox_assert(false);
			break;
		}

	if (done)
		return value;
	else
	{
		XBOX::ReleaseRefCountable(&value);
		return NULL;
	}
}

/** parse declarations */
void VCSSParser::_ParseDeclarations()
{
	fHasRules = true;

	if (fCurRule == NULL)
	{
		//add new rule with 'any' default selector
		fCurRules->push_back( CSSRuleSet::Rule( CSSRuleSet::Selectors(), CSSRuleSet::Declarations()));
		fCurRule = &(fCurRules->back());
		fCurRule->second.reserve(10);
	}

	VString name;
	VString value;
	bool	bIsImportant = false;
	bool	bParseValue = false;

	CSSProperty::eType valueType = CSSProperty::NUMBER;
	CSSProperty::Value *valueParsed = NULL;

	fLexParser.ParsePseudoClass(false);
	fLexParser.Next(true);
	while (fLexParser.GetCurToken() != CSSToken::END)
	{
		try
		{
			switch( fLexParser.GetCurToken())
			{
			case CSSToken::IDENT:
				{
				if (!bParseValue)
				{
					if (name.IsEmpty())
						name = fLexParser.GetCurTokenValue();
					else
						throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				else
					if (!bIsImportant)
					{
						if (value.IsEmpty())
							value = fLexParser.GetCurTokenDumpValue();
						else
							value += " "+fLexParser.GetCurTokenDumpValue();
					}
					else
						throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				break;
			case CSSToken::COLON:
				{
					if (name.IsEmpty() || bParseValue)
						throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());

					bParseValue = true;

					/** search for value type if it can be parsed here */
					if (fMapAttributeType && !fMapAttributeType->empty())
					{
						CSSProperty::MapAttributeType::const_iterator it = fMapAttributeType->find( name);
						if (it != fMapAttributeType->end())
						{
							valueType = it->second;

							fLexParser.Next(true);
							valueParsed = _ParseValue( valueType, value);
							if (valueParsed == NULL)
								throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
							switch (fLexParser.GetCurToken())
							{
								case CSSToken::SEMI_COLON:
									{
										//add declaration
										if ((!name.IsEmpty()) && (!value.IsEmpty()))
											fCurRule->second.push_back( CSSRuleSet::Declaration( name, value, valueParsed, bIsImportant));

										//reset declaration
										name = "";
										value = "";
										bIsImportant = false;
										bParseValue = false;
										XBOX::ReleaseRefCountable(&valueParsed);
									}
									break;
								case CSSToken::RIGHT_CURLY_BRACE:
									{
										//add declaration
										if ((!name.IsEmpty()) && (!value.IsEmpty()))
											fCurRule->second.push_back( CSSRuleSet::Declaration( name, value, valueParsed, bIsImportant));
										XBOX::ReleaseRefCountable(&valueParsed);
										fLexParser.ParsePseudoClass(true);
										return;
									}
								case CSSToken::IMPORTANT_SYMBOL:
									{
									if (bParseValue)
										bIsImportant = true;
									else
										throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
									}
									break;
								default:
									break;
							}
							break;
						}
					}
				}
				break;
			case CSSToken::SEMI_COLON:
				{
					//add declaration
					if ((!name.IsEmpty()) && (!value.IsEmpty()))
						fCurRule->second.push_back( CSSRuleSet::Declaration( name, value, valueParsed, bIsImportant));

					//reset declaration
					name = "";
					value = "";
					bIsImportant = false;
					bParseValue = false;
					XBOX::ReleaseRefCountable(&valueParsed);
				}
				break;
			case CSSToken::RIGHT_CURLY_BRACE:
				{
					//add declaration
					if ((!name.IsEmpty()) && (!value.IsEmpty()))
						fCurRule->second.push_back( CSSRuleSet::Declaration( name, value, valueParsed, bIsImportant));
					XBOX::ReleaseRefCountable(&valueParsed);
					fLexParser.ParsePseudoClass(true);
					return;
				}
				break;
			case CSSToken::IMPORTANT_SYMBOL:
				{
				if (bParseValue)
					bIsImportant = true;
				else
					throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				break;
			default:
				{
				if (bParseValue && (!bIsImportant))
					_ConcatCurTokenDumpValue( value);
				else
					throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, fLexParser.GetCurLine(), fLexParser.GetCurColumn());
				}
				break;
			}
			fLexParser.Next( true); 
		}
		catch (VCSSException err) 
		{
			if (fContinueParseAfterError) 
			{
				// If we were asked to parse the entire document instead of stopping at the first
				// sign of troubles, we need to add this exception to our list
				fExceptionList.AddCSSException( err );

				//reset current declaration
				name = "";
				value = "";
				bIsImportant = false;
				bParseValue = false;
				XBOX::ReleaseRefCountable(&valueParsed);

				do 
				{
					//line ending: resume parsing 
					if (fLexParser.HasCurTokenLineEnding())
					{
						fLexParser.Next( true );
						break;
					}

					//end of current block: end of declaration block
					if (fLexParser.GetCurToken() == CSSToken::RIGHT_CURLY_BRACE)
					{
						XBOX::ReleaseRefCountable(&valueParsed);
						fLexParser.ParsePseudoClass(true);
						return;
					}

					//end of current statement: resume parsing 
					if (fLexParser.GetCurToken() == CSSToken::SEMI_COLON) 
					{
						fLexParser.Next( true );
						break;
					}
				} while (fLexParser.Next( false ) != CSSToken::END);
			} 
			else 
			{
				// We are supposed to stop when we hit the first error, so just rethrow the exception
				throw err;
			}
		}
	} 
	XBOX::ReleaseRefCountable(&valueParsed);
	fLexParser.ParsePseudoClass(true);
}


/** return 0 if selectors are of the same weight,
		   1 if selector2 is more specific than selector1
		   -1 if selector1 is more specific than selector2 */
int VCSSSelectorCompare(const CSSRuleSet::Selector& inSelector1, const CSSRuleSet::Selector& inSelector2)
{
	CSSRuleSet::Selector::const_reverse_iterator itSimpleSelector1 = inSelector1.rbegin();
	CSSRuleSet::Selector::const_reverse_iterator itSimpleSelector2 = inSelector2.rbegin();	
	bool continue1 = itSimpleSelector1 != inSelector1.rend();
	bool continue2 = itSimpleSelector2 != inSelector2.rend();
	while( continue1 || continue2)
	{
		//check selector depth priority
		if (continue2
			&&
			(!continue1))
			return 1;
		if (continue1
			&&
			(!continue2))
			return -1;

		//check ID condition priority
		if (itSimpleSelector2->fHasCondID && (!itSimpleSelector1->fHasCondID))
			return 1;
		if (itSimpleSelector1->fHasCondID && (!itSimpleSelector2->fHasCondID))
			return -1;

		//check element name condition priority
		if (itSimpleSelector1->fNameElem.IsEmpty() && (!itSimpleSelector2->fNameElem.IsEmpty()))
			return 1;
		else if (itSimpleSelector2->fNameElem.IsEmpty() && (!itSimpleSelector1->fNameElem.IsEmpty()))
			return -1;
			
		//check attribute conditions priority:
		//condition with full attribute name has more priority than condition with include attribute name
		//and the latter has more priority than class condition
		//(just estimate priority by comparing count of conditions because 
		// conditions of same kind have same priority)
		int numClassCond1 = 0;
		int numClassCond2 = 0;
		int numIncludeCond1 = 0;
		int numIncludeCond2 = 0;
		int numCond1 = 0; 
		int numCond2 = 0;

		//here PC_FIRST_CHILD and PC_LANG pseudo-classes are assumed same priority
		//than conditions with full attribute name
		if (itSimpleSelector1->fPseudos.find( CSSRuleSet::Pseudo::PC_FIRST_CHILD) != itSimpleSelector1->fPseudos.end())
			numCond1++;
		if (itSimpleSelector1->fPseudos.find( CSSRuleSet::Pseudo::PC_LANG) != itSimpleSelector1->fPseudos.end())
			numCond1++;
		if (itSimpleSelector2->fPseudos.find( CSSRuleSet::Pseudo::PC_FIRST_CHILD) != itSimpleSelector2->fPseudos.end())
			numCond2++;
		if (itSimpleSelector2->fPseudos.find( CSSRuleSet::Pseudo::PC_LANG) != itSimpleSelector2->fPseudos.end())
			numCond2++;

		CSSRuleSet::CondVector::const_iterator itCondition1 = itSimpleSelector1->fConditions.begin();
		for (;itCondition1 != itSimpleSelector1->fConditions.end(); itCondition1++)
		{
			if (itCondition1->fIsClass)
				numClassCond1++;
			else if (itCondition1->fInclude || itCondition1->fBeginsWith)
					numIncludeCond1++;
			else
				numCond1++;
		}
		CSSRuleSet::CondVector::const_iterator itCondition2 = itSimpleSelector2->fConditions.begin();
		for (;itCondition2 != itSimpleSelector2->fConditions.end(); itCondition2++)
		{
			if (itCondition2->fIsClass)
				numClassCond2++;
			else if (itCondition2->fInclude || itCondition2->fBeginsWith)
					numIncludeCond2++;
			else
				numCond2++;
		}
		if (numCond2 > numCond1)
			return 1;
		else if (numCond2 < numCond1)
			return -1;

		if (numIncludeCond2 > numIncludeCond1)
			return 1;
		else if (numIncludeCond2 < numIncludeCond1)
			return -1;
		
		if (numClassCond2 > numClassCond1)
			return 1;
		else if (numClassCond2 < numClassCond1)
			return -1;

		//next step
		if (continue1)
		{
			itSimpleSelector1++;
			continue1 = itSimpleSelector1 != inSelector1.rend();
		}
		if (continue2)	
		{
			itSimpleSelector2++;
			continue2 = itSimpleSelector2 != inSelector2.rend();
		}
	}
	return 0;
}


typedef struct VCSSMatchAttribute
{
	const VString *fName;
	const VString *fValue;
	CSSProperty::Value *fValueParsed;
	const CSSRuleSet::Selector *fSelector;
	bool fIsImportant;
} VCSSMatchAttribute;

typedef std::vector<VCSSMatchAttribute> VCSSMatchAttributeVector;

/** scan CSS rules for the specified element 
	and call fHandlerSetAttribute for each attribute that is overriden
@param inElement
	element opaque ref
@param inMediaType
	media type filter (CSSMedia::ALL is default)
	(filter CSS styles with @media at-rules)
@remarks
	all handlers must be initialized
*/
void VCSSMatch::Match(opaque_ElementPtr inElement, CSSMedia::eMediaType inMediaType)
{
	if (fMediaRules == NULL)
		return;

	//JQ 16/12/2009: fixed cascading rules

	CSSRuleSet::Selector selectorEmpty;
	VCSSMatchAttributeVector attributes;
	const CSSRuleSet::Selector *curSelector;

	//iterate on medias
	xbox_assert(fMediaRules);
	CSSRuleSet::MediaRules::reverse_iterator itMedia = fMediaRules->rbegin();
	for (;itMedia != fMediaRules->rend(); itMedia++)
	{
		//check with media type filter
		if (inMediaType != CSSMedia::ALL)
		{
			const CSSMedia::Set& medias = (*itMedia).first;
			if (!((medias.find( inMediaType) != medias.end())
				  ||
				  (medias.find( CSSMedia::ALL) != medias.end())))
				continue;
		}

		//iterate on rules
		CSSRuleSet::Rules::reverse_iterator itRule = (*itMedia).second.rbegin();
		for (;itRule != (*itMedia).second.rend();itRule++)
		{
			curSelector = &selectorEmpty;

			//iterate on selectors (one at least must match)
			bool bMatch = false;
			if ((*itRule).first.empty())
				bMatch = true;
			else
			{
				CSSRuleSet::Selectors::const_iterator itSelector = (*itRule).first.begin();
				for (;(!bMatch) && (itSelector != (*itRule).first.end()); itSelector++)
				{
					curSelector = &(*itSelector);

					//check simple selectors (all must match)
					CSSRuleSet::Selector::const_reverse_iterator itSimpleSelector = (*itSelector).rbegin();
					int index = ((int)(*itSelector).size())-1;
					opaque_ElementPtr elemCur = inElement;
					bMatch = true;
					if (itSimpleSelector != (*itSelector).rend())
					{
						//first check last simple selector
						bMatch = _MatchSimpleSelector( inElement, *itSimpleSelector);

						//reverse iterate on simple selectors
						while (bMatch && index > 0)
						{
							//step to previous simple selector
							CSSRuleSet::eCombinator combinator = (*itSimpleSelector).fCombinator;
							itSimpleSelector++;

							//check combinator
							switch( combinator)
							{
							case CSSRuleSet::DESCENDANT:
								{
									//check for first ancestor that match simple selector
									opaque_ElementPtr elemParent = elemCur;
									do
									{
										elemParent = fHandlerGetParentElement( elemParent);
										if (elemParent && _MatchSimpleSelector( elemParent, *itSimpleSelector))
											break;
									} while (elemParent);
									if (elemParent == NULL)
										bMatch = false;
									elemCur = elemParent;
								}
								break;
							case CSSRuleSet::CHILD:
								{
									//check if parent element match simple selector
									opaque_ElementPtr elemParent = fHandlerGetParentElement( elemCur);
									if (!(elemParent && _MatchSimpleSelector( elemParent, *itSimpleSelector)))
										bMatch = false;
									elemCur = elemParent;
								}
								break;
							case CSSRuleSet::ADJACENT:
								{
									//check if previous element match simple selector
									opaque_ElementPtr elemPrev = fHandlerGetPrevElement( elemCur);
									if (!(elemPrev && _MatchSimpleSelector( elemPrev, *itSimpleSelector)))
										bMatch = false;
									elemCur = elemPrev;
								}
								break;
							default:
								xbox_assert(false);
								break;
							}
							index--;
						}
					}
				}
			}
			if (!bMatch)
				continue;

			//result is true: iterate on declarations
			CSSRuleSet::Declarations::const_iterator itDeclaration = (*itRule).second.begin();
			for (;itDeclaration != (*itRule).second.end(); itDeclaration++)
			{
				//ensure that only the most specific or important rule apply per attribute
				bool bSkip = false;
				VCSSMatchAttributeVector::iterator it = attributes.begin();
				for (;it != attributes.end(); it++)
				{
					if ((*it).fName->EqualToString( (*itDeclaration).fName, false))
					{
						//attribute has been defined by a rule yet

						if ((*itDeclaration).fIsImportant && (!(*it).fIsImportant))
						{
							//always override if new declaration is important and not the previous one 
							(*it).fValue = &((*itDeclaration).fValue);
							XBOX::CopyRefCountable( &((*it).fValueParsed), (*itDeclaration).fValueParsed);
							(*it).fIsImportant = true;
							(*it).fSelector = curSelector;
						}
						else if ((*itDeclaration).fIsImportant == (*it).fIsImportant)
						{
							//both current declaration & previous one have same importance:
							//rule selector strongness will determine which declaration have the precedence
							int compare = VCSSSelectorCompare( *((*it).fSelector), *curSelector);
							if (compare > 0)
							{
								//if current declaration selector is more specific than the previous one,
								//use it
								(*it).fValue = &((*itDeclaration).fValue);
								XBOX::CopyRefCountable( &((*it).fValueParsed), (*itDeclaration).fValueParsed);
								//(*it).fIsImportant = (*itDeclaration).fIsImportant;
								(*it).fSelector = curSelector;
							}
						}
						bSkip = true;
						break;
					}
				}
				if (!bSkip)
				{
					//add new attribute
					attributes.push_back(VCSSMatchAttribute());
					attributes.back().fName			= &((*itDeclaration).fName);
					attributes.back().fValue		= &((*itDeclaration).fValue);
					attributes.back().fValueParsed	= NULL;
					XBOX::CopyRefCountable( &(attributes.back().fValueParsed), (*itDeclaration).fValueParsed);
					attributes.back().fIsImportant	= (*itDeclaration).fIsImportant;
					attributes.back().fSelector		= curSelector;
				}
			}
		}
	}

	//finally set attributes
	VCSSMatchAttributeVector::iterator itAttribute = attributes.begin();
	for (;itAttribute != attributes.end(); itAttribute++)
	{
		fHandlerSetAttribute(inElement, *((*itAttribute).fName), *((*itAttribute).fValue), (*itAttribute).fIsImportant, (*itAttribute).fValueParsed);
		XBOX::ReleaseRefCountable( &((*itAttribute).fValueParsed));
	}
}


/** return true if the specified element match the specified simple selector conditions */
bool VCSSMatch::_MatchSimpleSelector(opaque_ElementPtr inElement, const CSSRuleSet::SimpleSelector& simpleSelector)
{
	if (!simpleSelector.fNameElem.IsEmpty())
	{
		//check element name
		if (!fHandlerGetElementName( inElement).EqualToString( simpleSelector.fNameElem, true)) //element name test is case sensitive in XML
			return false;
	}

	//iterate on pseudo-class conditions
	CSSRuleSet::Pseudo::Set::const_iterator itPseudo = simpleSelector.fPseudos.begin();
	for (;itPseudo != simpleSelector.fPseudos.end(); itPseudo++)
	{
		if (*itPseudo == (int)CSSRuleSet::Pseudo::PC_FIRST_CHILD)
		{
			//check first child rule
			if (fHandlerGetParentElement( inElement) == NULL
				||
				fHandlerGetPrevElement( inElement) != NULL)
				return false;
		}
		else if (*itPseudo == (int)CSSRuleSet::Pseudo::PC_LANG)
		{
			//check lang type rule 
			VString value;
			value = fHandlerGetElementLangType( inElement);
			if (!value.BeginsWith(simpleSelector.fLangType,false))
				return false;
		}
		else
		{
			//skip other pseudo-class rules
			return false;
		}
	}

	//iterate on attribute conditions 
	CSSRuleSet::CondVector::const_iterator itCond = simpleSelector.fConditions.begin();
	for (;itCond != simpleSelector.fConditions.end(); itCond++)
	{
		//check attribute condition
		const CSSRuleSet::Condition& cond = *itCond;
		VString value;

		if (cond.fIsClass)
		{
			//class condition

			value = fHandlerGetElementClass( inElement);
			if (!MatchInclude( value, cond.fValues))
				return false;
		}
		else
		{
			if (!fHandlerGetAttributeValue( inElement, cond.fName, value))
				return false;

			if (!cond.fValues.empty())
			{
				if (cond.fBeginsWith)
				{
					//begins with condition

					if (!value.BeginsWith( cond.fValues[0], false))
						return false;
				}
				else if (cond.fInclude)
				{
					//include condition
					if (!MatchInclude( value, cond.fValues))
						return false;
				}
				else
				{
					//strictly equal condition
					if (!cond.fValues[0].EqualToString( value, false))
						return false;			
				}
			}
		}
	}
	return true;
}

/** return true if inValue contains single value inSingleValue */
bool VCSSMatch::MatchInclude( const VString& inValue, const VString& inSingleValue)
{
	const UniChar *cDoc = inValue.GetCPointer();
	const UniChar *cRule = inSingleValue.GetCPointer();
	const UniChar *cRuleStart = cRule;
	bool bInclude = false;
	bool match = true;
	while(*cDoc != 0)
	{
		if (VCSSUtil::isSpace(*cDoc) || *cDoc == ',')
		{
			if (*cRule == 0)
			{
				bInclude = true;
				break;
			}
			cDoc++;
			cRule = cRuleStart;
			match = true;
		}
		if (*cDoc != 0)
		{
			if (match)
			{
				if (*cDoc != *cRule)
				{
					match = false;
					cRule = cRuleStart;
				}
			}
			cDoc++;
			if (match)
				cRule++;
		}
	}
	if ((*cDoc == 0) && (*cRule == 0))
		bInclude = true;
	return bInclude;
}

/** return true if inValue contains all values in inValues */
bool VCSSMatch::MatchInclude( const VString& inValue, const std::vector<VString>& inValues)
{
	std::vector<VString>::const_iterator itValue = inValues.begin();
	bool bInclude = false;
	for(;itValue != inValues.end();itValue++)
	{
		bInclude = MatchInclude( inValue, *itValue);
		if (!bInclude)
			break;
	}
	return bInclude;
}


/** start inline declarations parsing */
void VCSSParserInlineDeclarations::Start( const VString& inDeclarations)
{
	fDeclarations.clear();
	fDeclarations.reserve(10);
	fIndexCur = 0;

	VCSSLexParser *lexParser = new VCSSLexParser();
	try
	{
		//start lexical parser
		lexParser->Start( inDeclarations.GetCPointer());
		lexParser->ParsePseudoClass( false);
	
		VString name;
		VString value;
		bool bIsImportant = false;
		bool bParseValue = false;

		//step to next lexical token
		lexParser->Next(true);
		while (lexParser->GetCurToken() != CSSToken::END)
		{
			//check syntax 
			switch( lexParser->GetCurToken())
			{
			case CSSToken::IDENT:
				{
				if (!bParseValue)
				{
					if (name.IsEmpty())
						name = lexParser->GetCurTokenValue();
					else
						throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, lexParser->GetCurLine(), lexParser->GetCurColumn());
				}
				else
					if (!bIsImportant)
					{
						if (value.IsEmpty())
							value = lexParser->GetCurTokenDumpValue();
						else
							value += " "+lexParser->GetCurTokenDumpValue();
					}
					else
						throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, lexParser->GetCurLine(), lexParser->GetCurColumn());
				}
				break;
			case CSSToken::COLON:
				{
				if (name.IsEmpty() || bParseValue)
					throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, lexParser->GetCurLine(), lexParser->GetCurColumn());
				bParseValue = true;
				}
				break;
			case CSSToken::SEMI_COLON:
				{
					//add declaration
					if ((!name.IsEmpty()) && (!value.IsEmpty()))
						fDeclarations.push_back( CSSRuleSet::Declaration( name, value, bIsImportant));

					//reset declaration
					name = "";
					value = "";
					bIsImportant = false;
					bParseValue = false;
				}
				break;
			case CSSToken::IMPORTANT_SYMBOL:
				{
				if (bParseValue)
					bIsImportant = true;
				else
					throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, lexParser->GetCurLine(), lexParser->GetCurColumn());
				}
				break;
			default:
				{
				if (bParseValue && (!bIsImportant))
				{
					if (value.IsEmpty())
						value = lexParser->GetCurTokenDumpValue();
					else
					{
					if (lexParser->GetCurToken() == CSSToken::LEFT_BRACE 
						|| 
						lexParser->GetCurToken() == CSSToken::RIGHT_BRACE)
						value += lexParser->GetCurTokenDumpValue();
					else if (lexParser->GetCurToken() != CSSToken::COMMA)
						value += " "+lexParser->GetCurTokenDumpValue();
					else
						value += lexParser->GetCurTokenDumpValue();
					}
				}
				else
					throw VCSSException( VE_CSS_PARSER_BAD_DECLARATION, lexParser->GetCurLine(), lexParser->GetCurColumn());
				}
				break;
			}
			lexParser->Next( true); 
		} 
		//add declaration
		if ((!name.IsEmpty()) && (!value.IsEmpty()))
			fDeclarations.push_back( CSSRuleSet::Declaration( name, value, bIsImportant));
	}
	catch( VCSSException e)
	{
		if (!fSilentCSSExceptions)
		{
			delete lexParser;
			throw;
		}
	}
	delete lexParser;
}

//return next CSS2 attribute+value
//return false if there is none left
bool VCSSParserInlineDeclarations::GetNextAttribute( VString& outAttribut, VString& outValue, bool *outIsImportant)
{
	while (fIndexCur < fDeclarations.size())
	{
		outAttribut = fDeclarations[fIndexCur].fName;
		outValue = fDeclarations[fIndexCur].fValue;
		if (outIsImportant)
			*outIsImportant = fDeclarations[fIndexCur].fIsImportant;
		fIndexCur++;
		return true;
	}
	return false;
}



VString VCSSException::fDocURL = "";

/** CSS Exception handler */
VCSSException::VCSSException( VError inError, int inLine, int inColumn)
{
	fError = inError;
	if (!fDocURL.IsEmpty())
		fErrorPath = fDocURL+" ";
	else
		fErrorPath = "";
	if (inLine > -1)
		fErrorPath.AppendPrintf("[line:%d, column:%d]", inLine, inColumn);

	fLine = inLine;
	fColumn = inColumn;
}

const VString& VCSSException::GetDocURL() 
{
	return fDocURL;
}

void VCSSException::SetDocURL( const VString& inDocURL)
{
	fDocURL = inDocURL;
}


//parse a list of string values separated by either comma, space, tab, line feed or carriage return;
//return false if inCount > 0 and output list does not contain inCount values  
bool VCSSUtil::ParseValueList( const VString& inText, std::vector<VString>& outValues, sLONG inCount)
{
	const UniChar *p = inText.GetCPointer();
	const UniChar *start = p;
	const UniChar *end = p;
	bool parseString1 = false;
	bool parseString2 = false;
	while (*p)
	{
		if (parseString1)
		{
			if (*p == '"')
			{
				parseString1 = false;

				VString value;
				value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
				outValues.push_back( value);
				p++;
				start = end = p;
			}
			else
			{
				p++;
				end++;
			}
		}
		else if (parseString2)
		{
			if (*p == '\'')
			{
				parseString2 = false;

				VString value;
				value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
				outValues.push_back( value);
				p++;
				start = end = p;
			}
			else
			{
				p++;
				end++;
			}
		}
		else if (*p == '"')
		{
			if (end > start)
			{
				VString value;
				value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
				outValues.push_back( value);
			}
			
			parseString1 = true;
			p++;
			start = end = p;
		}
		else if (*p == '\'')
		{
			if (end > start)
			{
				VString value;
				value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
				outValues.push_back( value);
			}
			
			parseString2 = true;
			p++;
			start = end = p;
		}
		else if (VCSSUtil::isSpace( *p) || (*p) == ',')
		{
			if (end > start)
			{
				if (parseString1 || parseString2)
					return false;
				VString value;
				value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
				outValues.push_back( value);
			}
			p++;
			start = end = p;
		}
		else
		{
			p++;
			end++;
		}
	}
	if (end > start)
	{
		if (parseString1 || parseString2)
			return false;
		VString value;
		value.FromBlock(start, (end-start) * sizeof(UniChar), VTC_UTF_16);
		outValues.push_back( value);
	}
	
	if (inCount > 0)
		if (outValues.size() != inCount)
			return false;
	return true;
}


//parse a list of string values separated by the specified delimiter character;
//remove leading and trailing whitespaces from parsed values;
//if inCount > 0 return false if output list does not contain inCount values  
bool VCSSUtil::ParseValueListDelim( const VString& inText, std::vector<VString>& outValues, UniChar inDelim, sLONG inCount)
{
	const UniChar *p = inText.GetCPointer();
	VString value;
	value.EnsureSize(128);
	bool prevIsSpace = true;
	bool parseString1 = false;
	bool parseString2 = false;
	while (*p)
	{
		if (parseString1)
		{
			if (*p == '"')
			{
				parseString1 = false;

				if (!value.IsEmpty())
					outValues.push_back( value);
				value.Clear();
				prevIsSpace = true;
				p++;
			}
			else
			{
				value.AppendUniChar(*p);
				p++;
			}
		}
		else if (parseString2)
		{
			if (*p == '\'')
			{
				parseString2 = false;

				if (!value.IsEmpty())
					outValues.push_back( value);
				value.Clear();
				prevIsSpace = true;
				p++;
			}
			else
			{
				value.AppendUniChar(*p);
				p++;
			}
		}
		else if (*p == '"')
		{
			if (!value.IsEmpty())
				outValues.push_back( value);
			value.Clear();
			
			parseString1 = true;
			p++;
		}
		else if (*p == '\'')
		{
			if (!value.IsEmpty())
				outValues.push_back( value);
			value.Clear();
			
			parseString2 = true;
			p++;
		}
		else if (*p == inDelim)
		{
			if (!value.IsEmpty())
				outValues.push_back(value);
			p++;
			value.Clear();
			prevIsSpace = true;
		}
		else if (VCSSUtil::isSpace( *p))
		{
			if (!prevIsSpace)
			{
				value.AppendUniChar(' ');
				prevIsSpace = true;
			}
			p++;
		}
		else
		{
			value.AppendUniChar( *p);
			prevIsSpace = false;
			p++;
		}
	}
	if (parseString1 || parseString2)
		return false;
	if (!value.IsEmpty())
		outValues.push_back(value);
	if (inCount > 0)
	 if (outValues.size() != inCount)
		return false;
	return true;
}



/** parse color component 
@param inColorComponent
	string with [0..255] decimal value or percentage value
@param outDecimal
	[0..255] decimal value */
bool VCSSUtil::ParseColorComponentValue( const VString& inColorComponent, Real& outDecimal)
{
	VString text = inColorComponent;
    text.RemoveWhiteSpaces();

    if (text.EndsWith(kCSS_UNIT_PERCENT, true)) 
	{
        text.SubString(1,text.GetLength()-1);
        outDecimal		= text.GetReal()*2.55;
    } 
	else 	
	{
        outDecimal		= text.GetReal();
	}

	CSSClampVal(outDecimal, 0.0, 255.0);
	return true;
}


/** parse CSS color ident */
bool VCSSUtil::ParseColorIdent( const VString& inColorCSS, uLONG *outColorARGB)
{
	xbox_assert(outColorARGB);

	//initialize predefined color values
	static VValueBag mapColor;
	if (mapColor.IsEmpty())
	{
		mapColor.SetLong(kCSS_XML_COLOR_ALICEBLUE			,			(sLONG)kCSS_COLOR_ALICEBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_ANTIQUEWHITE		,			(sLONG)kCSS_COLOR_ANTIQUEWHITE);		
		mapColor.SetLong(kCSS_XML_COLOR_AQUA				,			(sLONG)kCSS_COLOR_AQUA);		
		mapColor.SetLong(kCSS_XML_COLOR_AQUAMARINE			,			(sLONG)kCSS_COLOR_AQUAMARINE);			
		mapColor.SetLong(kCSS_XML_COLOR_AZURE				,			(sLONG)kCSS_COLOR_AZURE);			
		mapColor.SetLong(kCSS_XML_COLOR_BEIGE				,			(sLONG)kCSS_COLOR_BEIGE);		
		mapColor.SetLong(kCSS_XML_COLOR_BISQUE				,			(sLONG)kCSS_COLOR_BISQUE);		
		mapColor.SetLong(kCSS_XML_COLOR_BLACK				,			(sLONG)kCSS_COLOR_BLACK);		
		mapColor.SetLong(kCSS_XML_COLOR_BLANCHEDALMOND		,			(sLONG)kCSS_COLOR_BLANCHEDALMOND);		
		mapColor.SetLong(kCSS_XML_COLOR_BLUE				,			(sLONG)kCSS_COLOR_BLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_BLUEVIOLET			,			(sLONG)kCSS_COLOR_BLUEVIOLET);		
		mapColor.SetLong(kCSS_XML_COLOR_BROWN				,			(sLONG)kCSS_COLOR_BROWN);			
		mapColor.SetLong(kCSS_XML_COLOR_BURLYWOOD			,			(sLONG)kCSS_COLOR_BURLYWOOD);		
		mapColor.SetLong(kCSS_XML_COLOR_CADETBLUE			,			(sLONG)kCSS_COLOR_CADETBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_CHARTREUSE			,			(sLONG)kCSS_COLOR_CHARTREUSE);		
		mapColor.SetLong(kCSS_XML_COLOR_CHOCOLATE			,			(sLONG)kCSS_COLOR_CHOCOLATE);			
		mapColor.SetLong(kCSS_XML_COLOR_CORAL				,			(sLONG)kCSS_COLOR_CORAL);		
		mapColor.SetLong(kCSS_XML_COLOR_CORNFLOWERBLUE		,			(sLONG)kCSS_COLOR_CORNFLOWERBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_CORNSILK			,			(sLONG)kCSS_COLOR_CORNSILK);		
		mapColor.SetLong(kCSS_XML_COLOR_CRIMSON				,			(sLONG)kCSS_COLOR_CRIMSON);		
		mapColor.SetLong(kCSS_XML_COLOR_CYAN				,			(sLONG)kCSS_COLOR_CYAN);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKBLUE			,			(sLONG)kCSS_COLOR_DARKBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKCYAN			,			(sLONG)kCSS_COLOR_DARKCYAN);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKGOLDENROD		,			(sLONG)kCSS_COLOR_DARKGOLDENROD);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKGRAY			,			(sLONG)kCSS_COLOR_DARKGRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKGREEN			,			(sLONG)kCSS_COLOR_DARKGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKGREY			,			(sLONG)kCSS_COLOR_DARKGREY);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKKHAKI			,			(sLONG)kCSS_COLOR_DARKKHAKI);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKMAGENTA			,			(sLONG)kCSS_COLOR_DARKMAGENTA);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKOLIVEGREEN		,			(sLONG)kCSS_COLOR_DARKOLIVEGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKORANGE			,			(sLONG)kCSS_COLOR_DARKORANGE);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKORCHID			,			(sLONG)kCSS_COLOR_DARKORCHID);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKRED				,			(sLONG)kCSS_COLOR_DARKRED);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKSALMON			,			(sLONG)kCSS_COLOR_DARKSALMON);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKSEAGREEN		,			(sLONG)kCSS_COLOR_DARKSEAGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKSLATEBLUE		,			(sLONG)kCSS_COLOR_DARKSLATEBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKSLATEGRAY		,			(sLONG)kCSS_COLOR_DARKSLATEGRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_DARKSLATEGREY		,			(sLONG)kCSS_COLOR_DARKSLATEGREY);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKTURQUOISE		,			(sLONG)kCSS_COLOR_DARKTURQUOISE);			
		mapColor.SetLong(kCSS_XML_COLOR_DARKVIOLET			,			(sLONG)kCSS_COLOR_DARKVIOLET);		
		mapColor.SetLong(kCSS_XML_COLOR_DEEPPINK			,			(sLONG)kCSS_COLOR_DEEPPINK);		
		mapColor.SetLong(kCSS_XML_COLOR_DEEPSKYBLUE			,			(sLONG)kCSS_COLOR_DEEPSKYBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_DIMGRAY				,			(sLONG)kCSS_COLOR_DIMGRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_DIMGREY				,			(sLONG)kCSS_COLOR_DIMGREY);			
		mapColor.SetLong(kCSS_XML_COLOR_DODGERBLUE			,			(sLONG)kCSS_COLOR_DODGERBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_FIREBRICK			,			(sLONG)kCSS_COLOR_FIREBRICK);			
		mapColor.SetLong(kCSS_XML_COLOR_FLORALWHITE			,			(sLONG)kCSS_COLOR_FLORALWHITE);		
		mapColor.SetLong(kCSS_XML_COLOR_FORESTGREEN			,			(sLONG)kCSS_COLOR_FORESTGREEN);			
		mapColor.SetLong(kCSS_XML_COLOR_FUCHSIA				,			(sLONG)kCSS_COLOR_FUCHSIA);		
		mapColor.SetLong(kCSS_XML_COLOR_GAINSBORO			,			(sLONG)kCSS_COLOR_GAINSBORO);			
		mapColor.SetLong(kCSS_XML_COLOR_GHOSTWHITE			,			(sLONG)kCSS_COLOR_GHOSTWHITE);		
		mapColor.SetLong(kCSS_XML_COLOR_GOLD				,			(sLONG)kCSS_COLOR_GOLD);		
		mapColor.SetLong(kCSS_XML_COLOR_GOLDENROD			,			(sLONG)kCSS_COLOR_GOLDENROD);		
		mapColor.SetLong(kCSS_XML_COLOR_GRAY				,			(sLONG)kCSS_COLOR_GRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_GREY				,			(sLONG)kCSS_COLOR_GREY);			
		mapColor.SetLong(kCSS_XML_COLOR_GREEN				,			(sLONG)kCSS_COLOR_GREEN);			
		mapColor.SetLong(kCSS_XML_COLOR_GREENYELLOW			,			(sLONG)kCSS_COLOR_GREENYELLOW);		
		mapColor.SetLong(kCSS_XML_COLOR_HONEYDEW			,			(sLONG)kCSS_COLOR_HONEYDEW);		
		mapColor.SetLong(kCSS_XML_COLOR_HOTPINK				,			(sLONG)kCSS_COLOR_HOTPINK);		
		mapColor.SetLong(kCSS_XML_COLOR_INDIANRED			,			(sLONG)kCSS_COLOR_INDIANRED);		
		mapColor.SetLong(kCSS_XML_COLOR_INDIGO				,			(sLONG)kCSS_COLOR_INDIGO);			
		mapColor.SetLong(kCSS_XML_COLOR_IVORY				,			(sLONG)kCSS_COLOR_IVORY);		
		mapColor.SetLong(kCSS_XML_COLOR_KHAKI				,			(sLONG)kCSS_COLOR_KHAKI);			
		mapColor.SetLong(kCSS_XML_COLOR_LAVENDER			,			(sLONG)kCSS_COLOR_LAVENDER);		
		mapColor.SetLong(kCSS_XML_COLOR_LAVENDERBLUSH		,			(sLONG)kCSS_COLOR_LAVENDERBLUSH);			
		mapColor.SetLong(kCSS_XML_COLOR_LAWNGREEN			,			(sLONG)kCSS_COLOR_LAWNGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_LEMONCHIFFON		,			(sLONG)kCSS_COLOR_LEMONCHIFFON);			
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTBLUE			,			(sLONG)kCSS_COLOR_LIGHTBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTCORAL			,			(sLONG)kCSS_COLOR_LIGHTCORAL);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTCYAN			,			(sLONG)kCSS_COLOR_LIGHTCYAN);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTGOLDENRODYELLOW,			(sLONG)kCSS_COLOR_LIGHTGOLDENRODYELLOW);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTGRAY			,			(sLONG)kCSS_COLOR_LIGHTGRAY);			
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTGREEN			,			(sLONG)kCSS_COLOR_LIGHTGREEN);			
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTGREY			,			(sLONG)kCSS_COLOR_LIGHTGREY);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTPINK			,			(sLONG)kCSS_COLOR_LIGHTPINK);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSALMON			,			(sLONG)kCSS_COLOR_LIGHTSALMON);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSEAGREEN		,			(sLONG)kCSS_COLOR_LIGHTSEAGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSKYBLUE		,			(sLONG)kCSS_COLOR_LIGHTSKYBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSLATEGRAY		,			(sLONG)kCSS_COLOR_LIGHTSLATEGRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSLATEGREY		,			(sLONG)kCSS_COLOR_LIGHTSLATEGREY);			
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTSTEELBLUE		,			(sLONG)kCSS_COLOR_LIGHTSTEELBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_LIGHTYELLOW			,			(sLONG)kCSS_COLOR_LIGHTYELLOW);			
		mapColor.SetLong(kCSS_XML_COLOR_LIME				,			(sLONG)kCSS_COLOR_LIME);		
		mapColor.SetLong(kCSS_XML_COLOR_LIMEGREEN			,			(sLONG)kCSS_COLOR_LIMEGREEN);			
		mapColor.SetLong(kCSS_XML_COLOR_LINEN				,			(sLONG)kCSS_COLOR_LINEN);		
		mapColor.SetLong(kCSS_XML_COLOR_MAGENTA				,			(sLONG)kCSS_COLOR_MAGENTA);		
		mapColor.SetLong(kCSS_XML_COLOR_MAROON				,			(sLONG)kCSS_COLOR_MAROON);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMAQUAMARINE	,			(sLONG)kCSS_COLOR_MEDIUMAQUAMARINE);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMBLUE			,			(sLONG)kCSS_COLOR_MEDIUMBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMORCHID		,			(sLONG)kCSS_COLOR_MEDIUMORCHID);			
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMPURPLE		,			(sLONG)kCSS_COLOR_MEDIUMPURPLE);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMSEAGREEN		,			(sLONG)kCSS_COLOR_MEDIUMSEAGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMSLATEBLUE		,			(sLONG)kCSS_COLOR_MEDIUMSLATEBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMSPRINGGREEN	,			(sLONG)kCSS_COLOR_MEDIUMSPRINGGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMTURQUOISE		,			(sLONG)kCSS_COLOR_MEDIUMTURQUOISE);			
		mapColor.SetLong(kCSS_XML_COLOR_MEDIUMVIOLETRED		,			(sLONG)kCSS_COLOR_MEDIUMVIOLETRED);		
		mapColor.SetLong(kCSS_XML_COLOR_MIDNIGHTBLUE		,			(sLONG)kCSS_COLOR_MIDNIGHTBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_MINTCREAM			,			(sLONG)kCSS_COLOR_MINTCREAM);		
		mapColor.SetLong(kCSS_XML_COLOR_MISTYROSE			,			(sLONG)kCSS_COLOR_MISTYROSE);			
		mapColor.SetLong(kCSS_XML_COLOR_MOCCASIN			,			(sLONG)kCSS_COLOR_MOCCASIN);		
		mapColor.SetLong(kCSS_XML_COLOR_NAVAJOWHITE			,			(sLONG)kCSS_COLOR_NAVAJOWHITE);			
		mapColor.SetLong(kCSS_XML_COLOR_NAVY				,			(sLONG)kCSS_COLOR_NAVY);		
		mapColor.SetLong(kCSS_XML_COLOR_OLDLACE				,			(sLONG)kCSS_COLOR_OLDLACE);		
		mapColor.SetLong(kCSS_XML_COLOR_OLIVE				,			(sLONG)kCSS_COLOR_OLIVE);		
		mapColor.SetLong(kCSS_XML_COLOR_OLIVEDRAB			,			(sLONG)kCSS_COLOR_OLIVEDRAB);		
		mapColor.SetLong(kCSS_XML_COLOR_ORANGE				,			(sLONG)kCSS_COLOR_ORANGE);			
		mapColor.SetLong(kCSS_XML_COLOR_ORANGERED			,			(sLONG)kCSS_COLOR_ORANGERED);			
		mapColor.SetLong(kCSS_XML_COLOR_ORCHID				,			(sLONG)kCSS_COLOR_ORCHID);		
		mapColor.SetLong(kCSS_XML_COLOR_PALEGOLDENROD		,			(sLONG)kCSS_COLOR_PALEGOLDENROD);		
		mapColor.SetLong(kCSS_XML_COLOR_PALEGREEN			,			(sLONG)kCSS_COLOR_PALEGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_PALETURQUOISE		,			(sLONG)kCSS_COLOR_PALETURQUOISE);		
		mapColor.SetLong(kCSS_XML_COLOR_PALEVIOLETRED		,			(sLONG)kCSS_COLOR_PALEVIOLETRED);			
		mapColor.SetLong(kCSS_XML_COLOR_PAPAYAWHIP			,			(sLONG)kCSS_COLOR_PAPAYAWHIP);		
		mapColor.SetLong(kCSS_XML_COLOR_PEACHPUFF			,			(sLONG)kCSS_COLOR_PEACHPUFF);			
		mapColor.SetLong(kCSS_XML_COLOR_PERU				,			(sLONG)kCSS_COLOR_PERU);		
		mapColor.SetLong(kCSS_XML_COLOR_PINK				,			(sLONG)kCSS_COLOR_PINK);			
		mapColor.SetLong(kCSS_XML_COLOR_PLUM				,			(sLONG)kCSS_COLOR_PLUM);		
		mapColor.SetLong(kCSS_XML_COLOR_POWDERBLUE			,			(sLONG)kCSS_COLOR_POWDERBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_PURPLE				,			(sLONG)kCSS_COLOR_PURPLE);		
		mapColor.SetLong(kCSS_XML_COLOR_RED					,			(sLONG)kCSS_COLOR_RED);		
		mapColor.SetLong(kCSS_XML_COLOR_ROSYBROWN			,			(sLONG)kCSS_COLOR_ROSYBROWN);		
		mapColor.SetLong(kCSS_XML_COLOR_ROYALBLUE			,			(sLONG)kCSS_COLOR_ROYALBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_SADDLEBROWN			,			(sLONG)kCSS_COLOR_SADDLEBROWN);			
		mapColor.SetLong(kCSS_XML_COLOR_SALMON				,			(sLONG)kCSS_COLOR_SALMON);			
		mapColor.SetLong(kCSS_XML_COLOR_SANDYBROWN			,			(sLONG)kCSS_COLOR_SANDYBROWN);		
		mapColor.SetLong(kCSS_XML_COLOR_SEAGREEN			,			(sLONG)kCSS_COLOR_SEAGREEN);		
		mapColor.SetLong(kCSS_XML_COLOR_SEASHELL			,			(sLONG)kCSS_COLOR_SEASHELL);		
		mapColor.SetLong(kCSS_XML_COLOR_SIENNA				,			(sLONG)kCSS_COLOR_SIENNA);		
		mapColor.SetLong(kCSS_XML_COLOR_SILVER				,			(sLONG)kCSS_COLOR_SILVER);			
		mapColor.SetLong(kCSS_XML_COLOR_SKYBLUE				,			(sLONG)kCSS_COLOR_SKYBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_SLATEBLUE			,			(sLONG)kCSS_COLOR_SLATEBLUE);			
		mapColor.SetLong(kCSS_XML_COLOR_SLATEGRAY			,			(sLONG)kCSS_COLOR_SLATEGRAY);		
		mapColor.SetLong(kCSS_XML_COLOR_SLATEGREY			,			(sLONG)kCSS_COLOR_SLATEGREY);			
		mapColor.SetLong(kCSS_XML_COLOR_SNOW				,			(sLONG)kCSS_COLOR_SNOW);		
		mapColor.SetLong(kCSS_XML_COLOR_SPRINGGREEN			,			(sLONG)kCSS_COLOR_SPRINGGREEN);			
		mapColor.SetLong(kCSS_XML_COLOR_STEELBLUE			,			(sLONG)kCSS_COLOR_STEELBLUE);		
		mapColor.SetLong(kCSS_XML_COLOR_TAN					,			(sLONG)kCSS_COLOR_TAN);		
		mapColor.SetLong(kCSS_XML_COLOR_TEAL				,			(sLONG)kCSS_COLOR_TEAL);		
		mapColor.SetLong(kCSS_XML_COLOR_THISTLE				,			(sLONG)kCSS_COLOR_THISTLE);		
		mapColor.SetLong(kCSS_XML_COLOR_TOMATO				,			(sLONG)kCSS_COLOR_TOMATO);			
		mapColor.SetLong(kCSS_XML_COLOR_TURQUOISE			,			(sLONG)kCSS_COLOR_TURQUOISE);			
		mapColor.SetLong(kCSS_XML_COLOR_VIOLET				,			(sLONG)kCSS_COLOR_VIOLET);		
		mapColor.SetLong(kCSS_XML_COLOR_WHEAT				,			(sLONG)kCSS_COLOR_WHEAT);		
		mapColor.SetLong(kCSS_XML_COLOR_WHITE				,			(sLONG)kCSS_COLOR_WHITE);		
		mapColor.SetLong(kCSS_XML_COLOR_WHITESMOKE			,			(sLONG)kCSS_COLOR_WHITESMOKE);		
		mapColor.SetLong(kCSS_XML_COLOR_YELLOW				,			(sLONG)kCSS_COLOR_YELLOW);			
		mapColor.SetLong(kCSS_XML_COLOR_YELLOWGREEN			,			(sLONG)kCSS_COLOR_YELLOWGREEN);		
	}	

	VString colorToken = inColorCSS;
	colorToken.ToUpperCase();
	uLONG colorARGB;
	if (mapColor.GetLong( colorToken, colorARGB))
	{
		*outColorARGB = colorARGB;
		return true;
	}	
	return false;
}

/** parse CSS hexadecimal color */
bool VCSSUtil::ParseColorHexa( const VString& inColorCSS, uLONG *outColorARGB)
{
	if ((inColorCSS.GetLength() != 3)
		&&
		(inColorCSS.GetLength() != 6))
		return false;

	const UniChar *c = inColorCSS.GetCPointer();
	uLONG red, green, blue;
	if (inColorCSS.GetLength() == 3)
	{
		//packed version

		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		red = VCSSUtil::HexaToDecimal( *c);
		red = red*16+red;
		c++;
		
		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		green = VCSSUtil::HexaToDecimal( *c);
		green = green*16+green;
		c++;

		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		blue = VCSSUtil::HexaToDecimal( *c);
		blue = blue*16+blue;
	}
	else
	{
		//unpacked version

		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		red = VCSSUtil::HexaToDecimal( *c);
		c++;
		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		red = red*16+VCSSUtil::HexaToDecimal( *c);
		c++;

		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		green = VCSSUtil::HexaToDecimal( *c);
		c++;
		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		green = green*16+VCSSUtil::HexaToDecimal( *c);
		c++;

		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		blue = VCSSUtil::HexaToDecimal( *c);
		c++;
		if (!VCSSUtil::isHexadecimal( *c))
			return false;
		blue = blue*16+VCSSUtil::HexaToDecimal( *c);
	}
	*outColorARGB = CSSMakeARGB(red,green,blue);
	return true;
}


/** parse CSS color value 
@param inColorCSS
	CSS color value as string
@param outColor
	decoded color as uLONG packed ARGB
*/
bool VCSSUtil::ParseColor( const VString& inColorCSS, uLONG *outColorARGB)
{
	xbox_assert(outColorARGB);

	VString sColor;
	if (inColorCSS.BeginsWith(CVSTR("rgb(")))
	{
		//parse color value formatted as a list of 3 numbers

		inColorCSS.GetSubString( 5, inColorCSS.GetLength()-5, sColor);
		std::vector<VString> vRGB;
		Real rgb[3];
		if (ParseValueListDelim( sColor, vRGB, ',', 3))
		{
			for (int i = 0; i < (int)vRGB.size(); i++)
			{
				if (!ParseColorComponentValue( vRGB[i], rgb[i]))
					return false;
			}
			*outColorARGB = CSSMakeARGB(rgb[0],rgb[1],rgb[2]);
			return true;
		}	
	}	
	else if (inColorCSS.BeginsWith(CVSTR("#")))
	{
		//parse color value formatted as a hexadecimal value

		inColorCSS.GetSubString( 2, inColorCSS.GetLength()-1, sColor);
		sColor.RemoveWhiteSpaces();
		if (ParseColorHexa( sColor, outColorARGB))
			return true;
		else
			return false;
	}
	else
		return ParseColorIdent( inColorCSS, outColorARGB);
	return false;
}


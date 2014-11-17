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
#ifndef __VXMLErrors__
#define __VXMLErrors__


BEGIN_TOOLBOX_NAMESPACE

const OsType	kCOMPONENT_XML	= 'xmlc';

const VError VE_XML_ActionError = MAKE_VERROR(kCOMPONENT_XML,1);
const VError VE_XML_ParsingError = MAKE_VERROR(kCOMPONENT_XML,2);
const VError VE_XML_ParsingFatalError = MAKE_VERROR(kCOMPONENT_XML,3);

//CSS parsing errors
DECLARE_VERROR( kCOMPONENT_XML, 500, VE_CSS_LEX_PARSER_UNKNOWN_ERROR)
DECLARE_VERROR( kCOMPONENT_XML, 501, VE_CSS_LEX_PARSER_BAD_COMMENT)
DECLARE_VERROR( kCOMPONENT_XML, 502, VE_CSS_LEX_PARSER_BAD_CDO)
DECLARE_VERROR( kCOMPONENT_XML, 503, VE_CSS_LEX_PARSER_BAD_CDC)
DECLARE_VERROR( kCOMPONENT_XML, 504, VE_CSS_LEX_PARSER_BAD_DASHMATCH)
DECLARE_VERROR( kCOMPONENT_XML, 505, VE_CSS_LEX_PARSER_BAD_INCLUDES)
DECLARE_VERROR( kCOMPONENT_XML, 506, VE_CSS_LEX_PARSER_BAD_HASH)
DECLARE_VERROR( kCOMPONENT_XML, 507, VE_CSS_LEX_PARSER_BAD_IMPORTANT_SYMBOL)
DECLARE_VERROR( kCOMPONENT_XML, 508, VE_CSS_LEX_PARSER_BAD_DECLARATION)
DECLARE_VERROR( kCOMPONENT_XML, 509, VE_CSS_LEX_PARSER_BAD_ESCAPE)
DECLARE_VERROR( kCOMPONENT_XML, 510, VE_CSS_LEX_PARSER_BAD_STRING)
DECLARE_VERROR( kCOMPONENT_XML, 511, VE_CSS_LEX_PARSER_BAD_ATKEYWORD)
DECLARE_VERROR( kCOMPONENT_XML, 512, VE_CSS_LEX_PARSER_BAD_URI)

DECLARE_VERROR( kCOMPONENT_XML, 550, VE_CSS_PARSER_URL_FAILED)
DECLARE_VERROR( kCOMPONENT_XML, 551, VE_CSS_PARSER_BAD_IMPORT_RULE)
DECLARE_VERROR( kCOMPONENT_XML, 552, VE_CSS_PARSER_BAD_MEDIA_RULE)
DECLARE_VERROR( kCOMPONENT_XML, 553, VE_CSS_PARSER_BAD_RULESET)
DECLARE_VERROR( kCOMPONENT_XML, 554, VE_CSS_PARSER_BAD_ATTRIBUTE_CONDITION)
DECLARE_VERROR( kCOMPONENT_XML, 555, VE_CSS_PARSER_BAD_DECLARATION)
DECLARE_VERROR( kCOMPONENT_XML, 556, VE_CSS_PARSER_BAD_BLOCK_END)
DECLARE_VERROR( kCOMPONENT_XML, 557, VE_CSS_PARSER_BAD_LANG_PSEUDO_CLASS)



END_TOOLBOX_NAMESPACE

#endif


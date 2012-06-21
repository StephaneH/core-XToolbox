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
#ifndef __VKernelErrors__
#define __VKernelErrors__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

// don't know why gcc complains about shortening 64-bit value into a 32-bit value only for this (0,0) combination
//DECLARE_VERROR( kCOMPONENT_SYSTEM, 0, VE_OK)

const VError VE_OK = 0;

// Generic Errors: 1->99
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 1, VE_UNKNOWN_ERROR)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 2, VE_UNIMPLEMENTED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 3, VE_INVALID_PARAMETER)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 4, VE_CANNOT_INITIALIZE_APPLICATION)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 5, VE_ACCESS_DENIED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 6, VE_USER_ABORT)

// Memory Errors: 100->199
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 100, VE_MEMORY_FULL)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 102, VE_NOT_A_HANDLE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 103, VE_HANDLE_LOCKED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 104, VE_CANNOT_LOCK_HANDLE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 105, VE_HANDLE_NOT_LOCKED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 106, VE_STRING_RESIZE_FIXED_STRING)	// Tried to resize a fixed size string
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 107, VE_STRING_ALLOC_FAILED)	// Failed to grow a string

// Stream Errors: 200->299
// common to VFileDesc and VEndPoint
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 200, VE_STREAM_EOF)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 201, VE_STREAM_NOT_OPENED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 202, VE_STREAM_ALREADY_OPENED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 203, VE_STREAM_CANNOT_READ)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 204, VE_STREAM_CANNOT_WRITE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 205, VE_STREAM_CANNOT_FIND_SOURCE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 206, VE_STREAM_USER_ABORTED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 207, VE_STREAM_BAD_SIGNATURE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 208, VE_STREAM_NO_TEXT_CONVERTER)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 210, VE_STREAM_TEXT_CONVERSION_FAILURE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 211, VE_STREAM_TOO_MANY_UNGET_DATA)	// Trying to unget more data than what has been read
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 212, VE_STREAM_BAD_VERSION)	// found an incompatible version number while reading from a stream
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 213, VE_STREAM_CANNOT_GET_POS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 214, VE_STREAM_CANNOT_SET_POS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 215, VE_STREAM_CANNOT_GET_DATA)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 216, VE_STREAM_CANNOT_PUT_DATA)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 217, VE_STREAM_CANNOT_FLUSH)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 218, VE_STREAM_CANNOT_GET_SIZE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 219, VE_STREAM_CANNOT_SET_SIZE)

// International Utilities Errors: 300->399
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 300, VE_INTL_TEXT_CONVERSION_FAILED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 301, VE_OS_TYPE_INSERTION_FAILED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 302, VE_FILE_EXT_INSERTION_FAILED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 303, VE_ICU_LOAD_FAILED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 304, VE_REGEX_FAILED)
	
// Resource Errors: 400->499
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 400, VE_RES_MAP_INVALID)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 401, VE_RES_NOT_FOUND)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 402, VE_RES_INDEX_OUT_OF_RANGE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 403, VE_RES_NOT_A_RESOURCE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 404, VE_RES_DUPLICATE_ID)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 405, VE_RES_NOT_SWAPPED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 406, VE_RES_ALREADY_EXISTS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 407, VE_RES_FILE_NOT_OPENED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 408, VE_RES_FILE_LOCKED)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 409, VE_RES_FILE_ALREADY_OPENED)

// XML/ValueBags Errors: 500->599
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 500, VE_CANNOT_LOAD_XML_DESCRIPTION)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 501, VE_MALFORMED_XML_DESCRIPTION)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 502, VE_CANNOT_SAVE_XML_DESCRIPTION)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 550, VE_MALFORMED_JSON_DESCRIPTION)

// File Manager Errors: 600->699
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 600, VE_FILE_NOT_FOUND)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 601, VE_FILE_CANNOT_RENAME)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 602, VE_FILE_CANNOT_OPEN)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 603, VE_FILE_CANNOT_CLOSE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 604, VE_FILE_CANNOT_DELETE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 605, VE_FILE_CANNOT_COPY)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 606, VE_FILE_CANNOT_MOVE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 607, VE_FILE_CANNOT_CREATE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 608, VE_FILE_CANNOT_GET_ATTRIBUTES)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 609, VE_FILE_CANNOT_SET_ATTRIBUTES)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 610, VE_FILE_CANNOT_CREATE_ALIAS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 611, VE_FILE_CANNOT_RESOLVE_ALIAS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 612, VE_FILE_CANNOT_REVEAL)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 613, VE_FILE_BAD_KIND)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 614, VE_FILE_BAD_NAME)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 615, VE_FILE_CANNOT_EXCHANGE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 616, VE_FILE_ALREADY_EXISTS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 617, VE_FILE_CANNOT_RESOLVE_ALIAS_TO_FILE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 618, VE_FILE_CANNOT_RESOLVE_ALIAS_TO_FOLDER)

DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 650, VE_FOLDER_NOT_FOUND)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 651, VE_FOLDER_NOT_EMPTY)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 652, VE_FOLDER_CANNOT_CREATE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 653, VE_FOLDER_CANNOT_DELETE)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 654, VE_FOLDER_CANNOT_GET_ATTRIBUTES)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 655, VE_FOLDER_CANNOT_SET_ATTRIBUTES)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 656, VE_FOLDER_CANNOT_REVEAL)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 657, VE_FOLDER_CANNOT_RENAME)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 658, VE_DISK_FULL)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 659, VE_FOLDER_ALREADY_EXISTS)
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 660, VE_CANNOT_COPY_ON_ITSELF)

// Lexer errors
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 700, VE_LEXER_NON_TERMINATED_STRING )
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 701, VE_LEXER_NON_TERMINATED_COMMENT )

// VFullURL errors
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 800, expecting_closing_single_quote )
DECLARE_VERROR( kCOMPONENT_XTOOLBOX, 801, expecting_closing_double_quote )


END_TOOLBOX_NAMESPACE

#endif

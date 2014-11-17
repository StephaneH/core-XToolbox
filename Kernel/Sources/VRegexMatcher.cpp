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

#if USE_ICU
#include "unicode/regex.h"

#include "VString.h"
#include "VValueBag.h"
#include "VError.h"
#include "VRegexMatcher.h"
#include "VIntlMgr.h"

BEGIN_TOOLBOX_NAMESPACE

static void _CheckError( const VString& inPattern, UErrorCode inStatus, VError *outError)
{
	if (outError != NULL)
	{
		if (U_SUCCESS( inStatus))
			*outError = VE_OK;
		else
		{
			StThrowError<> errThrow( VE_REGEX_FAILED);

			// retreive clear error message
			const char *icu_message = u_errorName( inStatus);
			errThrow->SetString( "message", VString( icu_message));

			// retreive pattern
			errThrow->SetString( "pattern", inPattern);
			
			*outError = VE_REGEX_FAILED;
		}
	}
}


VRegexMatcher::~VRegexMatcher()
{
	delete fMatcher;
}


VRegexMatcher *VRegexMatcher::Create( const VString& inPattern, VError *outError)
{
	
	VRegexMatcher *matcher=NULL;
	
    if (VIntlMgr::ICU_Available()) {
        UErrorCode status = U_ZERO_ERROR;
        
        xbox_icu::UnicodeString string( FALSE, inPattern.GetCPointer(), inPattern.GetLength());
        RegexMatcher *icu_matcher = new RegexMatcher( string, 0 /*flags*/, status);

        if ( !U_SUCCESS( status) )
        {
            _CheckError( inPattern, status, outError);

            delete icu_matcher;
            icu_matcher = NULL;
            matcher = NULL;
        }
        else
        {
            matcher = new VRegexMatcher( inPattern, icu_matcher);
        }
    }
	return matcher;
}


bool VRegexMatcher::Find( const VString& inText, VIndex inStart, bool inContinueSearching, VError *outError)
{
	UBool found=0;
    
    if (VIntlMgr::ICU_Available()) {
        UErrorCode status = U_ZERO_ERROR;

        fMatcher->reset( xbox_icu::UnicodeString( FALSE, inText.GetCPointer(), inText.GetLength()));
	
        if (inContinueSearching)
            found = fMatcher->find( inStart - 1, status);
        else
            found = fMatcher->lookingAt( inStart - 1, status);

        _CheckError( fPattern, status, outError);
	}
    
	return (found != 0);
}


VIndex VRegexMatcher::GetGroupCount() const
{
	return fMatcher->groupCount();
}


VIndex VRegexMatcher::GetGroupStart( VIndex inGroupIndex) const
{
	UErrorCode status = U_ZERO_ERROR;
	sLONG start = fMatcher->start( inGroupIndex, status);
	xbox_assert( U_SUCCESS( status));
	
	return (start >= 0) ? (start + 1) : start;
}


VIndex VRegexMatcher::GetGroupLength( VIndex inGroupIndex) const
{
	UErrorCode status = U_ZERO_ERROR;
	sLONG start = fMatcher->start( inGroupIndex, status);
	sLONG end = fMatcher->end( inGroupIndex, status);
	xbox_assert( U_SUCCESS( status));

	return (end >= 0) ? (end - start) : 0;
}


bool VRegexMatcher::IsSamePattern( const VString& inPattern) const
{
	if (inPattern.GetCPointer() == fPattern.GetCPointer())	// shortcut thanks to refcounting
		return true;
	return (inPattern.GetLength() == fPattern.GetLength()) && (::memcmp( inPattern.GetCPointer(), fPattern.GetCPointer(), inPattern.GetLength()*sizeof(UniChar)) == 0);
}

END_TOOLBOX_NAMESPACE

#endif

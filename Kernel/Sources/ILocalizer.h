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
#ifndef __ILocalizer__
#define __ILocalizer__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

class VString;
class VValueBag;

/*
	@brief pure interface for localization purpose.
	generally implemented using VLocalizationManager.
*/
class XTOOLBOX_API ILocalizer
{
public:
	/**
	* @brief Returns the localization language used by the localization manager.
	* You can get the localization language but not set it. If you want to set it, destroy the localization manager and create another one.
	* @return the current used language
	*/
	virtual DialectCode			GetLocalizationLanguage() = 0;

	virtual	bool				LocalizeErrorMessage( VError inErrorCode, VString& outLocalizedMessage) = 0;

	virtual	bool				LocalizeStringWithKey( const VString& inKeyToLookUp, VString& outLocalizedString) = 0;
	
	virtual bool				LocalizeStringWithDotStringsKey( const VString& inKeyToLookUp, VString& outLocalizedString) = 0;

	virtual	const VValueBag*	RetainGroupBag( const VString& inGroupResName) = 0;
};


XTOOLBOX_API const VString LocalizeString( const char *inResourceName, const char *inDefault);

END_TOOLBOX_NAMESPACE


#endif

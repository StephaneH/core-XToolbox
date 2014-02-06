
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

//in order to be able to use std::min && std::max
#undef max
#undef min

USING_TOOLBOX_NAMESPACE

VString	VDocImage::sElementName;

/** get image source (url) */
const VString& VDocImage::GetSource() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_IMG_SOURCE); }

/** set image source (url) */
void VDocImage::SetSource( const VString& inSource) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_IMG_SOURCE, inSource); }

/** get image alternate text */
const VString& VDocImage::GetText() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_IMG_ALT_TEXT); }

/** set image alternate text */
void VDocImage::SetText( const VString& inText) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_IMG_ALT_TEXT, inText); }

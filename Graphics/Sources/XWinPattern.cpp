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
#include "VGraphicsPrecompiled.h"
#include "VPattern.h"


void VStandardPattern::_Release()
{
	// Nothing to do
}


void VStandardPattern::DoApplyPattern(ContextRef inContext, FillRuleType) const
{
	// Standard pattern handling is done by VGraphicContext
	if (fPatternRef == NULL)
		fPatternRef = VWinGDIGraphicContext::_CreateStandardPattern(inContext, fPatternStyle, (void*)fBuffer);
	
	VWinGDIGraphicContext::_ApplyPattern(inContext, fPatternStyle, fPatternColor, fPatternRef);
}


void VStandardPattern::DoReleasePattern(ContextRef inContext) const
{
	if (fPatternRef != NULL)
	{
		VWinGDIGraphicContext::_ReleasePattern(inContext, fPatternRef);
		fPatternRef = NULL;
	}
}


HBRUSH VStandardPattern::WIN_CreateBrush() const
{
	return (HBRUSH) VWinGDIGraphicContext::_CreateStandardPattern(NULL, fPatternStyle, (void*)fBuffer);
}


#pragma mark-

void VGradientPattern::_Release()
{
	if (fGradientRef != NULL)
	{
		VWinGDIGraphicContext::_ReleaseGradient(fGradientRef);
		fGradientRef = NULL;
	}
}


#pragma mark-

void VAxialGradientPattern::DoApplyPattern(ContextRef inContext, FillRuleType ) const
{
	// Gradient pattern handling is done by VGraphicContext
	if (fGradientRef == NULL)
		fGradientRef = VWinGDIGraphicContext::_CreateAxialGradient(inContext, this);
	
	VWinGDIGraphicContext::_ApplyGradient(inContext, fGradientRef);
}


void VRadialGradientPattern::DoApplyPattern(ContextRef inContext, FillRuleType ) const
{
	// Gradient pattern handling is done by VGraphicContext
	if (fGradientRef == NULL)
		fGradientRef = VWinGDIGraphicContext::_CreateRadialGradient(inContext, this);
	
	VWinGDIGraphicContext::_ApplyGradient(inContext, fGradientRef);
}





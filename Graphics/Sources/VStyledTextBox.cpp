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
#include "VGraphicContext.h"
#include "VRect.h"
#include "vfont.h"
#include "vcolor.h"
#include "VStyledTextBox.h"
#if VERSIONWIN
#include "XWinStyledTextBox.h"
#elif VERSIONMAC
#include "XMacStyledTextBox.h"
#include "XMacFont.h"
#endif


#if VERSIONMAC
VStyledTextBox *VStyledTextBox::CreateImpl(ContextRef inContextRef, ContextRef /*inOutputContextRef*/,
										   const VString& inText, const VTreeTextStyle *inStyles, 
										   const VRect& inHwndBounds, 
										   const VColor& inTextColor, 
										   VFont *inFont, 
										   const TextRenderingMode inMode, 
										   const TextLayoutMode inLayoutMode, 
										   const GReal inRefDocDPI, 
										   const GReal inCharKerning,
										   bool inUseNSAttributes)
#else
VStyledTextBox *VStyledTextBox::CreateImpl(ContextRef inContextRef, ContextRef inOutputContextRef, 
										   const VString& inText, const VTreeTextStyle *inStyles, 
										   const VRect& inHwndBounds, 
										   const VColor& inTextColor, 
										   VFont *inFont, 
										   const TextRenderingMode /*inMode*/, 
										   const TextLayoutMode inLayoutMode, 
										   const GReal inRefDocDPI, 
										   const GReal /*inCharKerning*/,
										   bool /*inUseNSAttributes*/)
#endif
{
#if VERSIONWIN
	return static_cast<VStyledTextBox *>(new XWinStyledTextBox( inContextRef, inOutputContextRef, inText, inStyles, inHwndBounds, inTextColor, inFont, inLayoutMode, inRefDocDPI));
#elif VERSIONMAC
	return static_cast<VStyledTextBox *>(new XMacStyledTextBox( inContextRef, inText, inStyles, inHwndBounds, inTextColor, inFont, inMode, inLayoutMode, inCharKerning, inRefDocDPI, inUseNSAttributes));
#else
	return NULL;
#endif
}


VStyledTextBox *VStyledTextBox::CreateImpl(	ContextRef inContextRef, ContextRef inOutputContextRef,
											const VTextLayout& inTextLayout, 
											const VRect* inBounds, 
											const TextRenderingMode inMode, 
											const VColor* inTextColor, VFont *inFont, 
											const GReal inCharKerning,
											bool inUseNSAttributes)
{
	VRect bounds;
	if (inBounds)
		bounds = *inBounds;
	else
		bounds = VRect(0.0f, 0.0f, inTextLayout.GetMaxWidth() ? inTextLayout.GetMaxWidth() : 100000.0f, inTextLayout.GetMaxHeight() ? inTextLayout.GetMaxHeight() : 100000.0f);
	VColor textColor;
	if (inTextColor)
		textColor = *inTextColor;
	else
		inTextLayout.GetDefaultTextColor( textColor);
	VFont *font = inFont ? RetainRefCountable(inFont) : inTextLayout.RetainDefaultFont();
	xbox_assert(font);
#if VERSIONWIN
	//ensure character background color is only painted by VTextLayout & not by text services (avoid switching to RichTextEdit in order to paint selection back color)
	VTreeTextStyle *styles = inTextLayout.RetainStyles(0,-1,true,false,false);
	VStyledTextBox *textBox = static_cast<VStyledTextBox *>(new XWinStyledTextBox(	inContextRef, inOutputContextRef, inTextLayout.GetText(), styles, bounds, 
											textColor, font, inTextLayout.GetLayoutMode(), inTextLayout.GetDPI(), inTextLayout.GetBackColor(), false));
	ReleaseRefCountable(&styles);
#else
	VTreeTextStyle *styles = inTextLayout.RetainStyles(0,-1,true,false,false);
	VStyledTextBox *textBox = CreateImpl(	inContextRef, inOutputContextRef, inTextLayout.GetText(), styles, bounds, 
											textColor, font, inMode, inTextLayout.GetLayoutMode(), inTextLayout.GetDPI(), inCharKerning, inUseNSAttributes);
	ReleaseRefCountable(&styles);
#endif
	if (inTextLayout.GetTextAlign() != AL_DEFAULT)
		textBox->SetTextAlign( inTextLayout.GetTextAlign());
	if (inTextLayout.GetVerticalAlign() != AL_DEFAULT)
		textBox->SetParaAlign( inTextLayout.GetVerticalAlign());

	/* //will be only fully supported in v15 with new text layout
	if (inTextLayout.GetLineHeight() != -1)
		textBox->SetLineHeight( inTextLayout.GetLineHeight());
	if (inTextLayout.GetTextIndent() != 0.0f)
		textBox->SetTextIndent( 0.0f);
	textBox->SetTabStop( inTextLayout.GetTabStopOffset(), inTextLayout.GetTabStopType());
	*/

	ReleaseRefCountable(&font);
	return textBox;
}

VStyledTextBox::VStyledTextBox(const VString& inText, const VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont)
{
	fInitialized = false;
}

void VStyledTextBox::Initialize(const VString& inText, const VTreeTextStyle *inStyles, const VColor& inTextColor, VFont *inFont)
{
	DoInitialize();
	_SetText(inText);
	_SetFont(inFont, inTextColor);
	_ApplyAllStyles(inStyles, NULL);
	fInitialized = true;
}

Boolean VStyledTextBox::Draw(const VRect& inBounds)
{
	return DoDraw(inBounds);
}


#if !VERSIONMAC	
/** return uniform style on the specified range
@remarks
	return end limit of uniform style
*/
sLONG VStyledTextBox::_GetUniformStyle(VTextStyle *styleUniform, sLONG inStart, sLONG inEnd)
{
	if (inEnd == -1)
		inEnd = GetPlainTextLength();
	if (inStart >= inEnd)
	{
		styleUniform->Reset();
		return inStart;
	}

	bool isUniform = _GetStyle( styleUniform, inStart, inEnd);
	sLONG result = inStart;
	if(isUniform)
		result = inEnd;
	else 
	{
		sLONG start = inStart;
		sLONG maxEnd = inEnd;
		sLONG end = (inStart+inEnd)/2;
		xbox_assert(end > start);
		sLONG middle = inStart;
		while(start < end)
		{
			if (_GetStyle(styleUniform, inStart, end))
			{
				start = end;
				end = maxEnd;
				middle = (start + end)/2;
				if (middle == start)
				{
					end = start;
					break;
				}
				end = middle;
			}
			else
			{
				maxEnd = end-1;
				middle = (start + maxEnd)/2;
				if (middle == start)
					break;
				end = middle;
			}
		}

		if (_GetStyle(styleUniform, inStart, end))
			result =  end;
		else
		{
			if (start <= inStart)
				start = inStart+1;
			xbox_assert(_GetStyle(styleUniform, inStart, start));
			result =  start;
		}
	}
	return result;
}
#endif


sLONG VStyledTextBox::GetAllStyles(std::vector<VTextStyle*>& outStyles, sLONG inStart, sLONG inEnd)
{
	xbox_assert(outStyles.size() == 0);
	if (inEnd == -1)
		inEnd = GetPlainTextLength();

	if (inStart >= inEnd)
		return 0;

	while(inStart < inEnd)
	{
		VTextStyle* styleUniform = new VTextStyle();
		inStart = GetStyle(styleUniform, inStart, inEnd);
		if (!styleUniform->IsUndefined())
			outStyles.push_back( styleUniform);
		else
			delete styleUniform;
	}

	return (sLONG)outStyles.size();
}

void VStyledTextBox::_DoApplyStyleRef( const VTextStyle *inStyle)
{
	if (inStyle->IsSpanRef())
	{
		const VTextStyle *styleRef = inStyle->GetSpanRef()->GetStyle();
		if (styleRef)
		{
			xbox_assert(!styleRef->IsSpanRef()); 

			VTextStyle *style = new VTextStyle( styleRef);
			sLONG start, end;
			inStyle->GetRange( start, end);
			style->SetRange( start, end);
			DoApplyStyle( style);
		}
	}
}

void VStyledTextBox::_ApplyAllStyles(const XBOX::VTreeTextStyle* inStyles, VTextStyle *inStyleInherit)
{
	if(inStyles)
	{
		XBOX::VTextStyle* uniformStyle = inStyles->GetData();

		DoApplyStyle(uniformStyle, inStyleInherit);

		//appliquer les styles enfants

		sLONG count = inStyles->GetChildCount();
		if (count != 0)
		{
			VTextStyle *styleInherit = NULL;
			if (inStyleInherit)
			{
				styleInherit = new VTextStyle( inStyleInherit);
				styleInherit->MergeWithStyle(uniformStyle, NULL);
				
			}
			else
				styleInherit = uniformStyle;
			
			for(sLONG i = 1 ; i <= count ;i++)
			{
				XBOX::VTreeTextStyle* style = inStyles->GetNthChild(i);
				//PushCharFormat();
				_ApplyAllStyles(style, styleInherit);
				//PopCharFormat();
			}
			
			if (inStyleInherit)
				delete styleInherit;
		}
	}
}

void VStyledTextBox::DoOnRefCountZero ()
{
	DoRelease();
	IRefCountable::DoOnRefCountZero();
}

void VStyledTextBox::GetSize(GReal &ioWidth, GReal &outHeight,bool inForPrinting)
{
	DoGetSize(ioWidth, outHeight,inForPrinting);
}


/** set text horizontal alignment (default is AL_DEFAULT) */
void VStyledTextBox::SetTextAlign( AlignStyle inHAlign, sLONG inStart, sLONG inEnd)
{
	//default impl: should be derived in impl to optimize
	if (inEnd == -1)
		inEnd = GetPlainTextLength();
	if (inEnd < inStart)
		return;
	VTextStyle *style = new VTextStyle();
	style->SetJustification( ITextLayout::JustFromAlignStyle( inHAlign));
	style->SetRange(inStart, inEnd);
	ApplyStyle( style);
	delete style;
}

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

ISpanTextRefFactory*	ISpanTextRefFactory::fFactory = NULL;

#if ENABLE_VTEXTSTYLE_EXT
ITextStyleExtFactory*	ITextStyleExtFactory::fFactory = NULL;
#endif

//in order to be able to use std::min && std::max
#undef max
#undef min

VTextStyle::VTextStyle(const VTextStyle* inTextStyle)
{
	_Init( inTextStyle);
}

void VTextStyle::_Init(const VTextStyle *inTextStyle)
{
	fFontName = new VString();
	if(inTextStyle)
	{
		fIsBold = inTextStyle->fIsBold;
		fIsUnderline = inTextStyle->fIsUnderline;
		fIsStrikeout = inTextStyle->fIsStrikeout;
		fIsItalic = inTextStyle->fIsItalic;
		fBegin = inTextStyle->fBegin;
		fEnd = inTextStyle->fEnd;
		fFontSize = inTextStyle->fFontSize;
		*fFontName = *(inTextStyle->fFontName);
		fHasBackColor = inTextStyle->fHasBackColor;
		fHasForeColor = inTextStyle->fHasForeColor;
		fJust = inTextStyle->fJust;
		fColor = inTextStyle->fColor;
		fBkColor = inTextStyle->fBkColor;
		fSpanTextRef = inTextStyle->fSpanTextRef ? inTextStyle->fSpanTextRef->Clone() : NULL;
#if ENABLE_VTEXTSTYLE_EXT
		fStylesExt = inTextStyle->fStylesExt ? inTextStyle->fStylesExt->Clone() : NULL;
#endif
	}
	else
	{
		fIsBold = fIsUnderline = fIsStrikeout = fIsItalic = UNDEFINED_STYLE;
		fBegin = fEnd = 0;
		fFontSize = -1;
		*fFontName = "";
		fColor = 0xFF000000;
		fBkColor = 0xFFFFFFFF;
		fHasBackColor = false;
		fHasForeColor = false;
		fJust = JST_Notset;
		fSpanTextRef = NULL;
#if ENABLE_VTEXTSTYLE_EXT
		fStylesExt = NULL;
#endif
	}
}

VTextStyle::VTextStyle(const VTextStyle& inTextStyle)
{
	_Init( &inTextStyle);
}


VTextStyle& VTextStyle::operator = ( const VTextStyle& inTextStyle)
{
	if (this == &inTextStyle)
		return *this;

	fFontName = new VString(*(inTextStyle.fFontName));
	fIsBold = inTextStyle.fIsBold;
	fIsUnderline = inTextStyle.fIsUnderline;
	fIsStrikeout = inTextStyle.fIsStrikeout;
	fIsItalic = inTextStyle.fIsItalic;
	fBegin = inTextStyle.fBegin;
	fEnd = inTextStyle.fEnd;
	fFontSize = inTextStyle.fFontSize;
	fHasBackColor = inTextStyle.fHasBackColor;
	fHasForeColor = inTextStyle.fHasForeColor;
	fJust = inTextStyle.fJust;
	fColor = inTextStyle.fColor;
	fBkColor = inTextStyle.fBkColor;
	SetSpanRef( NULL);
	if (inTextStyle.fSpanTextRef)
		fSpanTextRef = inTextStyle.fSpanTextRef->Clone();
#if ENABLE_VTEXTSTYLE_EXT
	SetStylesExt( inTextStyle.fStylesExt ? inTextStyle.fStylesExt->Clone() : NULL);
#endif
	return  *this;
}

/** return true if this style & the input style are equal */
bool VTextStyle::IsEqualTo( const VTextStyle& inStyle, bool inIncludeRange) const
{
	if (inIncludeRange)
	{
		if (!(	fBegin == inStyle.fBegin
				&&
				fEnd == inStyle.fEnd
			  )
		   )
		   return false;
	}
	return (fIsBold == inStyle.fIsBold
			&&
			fIsUnderline == inStyle.fIsUnderline
			&&
			fIsStrikeout == inStyle.fIsStrikeout
			&&
			fIsItalic == inStyle.fIsItalic
			&&
			fFontSize == inStyle.fFontSize
			&&
			fHasForeColor == inStyle.fHasForeColor
			&&
			fHasBackColor == inStyle.fHasBackColor
			&&
			(!fHasForeColor || (fHasForeColor && fColor == inStyle.fColor))
			&&
			(!fHasBackColor || (fHasBackColor && fBkColor == inStyle.fBkColor))
			&&
			fJust == inStyle.fJust
			&&
			fFontName->EqualToString( *(inStyle.fFontName))
			&&
			(
			(fSpanTextRef == NULL && inStyle.fSpanTextRef == NULL)
			||
			(fSpanTextRef != NULL && inStyle.fSpanTextRef != NULL && fSpanTextRef->EqualTo( inStyle.fSpanTextRef))
			)
#if ENABLE_VTEXTSTYLE_EXT
			&&
			(
			(fStylesExt == NULL && inStyle.fStylesExt == NULL)
			||
			(fStylesExt == NULL && inStyle.fStylesExt->IsUndefined())
			||
			(inStyle.fStylesExt == NULL && fStylesExt->IsUndefined())
			||
			(fStylesExt != NULL && inStyle.fStylesExt != NULL && fStylesExt->EqualTo( inStyle.fStylesExt))
			)
#endif
			);
}


/** return style which have properties common to this style & the input style 
	and reset properties which are common in the two styles
	(NULL if none property is common)
*/
VTextStyle *VTextStyle::ExtractCommonStyle(VTextStyle *ioStyle2)
{
	if (!ioStyle2)
		return NULL;

	VTextStyle *retStyle = NULL;
	if (fIsBold != UNDEFINED_STYLE && fIsBold == ioStyle2->fIsBold)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsBold = fIsBold;
		fIsBold = UNDEFINED_STYLE;
		ioStyle2->fIsBold = UNDEFINED_STYLE;
	}
	if (fIsUnderline != UNDEFINED_STYLE && fIsUnderline == ioStyle2->fIsUnderline)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsUnderline = fIsUnderline;
		fIsUnderline = UNDEFINED_STYLE;
		ioStyle2->fIsUnderline = UNDEFINED_STYLE;
	}
	if (fIsStrikeout != UNDEFINED_STYLE && fIsStrikeout == ioStyle2->fIsStrikeout)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsStrikeout = fIsStrikeout;
		fIsStrikeout = UNDEFINED_STYLE;
		ioStyle2->fIsStrikeout = UNDEFINED_STYLE;
	}
	if (fIsItalic != UNDEFINED_STYLE && fIsItalic == ioStyle2->fIsItalic)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsItalic = fIsItalic;
		fIsItalic = UNDEFINED_STYLE;
		ioStyle2->fIsItalic = UNDEFINED_STYLE;
	}
	if (fJust != JST_Notset && fJust == ioStyle2->fJust)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fJust = fJust;
		fJust = JST_Notset;
		ioStyle2->fJust = JST_Notset;
	}
	if (fFontSize != UNDEFINED_STYLE && fFontSize == ioStyle2->fFontSize)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fFontSize = fFontSize;
		fFontSize = UNDEFINED_STYLE;
		ioStyle2->fFontSize = UNDEFINED_STYLE;
	}
	if (!fFontName->IsEmpty() && fFontName->EqualToString( *(ioStyle2->fFontName)))
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fFontName->FromString( *fFontName);
		fFontName->SetEmpty();
		ioStyle2->fFontName->SetEmpty();
	}
	if (fHasForeColor && ioStyle2->fHasForeColor && fColor == ioStyle2->fColor)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fHasForeColor = fHasForeColor;
		retStyle->fColor = fColor;

		SetHasForeColor( false);
		ioStyle2->SetHasForeColor( false);
	}
	if (fHasBackColor && ioStyle2->fHasBackColor && fBkColor == ioStyle2->fBkColor)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fHasBackColor = fHasBackColor;
		retStyle->fBkColor = fBkColor;

		SetHasBackColor( false);
		ioStyle2->SetHasBackColor( false);
	}
	return retStyle;
}

VTextStyle::~VTextStyle()
{
#if ENABLE_VTEXTSTYLE_EXT
	if (fStylesExt)
		delete fStylesExt;
#endif
	if (fSpanTextRef)
		delete fSpanTextRef;
	if(fFontName)
		delete fFontName;
}


void VTextStyle::SetFontStyleToNormal(bool inShouldCreateStylesExt)
{
	SetBold(0);
	SetItalic(0);
	SetUnderline(0);
	SetStrikeout(0);
	SetJustification(JST_Default);
#if ENABLE_VTEXTSTYLE_EXT
	if (fStylesExt)
		fStylesExt->SetStyleToNormal();
	else if (inShouldCreateStylesExt && testAssert(ITextStyleExtFactory::Get()))
	{
		fStylesExt = ITextStyleExtFactory::Get()->Create();
		fStylesExt->SetStyleToNormal();
	}
#endif
}

void VTextStyle::SetRange(sLONG inBegin, sLONG inEnd)
{
	fBegin = inBegin; 
	fEnd = inEnd;
}
	
void VTextStyle::GetRange(sLONG& outBegin, sLONG& outEnd) const	
{
	outBegin = fBegin; 
	outEnd = fEnd;
}

void VTextStyle::SetBold(sLONG inValue)													
{
	fIsBold = inValue;
}

sLONG VTextStyle::GetBold() const															
{
	return fIsBold;
}

void VTextStyle::SetItalic(sLONG inValue)												
{
	fIsItalic = inValue;
}

sLONG VTextStyle::GetItalic() const														
{
	return fIsItalic;
}

void VTextStyle::SetUnderline(sLONG inValue)												
{
	fIsUnderline = inValue;
}

sLONG VTextStyle::GetUnderline() const													
{
	return fIsUnderline;
}

void VTextStyle::SetStrikeout(sLONG inValue)												
{
	fIsStrikeout = inValue;
}

sLONG VTextStyle::GetStrikeout() const													
{
	return fIsStrikeout;
}

void VTextStyle::SetColor(const RGBAColor& inValue)									
{
	fColor = inValue;
}

RGBAColor VTextStyle::GetColor() const														
{
	return fColor;
}

void VTextStyle::SetBackGroundColor(const RGBAColor& inValue)							
{
	fBkColor = inValue;
}

RGBAColor VTextStyle::GetBackGroundColor() const												
{
	return fBkColor;
}

void VTextStyle::SetHasBackColor(bool inValue)											
{
	fHasBackColor = inValue;
	if (!fHasBackColor)
		fBkColor = 0xFFFFFFFF;
}

bool VTextStyle::GetHasBackColor() const													
{
	return fHasBackColor;
}

void VTextStyle::SetHasForeColor(bool inValue)											
{
	fHasForeColor = inValue;
	if (!fHasForeColor)
		fColor = 0xFF000000;
}

bool VTextStyle::GetHasForeColor() const													
{
	return fHasForeColor;
}

void VTextStyle::SetFontSize(const Real& inValue)										
{
	fFontSize = inValue;
}

Real VTextStyle::GetFontSize() const													
{
	return fFontSize;
}

void VTextStyle::SetFontName(const VString& inValue)								
{
	//deal with CSS composite font name - ie '"Tahoma", "Sans Serif"' for instance - parsed from HTML or RTF
	VIndex posComma = inValue.Find(",");
	if (posComma >= 1)
	{
		//truncate to first font name & remove quotes
		VString value;
		value = inValue;
		value.Truncate(posComma-1);
		bool hasStartQuote =(value.GetLength() >= 1 
							&& 
							(
							value.BeginsWith("\"")
							||
							value.BeginsWith("'")
							)
							);
		bool hasEndQuote =	(value.GetLength() >= 1 
							&& 
							(
							value.EndsWith("\"")
							||
							value.EndsWith("'")
							)
							);
		if (hasStartQuote)
			value.SubString( 2, value.GetLength()-1);
		if (hasEndQuote && value.GetLength() >= 1)
			value.Truncate(value.GetLength()-1);
		*fFontName = value;
		return;
	}
	else
	{
		bool hasStartQuote =(inValue.GetLength() >= 1 
							&& 
							(
							inValue.BeginsWith("\"")
							||
							inValue.BeginsWith("'")
							)
							);
		bool hasEndQuote =	(inValue.GetLength() >= 1 
							&& 
							(
							inValue.EndsWith("\"")
							||
							inValue.EndsWith("'")
							)
							);
		if (hasStartQuote || hasEndQuote)
		{
			//remove quotes
			VString value = inValue;
			if (hasStartQuote)
				value.SubString( 2, value.GetLength()-1);
			if (hasEndQuote && value.GetLength() >= 1)
				value.Truncate(value.GetLength()-1);
			*fFontName = value;
			return;
		}
	}
	
	*fFontName = inValue;
}

const VString& VTextStyle::GetFontName() const													
{
	return *fFontName;
}

void VTextStyle::SetJustification(eStyleJust inValue)	
{
	fJust = inValue;
}

eStyleJust VTextStyle::GetJustification() const				
{
	return fJust;
}

void VTextStyle::Translate(sLONG inValue)
{
	if(inValue) 
	{ 
		fBegin+= inValue;
		fEnd += inValue;
	}
}

/** merge current style with the specified style 
@remarks
	input style properties override current style properties without taking account input style range
	(current style range is preserved)
*/
void VTextStyle::MergeWithStyle( const VTextStyle *inStyle, const VTextStyle *inStyleParent, bool inForMetricsOnly, bool inAllowMergeSpanRefs)
{
	xbox_assert(inStyle);

	if (inStyleParent)
	{
		if (inStyle->GetBold() != UNDEFINED_STYLE)
			SetBold( inStyleParent->GetBold() == inStyle->GetBold() ? UNDEFINED_STYLE : inStyle->GetBold());
		if (inStyle->GetItalic() != UNDEFINED_STYLE)
			SetItalic( inStyleParent->GetItalic() == inStyle->GetItalic() ? UNDEFINED_STYLE : inStyle->GetItalic());
		if (inStyle->GetUnderline() != UNDEFINED_STYLE)
			SetUnderline( inStyleParent->GetUnderline() == inStyle->GetUnderline() ? UNDEFINED_STYLE : inStyle->GetUnderline());
		if (inStyle->GetStrikeout() != UNDEFINED_STYLE)
			SetStrikeout( inStyleParent->GetStrikeout() == inStyle->GetStrikeout() ? UNDEFINED_STYLE : inStyle->GetStrikeout());
		if (inStyle->GetFontSize() != UNDEFINED_STYLE)
			SetFontSize( inStyleParent->GetFontSize() == inStyle->GetFontSize() ? UNDEFINED_STYLE : inStyle->GetFontSize());
		if (!inStyle->GetFontName().IsEmpty())
			SetFontName( inStyleParent->GetFontName().EqualToString(inStyle->GetFontName(), true) ? VString("") : inStyle->GetFontName());

		if (!inForMetricsOnly)
		{
			if (inStyle->GetHasForeColor())
			{
				if	((!inStyleParent->GetHasForeColor()) || inStyleParent->GetColor() != inStyle->GetColor())
				{
					SetHasForeColor(true);
					SetColor(inStyle->GetColor());
				}
				else
				{
					SetHasForeColor(false);
				}
			}
			if (inStyle->GetHasBackColor())
			{
				if (!inStyleParent->GetHasBackColor() || inStyleParent->GetBackGroundColor() != inStyle->GetBackGroundColor())
				{
					SetHasBackColor( true);
					SetBackGroundColor( inStyle->GetBackGroundColor());
				}
				else
				{
					SetHasBackColor(false);
				}
			}
		}
		if (inStyle->GetJustification() != JST_Notset)
			SetJustification( inStyleParent->GetJustification() == inStyle->GetJustification() ? JST_Notset : inStyle->GetJustification());
	}
	else
	{
		if (inStyle->GetBold() != UNDEFINED_STYLE)
			SetBold( inStyle->GetBold());
		if (inStyle->GetItalic() != UNDEFINED_STYLE)
			SetItalic( inStyle->GetItalic());
		if (inStyle->GetUnderline() != UNDEFINED_STYLE)
			SetUnderline( inStyle->GetUnderline());
		if (inStyle->GetStrikeout() != UNDEFINED_STYLE)
			SetStrikeout( inStyle->GetStrikeout());
		if (inStyle->GetFontSize() != UNDEFINED_STYLE)
			SetFontSize( inStyle->GetFontSize());
		if (!inStyle->GetFontName().IsEmpty())
			SetFontName( inStyle->GetFontName());

		if (!inForMetricsOnly)
		{
			if (inStyle->GetHasForeColor())
			{
				SetHasForeColor(true);
				SetColor(inStyle->GetColor());
			}
			if (inStyle->GetHasBackColor())
			{
				SetHasBackColor( true);
				SetBackGroundColor( inStyle->GetBackGroundColor());
			}
		}
		if (inStyle->GetJustification() != JST_Notset)
			SetJustification( inStyle->GetJustification());
	}
	if (inAllowMergeSpanRefs) 
	{
		if (inStyle->IsSpanRef() && testAssert(!IsSpanRef()))
			SetSpanRef( inStyle->GetSpanRef()->Clone());
	}
#if ENABLE_VTEXTSTYLE_EXT
	if (inStyle->fStylesExt)
	{
		if (fStylesExt)
			fStylesExt->MergeWithStyle( inStyle->fStylesExt, inStyleParent ? inStyleParent->fStylesExt : NULL, inForMetricsOnly);
		else if (testAssert(ITextStyleExtFactory::Get()))
		{		
			ITextStyleExt *stylesExt = ITextStyleExtFactory::Get()->Create();
			stylesExt->MergeWithStyle( inStyle->fStylesExt, inStyleParent ? inStyleParent->fStylesExt : NULL, inForMetricsOnly);
			if (stylesExt->IsUndefined())
				delete stylesExt;
			else
				fStylesExt = stylesExt;
		}
	}
#endif
}

/** append styles from the passed style 
@remarks
	input style properties override current style properties without taking account input style range
	(current style range is preserved)

	if (inOnlyIfNotSet == true), overwrite only styles which are not defined locally
*/
void VTextStyle::AppendStyle( const VTextStyle *inStyle, bool inOnlyIfNotSet, bool inAllowMergeSpanRefs)
{
	if (inOnlyIfNotSet)
	{
		if (GetBold() == UNDEFINED_STYLE && inStyle->GetBold() != UNDEFINED_STYLE)
			SetBold( inStyle->GetBold());
		if (GetItalic() == UNDEFINED_STYLE && inStyle->GetItalic() != UNDEFINED_STYLE)
			SetItalic( inStyle->GetItalic());
		if (GetUnderline() == UNDEFINED_STYLE && inStyle->GetUnderline() != UNDEFINED_STYLE)
			SetUnderline( inStyle->GetUnderline());
		if (GetStrikeout() == UNDEFINED_STYLE && inStyle->GetStrikeout() != UNDEFINED_STYLE)
			SetStrikeout( inStyle->GetStrikeout());
		if (GetFontSize() == UNDEFINED_STYLE && inStyle->GetFontSize() != UNDEFINED_STYLE)
			SetFontSize( inStyle->GetFontSize());
		if (GetFontName().IsEmpty() && !inStyle->GetFontName().IsEmpty())
			SetFontName( inStyle->GetFontName());
     
		if (!GetHasForeColor() && inStyle->GetHasForeColor())
		{
			SetHasForeColor(true);
			SetColor(inStyle->GetColor());
		}
		if (!GetHasBackColor() && inStyle->GetHasBackColor())
		{
			SetHasBackColor( true);
			SetBackGroundColor( inStyle->GetBackGroundColor());
		}
		if (GetJustification() == JST_Notset && inStyle->GetJustification() != JST_Notset)
			SetJustification( inStyle->GetJustification());

		if (inAllowMergeSpanRefs) 
		{
			if (inStyle->IsSpanRef() && testAssert(!IsSpanRef()))
				SetSpanRef( inStyle->GetSpanRef()->Clone());
		}
#if ENABLE_VTEXTSTYLE_EXT
		if (inStyle->fStylesExt)
		{
			if (fStylesExt)
				fStylesExt->AppendStyle( inStyle->fStylesExt, inOnlyIfNotSet);
			else 
				fStylesExt = inStyle->fStylesExt->Clone();
		}
#endif
	}
	else
		MergeWithStyle( inStyle, NULL, false, inAllowMergeSpanRefs);
}


/** return true if none property is defined (such style is useless & should be discarded) */
bool VTextStyle::IsUndefined(bool inIgnoreSpanRef) const
{
	return	GetBold() == UNDEFINED_STYLE
			&&
			GetItalic() == UNDEFINED_STYLE
			&&
			GetUnderline() == UNDEFINED_STYLE
			&&
			GetStrikeout() == UNDEFINED_STYLE
			&&
			GetFontSize() == UNDEFINED_STYLE
			&&
			GetFontName().IsEmpty()
			&&
			(!GetHasForeColor())
			&&
			(!GetHasBackColor())
			&&
			GetJustification() == JST_Notset
			&&
			(!inIgnoreSpanRef ? fSpanTextRef == NULL : true)
#if ENABLE_VTEXTSTYLE_EXT
			&&
			(
				fStylesExt == NULL
				||
				fStylesExt->IsUndefined()
			)
#endif
			;
}

void VTextStyle::Reset(bool inKeepRange)
{
	if (!inKeepRange)
		fBegin = fEnd = 0;
	fIsBold = fIsUnderline = fIsStrikeout = fIsItalic = UNDEFINED_STYLE;
	fColor = 0xFF000000;
	fBkColor = 0xFFFFFFFF;
	fHasBackColor = false;
	fHasForeColor = false;
	fFontSize = -1;
	if (fFontName)
		fFontName->SetEmpty();
	else 
		fFontName = new VString();
	fJust = JST_Notset;
	if (fSpanTextRef)
	{
		delete fSpanTextRef;
		fSpanTextRef = NULL;
	}
#if ENABLE_VTEXTSTYLE_EXT
	if (fStylesExt)
	{
		delete fStylesExt;
		fStylesExt = NULL;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

VTreeTextStyle::VTreeTextStyle(VTextStyle* inData):IRefCountable()
{
	fParent = NULL;
	fdata = inData;
	fNumChildSpanRef = 0;
}


VTreeTextStyle::VTreeTextStyle(const VTreeTextStyle& inStyle):IRefCountable()
{
	fParent = NULL;
	fdata = NULL;
	fNumChildSpanRef = 0;
	const VTextStyle* data = inStyle.GetData();
	if(data)
	{
		fdata = new VTextStyle(data);

		for( VectorOfStyle::const_iterator i = inStyle.fChildren.begin() ; i != inStyle.fChildren.end() ; ++i)
		{
			VTreeTextStyle* newstyle = new VTreeTextStyle( *(*i));
			AddChild(newstyle);
			newstyle->Release();
		}
	}
}

VTreeTextStyle::VTreeTextStyle(const VTreeTextStyle* inStyle, bool inDeepCopy):IRefCountable()
{
	fParent = NULL;
	fdata = NULL;
	fNumChildSpanRef = 0;
	if(inStyle)
	{
		const VTextStyle* data = inStyle->GetData();
		if(data)
		{
			fdata = new VTextStyle(data);

			if(inDeepCopy)
			{
				for( VectorOfStyle::const_iterator i = inStyle->fChildren.begin() ; i != inStyle->fChildren.end() ; ++i)
				{
					VTreeTextStyle* newstyle = new VTreeTextStyle( *i);
					AddChild(newstyle);
					newstyle->Release();
				}
			}
		}
	}
}

VTreeTextStyle::~VTreeTextStyle()
{
	for( VectorOfStyle::iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
	{
		(*i)->SetParent(NULL);
		(*i)->Release();
	}
	delete fdata;
}

void VTreeTextStyle::RemoveChildren()
{
	for( VectorOfStyle::iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
	{
		(*i)->SetParent(NULL);
		(*i)->Release();
	}
	fChildren.clear();
	fNumChildSpanRef = 0;
}

void VTreeTextStyle::SetParent(VTreeTextStyle* inValue)
{
	fParent = inValue;
}

/** return true if font family is defined */
bool VTreeTextStyle::HasFontFamilyOverride() const
{
	bool fontoverride = false;

	VTextStyle *style = GetData();
	if (style)
	{
		sLONG start, end;
		style->GetRange( start, end);

		if (start < end)
			if (!style->GetFontName().IsEmpty())
				fontoverride = true;
	}
	if (!fontoverride)
	{
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			fontoverride = vstyle->HasFontFamilyOverride();
			if (fontoverride)
				break;
		}
	}
	return fontoverride;
}

void VTreeTextStyle::AddChild(VTreeTextStyle* inValue, bool inAutoGrowRange)
{
	if(inValue)
	{
		fChildren.push_back(inValue);
		inValue->Retain();
		inValue->SetParent(this);

		if (inValue->GetData()->IsSpanRef())
			fNumChildSpanRef++;

		if (inAutoGrowRange && inValue->GetData() && GetData())
		{
			sLONG start, end;
			GetData()->GetRange( start, end);

			sLONG startChild, endChild;
			inValue->GetData()->GetRange( startChild, endChild);
			if (startChild < start)
				start = startChild;
			if (endChild > end)
				end = endChild;
			GetData()->SetRange(start, end);
		}

	}
}

void VTreeTextStyle::RemoveChildAt(sLONG inNth)
{
	if (fChildren.size() == 0) 
		return;

	if(testAssert(inNth >= 1 && inNth <= fChildren.size()))
	{
		VTreeTextStyle* child = fChildren[inNth - 1];

		VectorOfStyle::iterator del_Iter;
		del_Iter = fChildren.begin();
		fChildren.erase(del_Iter+(inNth -1));
		
		if (child->GetData()->IsSpanRef())
			fNumChildSpanRef--;

		child->SetParent(NULL);
		child->Release();
	}
}

void VTreeTextStyle::Translate(sLONG inValue)
{
	if(inValue)
	{
		GetData()->Translate(inValue);
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			vstyle->Translate(inValue);
		}
	}
}

/** return true if theses styles are equal to the passed styles 
@remarks
	comparison ignores parent
*/
bool VTreeTextStyle::IsEqualTo( const VTreeTextStyle& inStyles) const
{
	if (fNumChildSpanRef != inStyles.fNumChildSpanRef)
		return false;
	if (fChildren.size() != inStyles.fChildren.size())
		return false;

	xbox_assert(GetData() && inStyles.GetData());
	if (!((*GetData()) == *(inStyles.GetData())))
		return false;

	VectorOfStyle::const_iterator it = fChildren.begin();
	VectorOfStyle::const_iterator it2 = inStyles.fChildren.begin();
	for(; it != fChildren.end(); it++, it2++)
		if (!(*it)->IsEqualTo( *(*it2)))
			return false;
	return true;
}

/** expand styles ranges from the specified range position by the specified offset 
@remarks
	should be called just after text has been inserted in a multistyled text
	(because inserted text should inherit style at current inserted text position-1)
	for instance:
		text.Insert(textToInsert, 10); //caution: here 1-based index
		styles->ExpandRangesAtPosBy( 9, textToInsert.GetLength()); //caution: here 0-based index so 10-1 and not 10
*/
void VTreeTextStyle::ExpandAtPosBy(	sLONG inStart, sLONG inInflate, const ISpanTextRef *inInflateSpanRef, VTextStyle **outStyleToPostApply, 
									bool inOnlyTranslate,
									bool inIsStartOfParagraph)
{
	if (inInflate <= 0)
		return;

	VTextStyle *style = GetData();
	sLONG start, end;
	style->GetRange( start, end);

	if (end < inStart)
		return;
	
	if (inInflateSpanRef)
	{
		//we fall here if we replace a span ref plain text:
		//we inflate only the passed span ref & the parent

		if (GetData()->IsSpanRef() && inInflateSpanRef == GetData()->GetSpanRef())
		{
			xbox_assert(end == start && start == inStart);
			style->SetRange( start, end+inInflate);
			return;
		}
		else if (HasSpanRefs()) //we inflate span ref parent
			style->SetRange( start, end+inInflate);
		else if (start >= inStart) //other styles or span refs are translated if after inflated span ref, otherwise do nothing
		{
			Translate( inInflate);
			return;
		}
	}
	else
	{
		if (inOnlyTranslate)  //if previous child has been translated or inflated, we cannot inflate otherwise we might overlap previous child range so translate
		{
			if (start >= inStart)
				Translate(inInflate);
			return;
		}

		//we fall here after some text has been inserted/replaced:
		//ensure span references are not expanded but only translated

		if (GetData()->IsSpanRef())
		{
			//ensure we do not inflate span refs 

			if (start >= inStart)
			{
				if (inIsStartOfParagraph && start == inStart && outStyleToPostApply && !GetData()->IsUndefined( true))
				{
					//we need to expand span ref character style without expanding span ref itself:
					//so pass new style to apply to caller (caller will propagate it to span ref next sibling(s))
					VTextStyle *newstyle = new VTextStyle();
					newstyle->MergeWithStyle( GetData()); //get only char styles but span ref
					*outStyleToPostApply = newstyle;
					newstyle->SetRange(inStart, inStart+inInflate);
				}
				Translate( inInflate);
			}
			else if (end == inStart && outStyleToPostApply && !inIsStartOfParagraph)
			{
				//we need to expand span ref character style without expanding span ref itself:
				//so pass new style to apply to caller (caller will propagate it to span ref next sibling(s))
				if (!GetData()->IsUndefined( true))
				{
					VTextStyle *newstyle = new VTextStyle();
					newstyle->MergeWithStyle( GetData()); //get only char styles but span ref
					*outStyleToPostApply = newstyle;
					newstyle->SetRange(inStart, inStart+inInflate);
				}
			}
			return;
		}

		if (start >= inStart) 
		{
			if (inIsStartOfParagraph)
			{
				if (start > inStart)
				{
					Translate(inInflate);
					return;
				}
				else
					//start == inStart == 0 or: insert at start of not empty paragraph & cur style range start == start of paragraph -> expand style
					style->SetRange( start, end+inInflate);
			}
			else
				if ((start > inStart || end > start) && start > 0)
				{
					Translate(inInflate);
					return;
				}
				else
					style->SetRange( start, end+inInflate);
		}
		else
			if (!inIsStartOfParagraph || end > inStart)
				style->SetRange( start, end+inInflate);
			else
				//insert at start of not empty paragraph & cur style range end == start of paragraph -> skip as we cannot expand style of previous paragraph over the paragraph unless it is empty
				return;
	}

	bool onlyTranslate = false;
	sLONG beforeChildStart, beforeChildEnd;
	sLONG afterChildStart, afterChildEnd;
	if (!outStyleToPostApply)
	{
		std::vector<VTextStyle *> postApplyStyles;
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			VTextStyle *postApplyStyle = NULL;
			if (!onlyTranslate)
				vstyle->GetData()->GetRange( beforeChildStart, beforeChildEnd);
			vstyle->ExpandAtPosBy( inStart, inInflate, inInflateSpanRef, &postApplyStyle, onlyTranslate, inIsStartOfParagraph);
			if (!onlyTranslate)
			{
				vstyle->GetData()->GetRange( afterChildStart, afterChildEnd);
				if (afterChildEnd > beforeChildEnd)
					onlyTranslate = true;
			}	
			if (postApplyStyle)
			{
				onlyTranslate = true;
				postApplyStyles.push_back( postApplyStyle);
			}
		}
		std::vector<VTextStyle *>::iterator itPostApplyStyles = postApplyStyles.begin();
		for(;itPostApplyStyles != postApplyStyles.end(); itPostApplyStyles++)
		{
			ApplyStyle( *itPostApplyStyles);
			delete *itPostApplyStyles;
		}
	}
	else
	{
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			if (!onlyTranslate)
				vstyle->GetData()->GetRange( beforeChildStart, beforeChildEnd);
			vstyle->ExpandAtPosBy( inStart, inInflate, inInflateSpanRef, NULL, onlyTranslate, inIsStartOfParagraph);
			if (!onlyTranslate)
			{
				vstyle->GetData()->GetRange( afterChildStart, afterChildEnd);
				if (afterChildEnd > beforeChildEnd)
					onlyTranslate = true;
			}	
		}
	}
}


/** truncate the specified style range
@param inStart
	start of truncated range
@param inLength
	length of truncated range (-1 to truncate from inStart to the end of this VTreeTextStyle range)
@remarks
	should be called just after text has been deleted in a multistyled text
	for instance:
		text.Remove(10, 5); //caution: here 1-based index
		styles->Truncate(9, 5); //caution: here 0-based index so 10-1 and not 10
*/
void VTreeTextStyle::Truncate(sLONG inStart, sLONG inLength, bool inRemoveStyleIfEmpty, bool inPreserveSpanRefIfEmpty)
{
	if (inLength == 0)
		return;

	VTextStyle *style = GetData();
	sLONG start, end;
	style->GetRange( start, end);

	sLONG inEnd;
	if (inLength < 0)
		inEnd =  end;
	else
		inEnd = inStart+inLength;

	if (inStart > end)
		return;
	if (inEnd <= start)
	{
		Translate(-(inEnd-inStart));
		return;
	}
	
	sLONG inStartBase = inStart;
	sLONG inEndBase = inEnd;

	if (inEnd > end)
		inEnd = end;
	//if (inEnd < start)
	//	inEnd = start;
	if (inStart < start)
		inStart = start;
	//if (inStart > end)
	//	inStart = end;
	if (inStart >= inEnd)
		return;

	if (inStartBase > start)
		style->SetRange( start, end-(inEnd-inStart));
	else
		style->SetRange( inStartBase, inStartBase+end-start-(inEnd-inStart));

	style->GetRange( start, end);

	//remark: if inPreserveSpanRefIfEmpty == true, we are replacing some span reference plain text (see UpdateSpanRefs)
	//		  so we should not remove any node from fChildren
	sLONG index = 0;
	for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ;)
	{
		VTreeTextStyle* vstyle = *i;
		sLONG childStart, childEnd;
		vstyle->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inStartBase && childEnd <= inEndBase)
		{
			if	((inRemoveStyleIfEmpty || (childStart > inStartBase || childEnd < inEndBase))
				 &&
				 (!inPreserveSpanRefIfEmpty))
			{
				//skip & remove it (we preserve only uniform styles on truncated range if inRemoveStyleIfEmpty == false)
				RemoveChildAt( index+1);
				i = fChildren.begin();
				std::advance( i, index);
				if (i == fChildren.end())
					break;
				else
					continue;
			}
			else if (!inPreserveSpanRefIfEmpty && vstyle->GetData()->IsSpanRef())
			{
				vstyle->GetData()->SetSpanRef( NULL); //clear span reference but other styles
				fNumChildSpanRef--;
			}
		}

		vstyle->Truncate( inStartBase, inEndBase-inStartBase, inRemoveStyleIfEmpty, inPreserveSpanRefIfEmpty);

		vstyle->GetData()->GetRange( childStart, childEnd);
		if (!inPreserveSpanRefIfEmpty && !vstyle->GetData()->IsSpanRef() && childStart <= start && childEnd >= end && vstyle->GetChildCount() <= 0)
		{
			//truncated child style cover same range as actual style: merge with child style...
			style->MergeWithStyle( vstyle->GetData(), fParent ? fParent->GetData() : NULL);
			
			//...and remove child style
			RemoveChildAt( index+1);
			i = fChildren.begin();
			std::advance( i, index);
			if (i == fChildren.end())
				break;
		}
		else
		{
			i++;
			index++;
		}
	}
}

VTreeTextStyle* VTreeTextStyle::GetParent()const 
{
	return fParent;
}

void VTreeTextStyle::InsertChild(VTreeTextStyle* inValue, sLONG pos, bool inAutoGrowRange)
{
	if(inValue)
	{
		inValue->SetParent(this);
		fChildren.insert( fChildren.begin() + (pos -1),inValue);
		inValue->Retain();

		if (inValue->GetData()->IsSpanRef())
			fNumChildSpanRef++;

		if (inAutoGrowRange && inValue->GetData() && GetData())
		{
			sLONG start, end;
			GetData()->GetRange( start, end);

			sLONG startChild, endChild;
			inValue->GetData()->GetRange( startChild, endChild);
			if (startChild < start)
				start = startChild;
			if (endChild > end)
				end = endChild;
			GetData()->SetRange(start, end);
		}
	}
}

void VTreeTextStyle::SetData(VTextStyle* inValue)	
{
	fdata = inValue;
}

VTextStyle* VTreeTextStyle::GetData () const		
{
	return fdata;
}

sLONG VTreeTextStyle::GetChildCount() const					
{
	return (sLONG) fChildren.size();
}

VTreeTextStyle* VTreeTextStyle::GetNthChild(sLONG inNth) const
{
	if(inNth > GetChildCount() || inNth <= 0)
		return NULL;
	else
		return fChildren[inNth-1];
}


/** build a vector of sequential styles from the current cascading styles 
@remarks
	output styles ranges are disjoint & contain merged cascading styles & n+1 style range follows immediately n style range
*/
void VTreeTextStyle::BuildSequentialStylesFromCascadingStyles( std::vector<VTextStyle *>& outStyles, const VTextStyle *inStyleParent, bool inForMetrics, bool inFirstUniformStyleOnly) const
{
	const VTextStyle *styleCur = GetData();
	if (styleCur)
	{
		VTextStyle *style = NULL;
		if (inStyleParent)
		{
			//merge both styles
			style = new VTextStyle( inStyleParent);
			style->MergeWithStyle( styleCur, NULL, inForMetrics, true);
		}
		else
			style = new VTextStyle( styleCur);

		sLONG start,end;
		styleCur->GetRange( start, end);
		sLONG endCur = end;
		if (GetChildCount() > 0)
		{
			sLONG childStart, childEnd;
			GetNthChild(1)->GetData()->GetRange(childStart, childEnd);
			end = childStart;
			style->SetRange( start, end);
			bool doDeleteStyle = false;
			if (end > start)
			{
				if (outStyles.size() > 0 && !outStyles.back()->IsSpanRef() && !style->IsSpanRef() && outStyles.back()->IsEqualTo(*style, false))
				{
					sLONG startExpand, endExpand;
					outStyles.back()->GetRange(startExpand, endExpand);
					outStyles.back()->SetRange(startExpand, end);
					doDeleteStyle = true;
				}
				else 
					outStyles.push_back( style);
				if (inFirstUniformStyleOnly)
				{
					if (doDeleteStyle)
						delete style;
					return;
				}
			}
			else
				doDeleteStyle = true;

			for (int i = 1; i <= GetChildCount(); i++)
			{
				GetNthChild(i)->BuildSequentialStylesFromCascadingStyles( outStyles, style, inForMetrics, inFirstUniformStyleOnly);
				if (outStyles.size() > 0)
					outStyles.back()->GetRange(childStart, childEnd);
				start = childEnd;
				if (start >= endCur)
				{
					if (doDeleteStyle)
						delete style;
					return;
				}
				else
				{
					if (i+1 <= GetChildCount())
					{
						GetNthChild(i+1)->GetData()->GetRange(childStart, childEnd);
						if (start < childStart)
						{
							if (!doDeleteStyle)
								style = new VTextStyle( style);
							doDeleteStyle = false;
							style->SetRange( start, childStart);
							if (outStyles.size() > 0 && !outStyles.back()->IsSpanRef() && !style->IsSpanRef() && outStyles.back()->IsEqualTo(*style, false))
							{
								sLONG startExpand, endExpand;
								outStyles.back()->GetRange(startExpand, endExpand);
								outStyles.back()->SetRange(startExpand, childStart);
								doDeleteStyle = true;
							}
							else 
								outStyles.push_back( style);
							if (inFirstUniformStyleOnly)
							{
								if (doDeleteStyle)
									delete style;
								return;
							}
						}
					}
					else
					{
						if (!doDeleteStyle)
							style = new VTextStyle( style);
						doDeleteStyle = true;
					}
				}
			}

			if (start < endCur)
			{
				style->SetRange( start, endCur);
				if (outStyles.size() > 0 && !outStyles.back()->IsSpanRef() && !style->IsSpanRef() && outStyles.back()->IsEqualTo(*style, false))
				{
					sLONG startExpand, endExpand;
					outStyles.back()->GetRange(startExpand, endExpand);
					outStyles.back()->SetRange(startExpand, endCur);
					delete style;
				}
				else 
					outStyles.push_back( style);
				if (inFirstUniformStyleOnly)
					return;
			}
			else 
				delete style;
		}
		else if (!style->IsUndefined())
		{
			style->SetRange( start, endCur);
			if (outStyles.size() > 0 && !outStyles.back()->IsSpanRef() && !style->IsSpanRef() && outStyles.back()->IsEqualTo(*style, false))
			{
				sLONG startExpand, endExpand;
				outStyles.back()->GetRange(startExpand, endExpand);
				outStyles.back()->SetRange(startExpand, endCur);
				delete style;
			}
			else 
				outStyles.push_back( style);
		}
		else
			delete style;
	}
}


/** convert cascading styles to sequential styles 
@remarks
	cascading styles are merged in order to build a tree which has only one level deeper;
	on output, child VTreeTextStyle ranges will be disjoint & n+1 style range follows immediately n style range
*/
void VTreeTextStyle::BuildSequentialStylesFromCascadingStyles( const VTextStyle *inStyleParent, bool inFirstUniformStyleOnly)
{
	std::vector<VTextStyle *> vstyle;
	BuildSequentialStylesFromCascadingStyles( vstyle, inStyleParent, false, inFirstUniformStyleOnly);

	RemoveChildren();

	sLONG start, end;
	GetData()->GetRange( start, end);
	GetData()->Reset();
	GetData()->SetRange(start, end);
	for (int i = 0; i < vstyle.size(); i++)
	{
		VTreeTextStyle *styles = new VTreeTextStyle( vstyle[i]);
		AddChild( styles);
		styles->Release();
	}
}

/** build cascading styles from the input sequential styles 
@remarks
	input styles ranges should not overlap  

	this methods overrides current styles with the input styles
	this class owns input VTextStyle instances so caller should not delete it
*/
void VTreeTextStyle::BuildCascadingStylesFromSequentialStyles( std::vector<VTextStyle *>& inStyles, bool inKeepCurRange)
{
	if (!GetData())
		SetData( new VTextStyle());

	RemoveChildren();
	xbox_assert(GetChildCount() == 0);

	VTextStyle *curStyle = GetData();
	if (inStyles.size() == 1 && !inStyles[0]->IsSpanRef() && !inKeepCurRange)
	{
		*curStyle = *(inStyles[0]);
		delete inStyles[0];
	}
	else
	{
		curStyle->Reset(inKeepCurRange);
		if (!inStyles.size())
			return;

		curStyle->SetJustification( inStyles[0]->GetJustification()); //get global justification from first style
		
		std::vector<VTextStyle *>::iterator it;
		if (!inKeepCurRange)
		{
			sLONG start = XBOX::MaxLongInt;
			sLONG end = 0;

			//first determine full range
			it = inStyles.begin();
			for (;it != inStyles.end(); it++)
			{
				VTextStyle *styleChild = *it;
				xbox_assert(styleChild);

				styleChild->SetJustification( JST_Notset); //justification is global & set on parent
				if (styleChild->IsUndefined())
				{
					delete styleChild;
					*it = NULL;
				}
				else
				{
					sLONG childStart, childEnd;
					styleChild->GetRange( childStart, childEnd);
					if (childStart == 0 || childStart < childEnd)
					{
						start = std::min( start, childStart);
						end = std::max( end, childEnd);
					}
					else
					{
						delete styleChild;
						*it = NULL;
					}
				}
			}
			curStyle->SetRange( start, end);
		}

		//then apply styles
		it = inStyles.begin();
		for (;it != inStyles.end(); it++)
		{
			VTextStyle *styleChild = *it;
			if (!styleChild)
				continue;

			ApplyStyle( styleChild);

			delete styleChild;
		}
	}
}


/** this ensure decoration styles are not cascading (necessary for compatibility with most HTML browsers which do not manage well cascading decoration styles with span tags) 
@remarks
	call this method for instance before exporting to HTML
*/
void VTreeTextStyle::DisableCascadingForDecorations(const VTextStyle *inStyleInherit)
{
	VTextStyle *style = GetData();
	if (style->IsSpanRef())
	{
#if ENABLE_VTEXTSTYLE_EXT
		if (style->GetStylesExt())
		{
			//cleanup unused decoration sub-styles
			if (style->GetUnderline() != TRUE)
				style->GetStylesExt()->ClearUnderline();
			if (style->GetStrikeout() != TRUE)
				style->GetStylesExt()->ClearLineThrough();
		}	
#endif
	}
	else
	{
		VTextStyle *styleInherit = NULL;
#if ENABLE_VTEXTSTYLE_EXT
		if (style->GetStylesExt() && (style->GetUnderline() != TRUE && style->GetStrikeout() != TRUE))
		{
			//cleanup unused decoration sub-styles (but retain sub-styles in order to be taken account deeper if decoration is enabled deeper)
			if (GetChildCount() > 0)
			{
				if (inStyleInherit)
				{
					styleInherit = new VTextStyle(inStyleInherit);
					styleInherit->MergeWithStyle(style);
				}
				else
					styleInherit = new VTextStyle( style);
			}
			style->GetStylesExt()->ClearUnderline();
			style->GetStylesExt()->ClearLineThrough();
		}	
#endif
		if (style->GetUnderline() == TRUE || style->GetStrikeout() == TRUE)
		{
			BuildSequentialStylesFromCascadingStyles( styleInherit ? styleInherit : inStyleInherit);
			if (!HasSpanRefs() && GetChildCount() == 1)
			{
				sLONG start, end;
				GetData()->GetRange(start, end);
				VTreeTextStyle *child = GetNthChild(1);
				sLONG childStart, childEnd;
				child->GetData()->GetRange(childStart, childEnd);
				if (childStart <= start && childEnd >= end)
				{
					child->Retain();
					RemoveChildren();
					GetData()->MergeWithStyle( child->GetData());
					std::vector<VTreeTextStyle *> subchildren;
					for (int i = 1; i <= child->GetChildCount(); i++)
					{
						VTreeTextStyle *subchild = child->GetNthChild(i);
						subchild->Retain();
						subchildren.push_back(subchild);
					}
					child->RemoveChildren();
					child->Release();
					std::vector<VTreeTextStyle *>::const_iterator itChild = subchildren.begin();
					for (; itChild != subchildren.end(); itChild++)
					{
						AddChild( *itChild);
						(*itChild)->Release();
					}
				}
			}

#if ENABLE_VTEXTSTYLE_EXT
			//cleanup unused decoration sub-styles 
			style = GetData();
			if (style->GetStylesExt())
			{
				if (style->GetUnderline() != TRUE)
					style->GetStylesExt()->ClearUnderline();
				if (style->GetStrikeout() != TRUE)
					style->GetStylesExt()->ClearLineThrough();
			}
			for (int i = 1; i <= GetChildCount(); i++)
			{
				VTreeTextStyle *child = GetNthChild(i);
				style = child->GetData();
				if (style->GetStylesExt())
				{
					if (style->GetUnderline() != TRUE)
						style->GetStylesExt()->ClearUnderline();
					if (style->GetStrikeout() != TRUE)
						style->GetStylesExt()->ClearLineThrough();
				}
			}
#endif
		}
		else if (GetChildCount() > 0)
		{
			if (!styleInherit)
			{
				if (inStyleInherit)
				{
					styleInherit = new VTextStyle(inStyleInherit);
					styleInherit->MergeWithStyle(style);
				}
				else
					styleInherit = new VTextStyle( style);
			}

			for (int i = 1; i <= GetChildCount(); i++)
			{
				VTreeTextStyle *child = GetNthChild(i);
				child->DisableCascadingForDecorations( styleInherit);
			}
		}
		if (styleInherit)
			delete styleInherit;
	}
	if (GetChildCount() > 1 && !fParent)
	{
		//now re-apply children styles (in order to cascade again after decoration cleanup)
		std::vector<VTreeTextStyle *> children;
		for (int i = 1; i <= GetChildCount(); i++)
		{
			VTreeTextStyle *child = GetNthChild(i);
			child->Retain();
			children.push_back(child);
		}
		RemoveChildren();
		std::vector<VTreeTextStyle *>::const_iterator itChild = children.begin();
		for (; itChild != children.end(); itChild++)
		{
			ApplyStyles( *itChild);
			(*itChild)->Release();
		}
	}
}


/** apply styles 
@param inStyle
	styles to apply
@remarks
	caller is owner of inStyle
*/
void VTreeTextStyle::ApplyStyles( const VTreeTextStyle *inStyles, bool inIgnoreIfEmpty, sLONG inStartBase)
{
	ApplyStyle( inStyles->GetData(), inIgnoreIfEmpty, inStartBase);

	VectorOfStyle::const_iterator it = inStyles->fChildren.begin();
	for (;it != inStyles->fChildren.end(); it++)
		ApplyStyles( *it, inIgnoreIfEmpty, inStartBase);
}



/** apply style to the specified range 
@param inStyle
	style to apply 
*/
void VTreeTextStyle::ApplyStyle( const VTextStyle *inStyle, bool inIgnoreIfEmpty, sLONG inStartBase)
{
	_ApplyStyleRec( inStyle, 0, NULL, inIgnoreIfEmpty, inStartBase);
}


/** return true if plain text range can be deleted or replaced 
@remarks
	return false if range overlaps span text references plain text & if range does not fully includes span text reference(s) plain text
	(we cannot delete or replace some sub-part of span text reference plain text but only full span text reference plain text at once)
*/
bool VTreeTextStyle::CanDeleteOrReplace( sLONG inStart, sLONG inEnd)
{
	if (fChildren.size() == 0)
		return true;
	
	VectorOfStyle::const_iterator it = fChildren.begin();
	for(;it != fChildren.end(); it++)
	{
		const VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG curStart, curEnd;
			style->GetRange( curStart, curEnd);

			if (inStart > curStart && inStart < curEnd)
				return false;
			if (inEnd > curStart && inEnd < curEnd)
				return false;
		}
	}
	return true;
}

bool VTreeTextStyle::CanApplyStyle( const VTextStyle *inStyle, sLONG inStartBase) const
{
	if (GetData()->IsSpanRef() && inStyle->IsSpanRef()) //span references are not recursive
		return false;
	if (fChildren.size() == 0)
		return true;
	
	sLONG inStart, inEnd;
	inStyle->GetRange( inStart, inEnd);
	inStart += inStartBase;
	inEnd += inStartBase;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for(;it != fChildren.end(); it++)
	{
		const VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG curStart, curEnd;
			style->GetRange( curStart, curEnd);

			if (inStart > curStart && inStart < curEnd)
				return false;
			if (inEnd > curStart && inEnd < curEnd)
				return false;
		}
	}
	return true;
}

/** reset styles if equal to inStyle styles */
void VTextStyle::ResetCommonStyles( const VTextStyle *inStyle, bool inIgnoreSpanRef)
{
	xbox_assert(inStyle);

	if (GetBold() != UNDEFINED_STYLE && inStyle->GetBold() == GetBold())
		SetBold( UNDEFINED_STYLE);
	if (GetItalic() != UNDEFINED_STYLE && inStyle->GetItalic() == GetItalic())
		SetItalic( UNDEFINED_STYLE);
	if (GetUnderline() != UNDEFINED_STYLE && inStyle->GetUnderline() == GetUnderline())
		SetUnderline( UNDEFINED_STYLE);
	if (GetStrikeout() != UNDEFINED_STYLE && inStyle->GetStrikeout() == GetStrikeout())
		SetStrikeout( UNDEFINED_STYLE);
	if (GetFontSize() != UNDEFINED_STYLE && inStyle->GetFontSize() == GetFontSize())
		SetFontSize( UNDEFINED_STYLE);
	if (!GetFontName().IsEmpty() && inStyle->GetFontName().EqualToString(GetFontName(), true))
		SetFontName( CVSTR(""));

	if (GetHasForeColor() && inStyle->GetHasForeColor() && inStyle->GetColor() == GetColor())
		SetHasForeColor(false);
	if (GetHasBackColor() && inStyle->GetHasBackColor() && inStyle->GetBackGroundColor() == GetBackGroundColor())
		SetHasBackColor(false);

	if (GetJustification() != JST_Notset && inStyle->GetJustification() == GetJustification())
		SetJustification( JST_Notset);
	
#if ENABLE_VTEXTSTYLE_EXT
	if (fStylesExt && inStyle->fStylesExt)
		fStylesExt->ResetCommonStyles( inStyle->fStylesExt);
#endif

	if (inIgnoreSpanRef)
		return;
	if (IsSpanRef() && inStyle->IsSpanRef() && GetSpanRef()->EqualTo( inStyle->GetSpanRef()))
		SetSpanRef( NULL);
}


/** apply style to the specified range 
@param inStyle
	style to apply 
*/
void VTreeTextStyle::_ApplyStyleRec( const VTextStyle *inStyle, sLONG inRecLevel, VTextStyle *inStyleInherit, bool inIgnoreIfEmpty, sLONG inStartBase)
{
	sLONG inStart, inEnd;
	inStyle->GetRange( inStart, inEnd);
	inStart += inStartBase;
	inEnd += inStartBase;
	bool inRangeEmpty = false;

	if (inStart >= inEnd)
	{
		inRangeEmpty = true;
		if (inIgnoreIfEmpty && inStart > 0)
			return;
		if (inStyle->IsSpanRef()) //span ref should be at least one character length
			return;
	}
	if (inStyle->IsUndefined())
		return;

	sLONG curStart, curEnd;
	GetData()->GetRange( curStart, curEnd);

	if (inStart <= curStart && inEnd >= curEnd && !inStyle->IsSpanRef())
	{
		//merge directly with current style if input style range fully includes current style range
		_MergeWithStyle( inStyle, inStyleInherit);
		return;
	}

	if (inStart > curEnd)
		return;
	if (inEnd < curStart)
		return;

	if (inStart < curStart)
		inStart = curStart;
	if (inStart > curEnd)
		inStart = curEnd;

	if (inEnd < curStart)
		inEnd = curStart;
	if (inEnd > curEnd)
		inEnd = curEnd;

	if (inStart > inEnd)
		return;

	if (inStart == inEnd)
	{
		inRangeEmpty = true;
		if (inIgnoreIfEmpty && inStart > 0)
			return;
		if (inStyle->IsSpanRef()) //span ref should be at least one character length
			return;
	}

	if (!CanApplyStyle( inStyle, inStartBase))
		return;
	if (inStyle->IsSpanRef())
	{
		//span reference style can only be attached to this VTreeTextStyle

		VTextStyle *styleSpanRef = new VTextStyle( inStyle);
		styleSpanRef->ResetCommonStyles( GetData(), true);
		styleSpanRef->SetRange(inStart, inEnd);  
		VTreeTextStyle *stylesSpanRef = new VTreeTextStyle( styleSpanRef);

		if (GetChildCount() <= 0)
		{
			AddChild( stylesSpanRef);
			ReleaseRefCountable(&stylesSpanRef);
		}
		else
		{
			VTreeTextStyle *stylesBefore = curStart < inStart ? CreateTreeTextStyleOnRange(curStart, inStart, false) : NULL;
			
			xbox_assert(inStart < inEnd);
			VTreeTextStyle *styles = CreateTreeTextStyleOnRange( inStart, inStart+1, false);
			if (styles)
			{
				styles->GetData()->ResetCommonStyles( GetData(), true);
				styleSpanRef->AppendStyle( styles->GetData(), true);
				styles->Release();
			}

			VTreeTextStyle *stylesAfter = inEnd < curEnd ? CreateTreeTextStyleOnRange(inEnd, curEnd, false) : NULL;

			RemoveChildren();
			std::vector<VTreeTextStyle *> vstyles;
			if (stylesBefore)
			{
				stylesBefore->GetData()->ResetCommonStyles( GetData(), true);
				if (stylesBefore->HasSpanRefs() || stylesBefore->GetData()->IsUndefined())
				{
					sLONG count = stylesBefore->GetChildCount();
					for (int i = 1; i <= count; i++)
					{
						VTreeTextStyle *childStyle = stylesBefore->GetNthChild(i);
						childStyle->Retain();
						vstyles.push_back(childStyle);
					}
				}
				else
				{
					stylesBefore->Retain();
					vstyles.push_back(stylesBefore);
				}
			}
			ReleaseRefCountable(&stylesBefore);

			vstyles.push_back( stylesSpanRef);

			if (stylesAfter)
			{
				stylesAfter->GetData()->ResetCommonStyles( GetData(), true);
				if (stylesAfter->HasSpanRefs() || stylesAfter->GetData()->IsUndefined())
				{
					sLONG count = stylesAfter->GetChildCount();
					for (int i = 1; i <= count; i++)
					{
						VTreeTextStyle *childStyle = stylesAfter->GetNthChild(i);
						childStyle->Retain();
						vstyles.push_back(childStyle);
					}
				}
				else
				{
					stylesAfter->Retain();
					vstyles.push_back(stylesAfter);
				}
			}
			ReleaseRefCountable(&stylesAfter);

			std::vector<VTreeTextStyle *>::const_iterator itStyles = vstyles.begin();
			for (;itStyles != vstyles.end(); itStyles++)
			{
				AddChild( *itStyles);
				(*itStyles)->Release();
			}
		}
		return;
	}

	//compute inherited styles to pass to children = inherited styles + current styles
	VTextStyle *styleInheritForChilds = NULL;
	if (inStyleInherit)
	{
		styleInheritForChilds = new VTextStyle( inStyleInherit);
		styleInheritForChilds->MergeWithStyle( GetData(), NULL);
	}
	else if (GetParent())
	{
		styleInheritForChilds = new VTextStyle(GetParent()->GetData());
		styleInheritForChilds->MergeWithStyle( GetData(), NULL);
	}

	//ensure inStyle overrides only styles not equal to inherited styles
	VTextStyle *newStyle = new VTextStyle();
	xbox_assert(newStyle);
	xbox_assert(!inStyle->IsSpanRef());
	newStyle->MergeWithStyle( inStyle, styleInheritForChilds ? styleInheritForChilds : GetData());
	newStyle->SetRange( inStart, inEnd);
	bool applyOnCurrent = !newStyle->IsUndefined();

	//recursively apply to child style

	//apply style recursively 
	sLONG start = inStart;
	sLONG indexLastChild = 0;
	VTextStyle *styleLastChild = NULL;
	VTreeTextStyle *stylesLastChild = NULL;
	sLONG endLastChild = 0;
	sLONG startLastChild = 0;
	int i = 1;

	if (!inRangeEmpty)
		//if new style range is not empty -> we need to merge new style with current style(s)
		for (;i <= GetChildCount();)
		{
			VTreeTextStyle *childStyles = GetNthChild( i);
			
			sLONG childStart, childEnd;
			childStyles->GetData()->GetRange( childStart, childEnd);

			sLONG endTemp = std::min(childStart, inEnd);
			if (applyOnCurrent && start < endTemp)
			{
				if (styleLastChild && endLastChild == start && !styleLastChild->IsSpanRef() && styleLastChild->IsEqualTo( *newStyle, false))
				{
					//concat with previous child style
					styleLastChild->SetRange( startLastChild, endTemp);
				}
				else 
				{
					bool childInserted = false;
					if (inRecLevel <= 5 && styleLastChild && !styleLastChild->IsSpanRef() && endLastChild == start)
					{
						VTextStyle *newStyle2 = new VTextStyle( newStyle);
						VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( newStyle2);
						if (styleCommon)
						{
							if (styleLastChild->IsUndefined())
							{
								//set last child styles to common styles, expand range & add new styles minus common prop as children
								*styleLastChild = *styleCommon;
								delete styleCommon;
								styleLastChild->SetRange( startLastChild, endTemp);
								VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
								newStyle2->SetRange( start, endTemp);
								stylesLastChild->AddChild( newStyles2);
								newStyles2->Release();
								endLastChild = endTemp;
								childInserted = true;
							}
							else
							{
								//insert new child with common styles & for children the last child & new styles minus common properties
								xbox_assert(stylesLastChild == GetNthChild( i-1));
								stylesLastChild->Retain();
								RemoveChildAt(i-1);
								i = i-1;
								xbox_assert(childStyles == GetNthChild( i));

								styleCommon->SetRange( startLastChild, endTemp);
								VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
								newStyles->AddChild( stylesLastChild);
								stylesLastChild->Release();
								if (!newStyle2->IsUndefined())
								{
									VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
									newStyle2->SetRange( start, endTemp);
									newStyles->AddChild( newStyles2);
									newStyles2->Release();
								}	
								else
									delete newStyle2;
								
								stylesLastChild = newStyles;
								styleLastChild = stylesLastChild->GetData();
								InsertChild( newStyles, i);
								indexLastChild = i;
								endLastChild = endTemp;
								newStyles->Release();
								i++;
								xbox_assert(childStyles == GetNthChild( i));

								childInserted = true;
							}
						}
						else
							delete newStyle2;
					}
					if (!childInserted)
					{
						//insert new child style
						VTreeTextStyle *newStyles = stylesLastChild = new VTreeTextStyle( styleLastChild = new VTextStyle( newStyle));
						newStyles->GetData()->SetRange( start, endTemp);
						InsertChild( newStyles, i);
						indexLastChild = i;
						startLastChild = start;
						endLastChild = endTemp;
						newStyles->Release();
						i++;
						xbox_assert(childStyles == GetNthChild( i));
					}
				}
			}
			if (childStart >= inEnd)
			{
				start = inEnd;
				break;
			}

			childStyles->_ApplyStyleRec( inStyle, inRecLevel+1, styleInheritForChilds, inIgnoreIfEmpty, inStartBase);

			if (childStyles->GetChildCount() == 0 && childStyles->IsUndefined(true))
				RemoveChildAt(i);
			else if (childStyles->GetData()->IsSpanRef())
			{
				stylesLastChild = childStyles; 
				styleLastChild = childStyles->GetData();
				indexLastChild = i;
				startLastChild = childStart;
				endLastChild = childEnd;
				i++;
			}
			else
			{	
				if (styleLastChild && !styleLastChild->IsSpanRef() && endLastChild == childStart && childStyles->GetChildCount() == 0 && styleLastChild->IsEqualTo( *(childStyles->GetData()), false))
				{
					//concat with previous child style
					styleLastChild->SetRange( startLastChild, childEnd);
					RemoveChildAt(i);
				}
				else
				{
					bool childInserted = false;
					if (inRecLevel <= 5 && styleLastChild && !styleLastChild->IsSpanRef() && endLastChild == childStart)
					{
						VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( childStyles->GetData());
						if (styleCommon)
						{
							if (styleLastChild->IsUndefined())
							{
								//set last child styles to common styles, expand range & add current child styles minus common prop as children for last child styles
								*styleLastChild = *styleCommon;
								delete styleCommon;
								styleLastChild->SetRange( startLastChild, childEnd);
					
								childStyles->Retain();
								RemoveChildAt(i);
								stylesLastChild->AddChild( childStyles);
								childStyles->Release();
								endLastChild = childEnd;
								childInserted = true;
							}
							else
							{
								//insert new child with common styles & for children the last child & currend child styles minus common properties
								xbox_assert(stylesLastChild == GetNthChild( i-1));
								stylesLastChild->Retain();
								RemoveChildAt(i-1);
								i = i-1;
								xbox_assert(childStyles == GetNthChild( i));
								childStyles->Retain();
								RemoveChildAt(i);

								styleCommon->SetRange( startLastChild, childEnd);
								VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
								newStyles->AddChild( stylesLastChild);
								stylesLastChild->Release();
								newStyles->AddChild( childStyles);
								childStyles->Release();

								stylesLastChild = newStyles;
								styleLastChild = stylesLastChild->GetData();
								InsertChild( newStyles, i);
								indexLastChild = i;
								endLastChild = childEnd;
								newStyles->Release();
								i++;
								childInserted = true;
							}
						}
					}
					if (!childInserted)
					{
						stylesLastChild = childStyles; 
						styleLastChild = childStyles->GetData();
						indexLastChild = i;
						startLastChild = childStart;
						endLastChild = childEnd;
						i++;
					}
				}
			}
			start = std::max(start,childEnd);
			if (start >= inEnd)
				break;
		}
	else
		//new style range is empty -> we insert the new style 
		for (;i <= GetChildCount();)
		{
			VTreeTextStyle *childStyles = GetNthChild( i);
			
			sLONG childStart, childEnd;
			childStyles->GetData()->GetRange( childStart, childEnd);

			if (childStart > inEnd)
			{
				//insert new child style
				VTreeTextStyle *newStyles = new VTreeTextStyle( new VTextStyle( newStyle));
				newStyles->GetData()->SetRange( inStart, inEnd);
				InsertChild( newStyles, i);
				newStyles->Release();
				applyOnCurrent = false;
				break;
			}
			else if (childEnd >= inEnd)
			{
				childStyles->_ApplyStyleRec( inStyle, inRecLevel+1, styleInheritForChilds, inIgnoreIfEmpty, inStartBase);
				applyOnCurrent = false;
				break;
			}
			else
				i++;
		}

	if (applyOnCurrent)
	{
		if (start < inEnd)
		{
			if (styleLastChild && !styleLastChild->IsSpanRef() && endLastChild == start && styleLastChild->IsEqualTo( *newStyle, false))
			{
				//concat with previous child style
				styleLastChild->SetRange( startLastChild, inEnd);
			}
			else
			{
				bool childInserted = false;
				if (inRecLevel <= 5 && styleLastChild && !styleLastChild->IsSpanRef() && endLastChild == start)
				{
					VTextStyle *newStyle2 = new VTextStyle( newStyle);
					VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( newStyle2);
					if (styleCommon)
					{
						if (styleLastChild->IsUndefined())
						{
							//set last child styles to common styles, expand range & add new styles minus common prop as children
							*styleLastChild = *styleCommon;
							delete styleCommon;
							styleLastChild->SetRange( startLastChild, inEnd);
							VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
							newStyle2->SetRange( start, inEnd);
							stylesLastChild->AddChild( newStyles2);
							newStyles2->Release();
							childInserted = true;
						}
						else
						{
							//insert new child with common styles & for children the last child & new styles minus common properties
							xbox_assert(stylesLastChild == GetNthChild( i-1));
							stylesLastChild->Retain();
							RemoveChildAt(i-1);

							styleCommon->SetRange( startLastChild, inEnd);
							VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
							newStyles->AddChild( stylesLastChild);
							stylesLastChild->Release();
							if (!newStyle2->IsUndefined())
							{
								VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
								newStyle2->SetRange( start, inEnd);
								newStyles->AddChild( newStyles2);
								newStyles2->Release();
							}	
							else
								delete newStyle2;
							
							AddChild( newStyles);
							newStyles->Release();
							childInserted = true;
						}
					}
					else
						delete newStyle2;
				}
				if (!childInserted)
				{
					//add new child style
					VTreeTextStyle *newStyles = new VTreeTextStyle( new VTextStyle( newStyle));
					newStyles->GetData()->SetRange( start, inEnd);
					AddChild( newStyles);
					newStyles->Release();
				}
			}
		}
		else if (inRangeEmpty)
		{
			//new style range is empty & has not been inserted yet -> append new style now
			VTreeTextStyle *newStyles = new VTreeTextStyle( new VTextStyle( newStyle));
			newStyles->GetData()->SetRange( inStart, inEnd);
			AddChild( newStyles);
			newStyles->Release();
		}
	}
	if (GetChildCount() == 1)
	{
		VTreeTextStyle *childStyles = GetNthChild(1);
		sLONG end;
		childStyles->GetData()->GetRange( start, end);
		if (!childStyles->GetData()->IsSpanRef() &&  start <= curStart && end >= curEnd)
		{
			//merge cur style whith child node style & branch child node children to current node
			childStyles->Retain();
			RemoveChildAt(1);
			GetData()->MergeWithStyle( childStyles->GetData(), inStyleInherit);
			sLONG childCount = childStyles->GetChildCount();
			for (sLONG j = 1; j <= childCount; j++)
			{
				VTreeTextStyle *child = childStyles->GetNthChild(1);
				child->Retain();
				childStyles->RemoveChildAt(1);
				AddChild( child);
				child->Release();
			}
			childStyles->Release();
		}
	}

	if (styleInheritForChilds)
		delete styleInheritForChilds;
	delete newStyle;

	//in order to prevent stack overflow, 
	//we convert cascading styles to sequential styles if recursivity level is too high
	if (inRecLevel > 5 && GetChildCount())
		BuildSequentialStylesFromCascadingStyles( NULL);
}


/** merge current style recursively with the specified style 
@remarks
	input style properties which are defined override current style properties without taking account input style range
	(current style range is preserved)
*/
void VTreeTextStyle::_MergeWithStyle( const VTextStyle *inStyle, VTextStyle *inStyleInherit)
{
	VTextStyle *styleInherit = NULL;

	GetData()->MergeWithStyle( inStyle, inStyleInherit ? inStyleInherit : (GetParent() ? GetParent()->GetData() : NULL));

	if (GetChildCount() > 0)
	{
		if (inStyleInherit)
		{
			styleInherit = new VTextStyle( inStyleInherit);
			styleInherit->MergeWithStyle( GetData(), NULL);
		}
		else if (GetParent())
		{
			styleInherit = new VTextStyle(GetParent()->GetData());
			styleInherit->MergeWithStyle( GetData(), NULL);		
		}
	}

	for (int j = GetChildCount(); j >= 1; j--)
	{
		GetNthChild(j)->_MergeWithStyle( inStyle, styleInherit);
		
		if (GetNthChild(j)->GetChildCount() == 0 && GetNthChild(j)->IsUndefined(true))
			RemoveChildAt(j);
	}

	if (styleInherit)
		delete styleInherit;
}

/** return true if it is undefined */
bool VTreeTextStyle::IsUndefined(bool inTrueIfRangeEmpty) const
{
	if (!GetData())
		return true;

	if (inTrueIfRangeEmpty)
	{
		sLONG start = 0, end = 0;
		GetData()->GetRange( start, end);
		if (start >= end)
			return true;
	}

	if (!(GetData()->IsUndefined()))
		return false;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *style = GetNthChild(i);
		if (!style->IsUndefined( inTrueIfRangeEmpty))
			return false;
	}
	return true;
}



/** create new tree text style from the current but constrained to the specified range & eventually relative to the start of the specified range */ 
VTreeTextStyle *VTreeTextStyle::CreateTreeTextStyleOnRange( const sLONG inStart, const sLONG inEnd, bool inRelativeToStart, bool inNULLIfRangeEmpty, bool inNoMerge) const
{
	if (inNULLIfRangeEmpty && inStart == inEnd)
		return NULL;

	const VTextStyle *style = GetData();
	if (style)
	{
		sLONG s,e;
		style->GetRange( s, e);
		if (s > inEnd)
			return NULL;
		if (e < inStart)
			return NULL;

		if (s < inStart)
			s = inStart;
		//if (s > inEnd)
		//	s = inEnd;
		//if (e < inStart)
		//	e = inStart;
		if (e > inEnd)
			e = inEnd;
		if ((inNULLIfRangeEmpty || style->IsSpanRef()) && e-s <= 0)
			return NULL;
		if (inRelativeToStart)
		{
			s -= inStart;
			e -= inStart;
		}
		VTextStyle *newStyle = new VTextStyle( style);
		newStyle->SetRange( s, e);

		VTreeTextStyle *styles = new VTreeTextStyle( newStyle);

		bool doMerge = !inNoMerge; 
		for (int i = 1; i <= GetChildCount(); i++)
		{
			VTreeTextStyle *childStyles = GetNthChild( i);
			VTreeTextStyle *newChildStyles = childStyles->CreateTreeTextStyleOnRange( inStart, inEnd, inRelativeToStart, inNULLIfRangeEmpty, inNoMerge);
			if (newChildStyles)
			{
				sLONG childStart, childEnd;
				newChildStyles->GetData()->GetRange( childStart, childEnd);
				if (doMerge && ((s == e) || !newChildStyles->GetData()->IsSpanRef()) && childStart == s && childEnd == e)
				{
					//optimization: merge child style with current style if child style range is equal to current style range
					styles->_MergeWithStyle( newChildStyles->GetData());
					std::vector<VTreeTextStyle *> vstylesToAdd;
					for (int j = 1; j <= newChildStyles->GetChildCount(); j++)
					{
						VTreeTextStyle *stylesToAdd = newChildStyles->GetNthChild( j);
						stylesToAdd->Retain();
						vstylesToAdd.push_back(stylesToAdd);
					}
					newChildStyles->RemoveChildren();
				
					std::vector<VTreeTextStyle *>::iterator itStyles = vstylesToAdd.begin();
					for (;itStyles != vstylesToAdd.end(); itStyles++)
					{
						styles->AddChild( *itStyles);
						(*itStyles)->Release();
					}
					newChildStyles->Release();
				}
				else
				{
					styles->AddChild( newChildStyles);
					newChildStyles->Release();
				}
			}
		}
		return styles;
	}
	return NULL;
}


/** create & return uniform style on the specified range 
@remarks
	undefined value is set for either mixed or undefined style 

	mixed status is returned optionally in outMaskMixed using VTextStyle::Style mask values
	return value is always not NULL
*/
VTextStyle *VTreeTextStyle::CreateUniformStyleOnRange( sLONG inStart, sLONG inEnd, bool inRelativeToStart, uLONG *outMaskMixed) const
{
	VTreeTextStyle *styles = CreateTreeTextStyleOnRange( inStart, inEnd, inRelativeToStart, inEnd > inStart);
	
	VTextStyle *style = new VTextStyle();
	if (inRelativeToStart)
	{
		inEnd -= inStart;
		inStart = 0;
	}
	style->SetRange( inStart, inEnd);

	if (outMaskMixed)
		*outMaskMixed = 0;
	if (styles)
	{
		std::vector<VTextStyle *> vstyles;
		styles->BuildSequentialStylesFromCascadingStyles( vstyles);
		ReleaseRefCountable(&styles);

		std::vector<VTextStyle *>::iterator it = vstyles.begin();
		sLONG curEnd = inStart;
		for(;it != vstyles.end(); it++)
		{
			VTextStyle *styleCur = *it;

			sLONG styleCurStart, styleCurEnd;
			styleCur->GetRange( styleCurStart, styleCurEnd);
			if (!testAssert(styleCurStart == curEnd))
			{
				//if there is a hole between styles, style is not uniform at all 
				//(but should never happen after call to BuildSequentialStylesFromCascadingStyles)
				style->Reset();
				if (outMaskMixed)
					*outMaskMixed = VTextStyle::eStyle_Mask;
				break;
			}
			xbox_assert(styleCurStart == curEnd);

			if (it == vstyles.begin())
			{
				*style = *styleCur;
				style->SetRange( inStart, inEnd);
			}
			else
			{
				//check if contiguous styles have uniform style
				if (style->GetBold() != styleCur->GetBold())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Bold;
					style->SetBold(UNDEFINED_STYLE);
				}
				if (style->GetItalic() != styleCur->GetItalic())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Italic;
					style->SetItalic(UNDEFINED_STYLE);
				}
				if (style->GetUnderline() != styleCur->GetUnderline())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Underline;
					style->SetUnderline(UNDEFINED_STYLE);
				}
				if (style->GetStrikeout() != styleCur->GetStrikeout())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Strikeout;
					style->SetStrikeout(UNDEFINED_STYLE);
				}
				if (style->GetHasForeColor() != styleCur->GetHasForeColor() || (style->GetHasForeColor() && style->GetColor() != styleCur->GetColor()))
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Color;
					style->SetHasForeColor(false);
				}
				if (style->GetHasBackColor() != styleCur->GetHasBackColor() || (style->GetHasBackColor() && style->GetBackGroundColor() != styleCur->GetBackGroundColor()))
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_BackColor;
					style->SetHasBackColor(false);
				}
				if (style->GetFontName() != styleCur->GetFontName())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Font;
					style->SetFontName(CVSTR(""));
				}
				if (style->GetFontSize() != styleCur->GetFontSize())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_FontSize;
					style->SetFontSize( -1.0);
				}
				if (style->GetJustification() != styleCur->GetJustification())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | VTextStyle::eStyle_Just;
					style->SetJustification(JST_Notset);
				}
#if ENABLE_VTEXTSTYLE_EXT
				if (style->GetStylesExt() && styleCur->GetStylesExt())
					style->GetStylesExt()->ResetUnCommonStyles( styleCur->GetStylesExt(), outMaskMixed);
				else if (style->GetStylesExt())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | style->GetStylesExt()->GetMaskSet();
					style->GetStylesExt()->Reset();
				}
				else if (styleCur->GetStylesExt())
				{
					if (outMaskMixed)
						*outMaskMixed = *outMaskMixed | styleCur->GetStylesExt()->GetMaskSet();
				}
#endif
			}
			curEnd = styleCurEnd;
		}

		it = vstyles.begin();
		for(;it != vstyles.end(); it++)
			delete *it;
	}
	return style;
}


bool VTreeTextStyle::GetFontName(sLONG inStart, sLONG inEnd, VString& outFontName) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (!style->GetFontName().IsEmpty())
		outFontName = style->GetFontName();

	VString prevFontName = outFontName;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetFontName( inStart, inEnd, outFontName);
		if (!ok)
			return false;
		if (!outFontName.EqualToString( prevFontName, true))
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    outFontName = prevFontName;
				return false;
			}
			prevFontName = outFontName;
		}
	}
	return true;
}

bool VTreeTextStyle::GetFontSize(sLONG inStart, sLONG inEnd, Real& ioFontSize) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetFontSize() >= 0)
		ioFontSize = style->GetFontSize();

	Real prevFontSize = ioFontSize;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetFontSize( inStart, inEnd, ioFontSize);
		if (!ok)
			return false;
		if (ioFontSize != prevFontSize)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioFontSize = prevFontSize;
				return false;
			}
			prevFontSize = ioFontSize;
		}
	}
	return true;
}

/** get text color on the specified range 
@return
	true: text color is used on all range
	false: text color is mixed: return text color of the font used by range start character
*/
bool VTreeTextStyle::GetTextForeColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetHasForeColor())
		outColor = style->GetColor();

	RGBAColor prevColor = outColor;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetTextForeColor( inStart, inEnd, outColor);
		if (!ok)
			return false;
		if (outColor != prevColor)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    outColor = prevColor;
				return false;
			}
			prevColor = outColor;
		}
	}
	return true;
}

/** get back text color on the specified range 
@return
	true: back text color is used on all range
	false: back text color is mixed: return back text color of the font used by range start character
*/
bool VTreeTextStyle::GetTextBackColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetHasBackColor())
		outColor = style->GetBackGroundColor();

	RGBAColor prevColor = outColor;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetTextBackColor( inStart, inEnd, outColor);
		if (!ok)
			return false;
		if (outColor != prevColor)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    outColor = prevColor;
				return false;
			}
			prevColor = outColor;
		}
	}
	return true;
}


bool VTreeTextStyle::GetBold( sLONG inStart, sLONG inEnd, sLONG& ioBold) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetBold() >= 0)
		ioBold = style->GetBold();

	sLONG prevBold = ioBold;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetBold( inStart, inEnd, ioBold);
		if (!ok)
			return false;
		if (ioBold != prevBold)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioBold = prevBold;
				return false;
			}
			prevBold = ioBold;
		}
	}
	return true;
}

bool VTreeTextStyle::GetItalic(sLONG inStart, sLONG inEnd, sLONG& ioItalic) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetItalic() >= 0)
		ioItalic = style->GetItalic();

	sLONG prevItalic = ioItalic;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetItalic( inStart, inEnd, ioItalic);
		if (!ok)
			return false;
		if (ioItalic != prevItalic)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioItalic = prevItalic;
				return false;
			}
			prevItalic = ioItalic;
		}
	}
	return true;
}

bool VTreeTextStyle::GetUnderline(sLONG inStart, sLONG inEnd, sLONG& ioUnderline) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetUnderline() >= 0)
		ioUnderline = style->GetUnderline();

	sLONG prevUnderline = ioUnderline;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetUnderline( inStart, inEnd, ioUnderline);
		if (!ok)
			return false;
		if (ioUnderline != prevUnderline)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioUnderline = prevUnderline;
				return false;
			}
			prevUnderline = ioUnderline;
		}
	}
	return true;
}


bool VTreeTextStyle::GetStrikeout(sLONG inStart, sLONG inEnd, sLONG& ioStrikeout) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetStrikeout() >= 0)
		ioStrikeout = style->GetStrikeout();

	sLONG prevStrikeout = ioStrikeout;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetStrikeout( inStart, inEnd, ioStrikeout);
		if (!ok)
			return false;
		if (ioStrikeout != prevStrikeout)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioStrikeout = prevStrikeout;
				return false;
			}
			prevStrikeout = ioStrikeout;
		}
	}
	return true;
}

/** get style on the specified range as sLONG value using a custom function
@return
	true: style value is uniform on range
	false: style value is not uniform on range: return style value used by range start character
*/
bool VTreeTextStyle::GetStyleAsLong( sLONG inStart, sLONG inEnd, sLONG& ioValue, fnGetStyleLong inFnGetStyle, uLONG inUserData, VTextStyle *inStyleInherit) const
{
	xbox_assert(inFnGetStyle);

	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inEnd == -1)
		inEnd = end;

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	sLONG undefined = -1;
	if (inStyleInherit)
	{
		inStyleInherit = new VTextStyle(inStyleInherit);
		inStyleInherit->SetSpanRef(NULL);
		inStyleInherit->MergeWithStyle(style);
		style = inStyleInherit;
	}
	sLONG value = inFnGetStyle(style, undefined, inUserData);
	if (value != undefined)
		ioValue = value;

	sLONG prevValue = ioValue;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		if (!inStyleInherit)
		{
			if (style->GetUnderline() == TRUE || style->GetStrikeout() == TRUE) //ensure we take account inherited underline/strikeout status (for proper underline or strikeout style/color parsing)
				inStyleInherit = new VTextStyle(style);
		}
		bool ok = childStyles->GetStyleAsLong( inStart, inEnd, ioValue, inFnGetStyle, inUserData, inStyleInherit);
		if (!ok)
		{
			if (inStyleInherit)
				delete inStyleInherit;
			return false;
		}
		if (ioValue != prevValue)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioValue = prevValue;
				if (inStyleInherit)
					delete inStyleInherit;
				return false;
			}
			prevValue = ioValue;
		}
	}
	if (inStyleInherit)
		delete inStyleInherit;
	return true;
}

bool VTreeTextStyle::GetJustification(sLONG inStart, sLONG inEnd, eStyleJust& ioJustification) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetJustification() >= 0)
		ioJustification = style->GetJustification();

	eStyleJust prevJustification = ioJustification;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetJustification( inStart, inEnd, ioJustification);
		if (!ok)
			return false;
		if (ioJustification != prevJustification)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
                if (childStart > inStart) //ensure we keep child style if and only if it overrides input range first character (for mixed style, we return first char style)
                    ioJustification = prevJustification;
				return false;
			}
			prevJustification = ioJustification;
		}
	}
	return true;
}


/** replace styled text at the specified range with text 
@remarks
	replaced text will inherit only uniform style on replaced range

	return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
*/
VIndex VTreeTextStyle::ReplaceStyledText( VString& ioText, VTreeTextStyle*& ioStyles, const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
										bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, const ISpanTextRef * inPreserveSpanTextRef, 
										void *inUserData, fnBeforeReplace inBeforeReplace, fnAfterReplace inAfterReplace)
{
	//replace selected text or insert text

	const VString *pText = &inText;
	VString text;
	if (inBeforeReplace)
	{
		text = inText;
		inBeforeReplace( inUserData, inStart, inEnd, text);
		pText = &text;
	}

	if (inStart < 0)
		inStart = 0;
	if (inEnd < 0 || inEnd > ioText.GetLength())
		inEnd = ioText.GetLength();
	if (inStart > inEnd)
		return 0;

	if (ioStyles)
	{
		if (inAutoAdjustRangeWithSpanRef)
			ioStyles->AdjustRange( inStart, inEnd);
		else if (!inSkipCheckRange && !ioStyles->CanDeleteOrReplace( inStart, inEnd))
			return 0;
	}

	if (ioStyles)
	{
		bool isStartOfParagraph = false;
		if (!pText->IsEmpty() && inEnd < ioText.GetLength() && (inStart <= 0 || ioText[inStart-1] == 13)) 
			//ensure inserted text will inherit style from paragraph first char if inserted at start of not empty paragraph
			isStartOfParagraph = true;

		bool done = false;
		if (inEnd > inStart && !pText->IsEmpty()) //same as at start of paragraph as we want new text to inherit previous text uniform style and not style from previous character
		{
			isStartOfParagraph = true;

			if (inInheritUniformStyle && !inPreserveSpanTextRef)
			{
				//style might not be uniform on the specified range: ensure we inherit only the first char style
				ioStyles->FreezeSpanRefs(inStart, inEnd); //reset span references but keep normal style (we need to reset before as we truncate here not the full replaced range)
				if (inEnd > inStart+1)
					ioStyles->Truncate( inStart+1, inEnd-(inStart+1)); 
				if (pText->GetLength() > 1)
					ioStyles->ExpandAtPosBy( inStart+1, pText->GetLength()-1, NULL, NULL, false, false);
				done = true;
			}
		}
		if (!done)
		{
			if (inEnd > inStart)
				ioStyles->Truncate( inStart, inEnd-inStart, 
									(inInheritUniformStyle && !pText->IsEmpty()) ? false : true,	//true  -> new text will inherit only style from inserted position 
																									//false -> new text will inherit style from replaced text uniform style 
									inPreserveSpanTextRef != NULL); //if != NULL, span refs are preserved even if style range length is reset to 0 (only while updating a span ref plain text)
			if (!pText->IsEmpty())
				ioStyles->ExpandAtPosBy( inStart, pText->GetLength(), inPreserveSpanTextRef, NULL, false, isStartOfParagraph);
		}
	}

	if (inEnd > inStart)
		ioText.Replace( *pText, inStart+1, inEnd-inStart);
	else if (pText->GetLength() > 0)
		ioText.Insert( *pText, inStart+1);

	if (inStyles)
	{
		if (!ioStyles)
		{
			ioStyles = new VTreeTextStyle( new VTextStyle());
			ioStyles->GetData()->SetRange( 0, ioText.GetLength());
		}
		ioStyles->ApplyStyles( inStyles, true, inStart);
	}

	if (inAfterReplace)
		inAfterReplace(inUserData, inStart, inStart+pText->GetLength());
	return (pText->GetLength()+1);
}

/** set and own span text reference */
void VTextStyle::SetSpanRef( ISpanTextRef *inRef)
{
	if (fSpanTextRef == inRef)
		return;

	if (fSpanTextRef)
		delete fSpanTextRef;
	fSpanTextRef = inRef;
}


#if ENABLE_VTEXTSTYLE_EXT
/** set and own extended styles reference (pass NULL to destroy current reference) */
void VTextStyle::SetStylesExt( ITextStyleExt *inStylesExt)
{
	if (fStylesExt == inStylesExt)
		return;
	if (fStylesExt)
		delete fStylesExt;
	fStylesExt = inStylesExt;
	if (fStylesExt)
		fStylesExt->NotifyAttach( this);
}

/** get extended styles (create it if not done yet) */
ITextStyleExt *VTextStyle::GetStylesExtAndCreate()
{
	if (!fStylesExt && testAssert(ITextStyleExtFactory::Get()))
		fStylesExt = ITextStyleExtFactory::Get()->Create();
	return fStylesExt;
}
#endif


/** convert uniform character index (the index used by 4D commands) to actual plain text position 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VTreeTextStyle::CharPosFromUniform( sLONG& ioPos) const
{
	if (!HasSpanRefs())
		return;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (ioPos <= start)
				break;
			ioPos += end-start-1;
		}
	}
}

/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VTreeTextStyle::CharPosToUniform( sLONG& ioPos) const
{
	if (!HasSpanRefs())
		return;

	sLONG delta = 0;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (ioPos <= start)
				break;
			if (ioPos < end)
				delta -= ioPos-start; //move to uniform start
			else 
				delta -= end-(start+1); //move to uniform end
		}
	}
	ioPos += delta;
}

/** convert uniform character range (as used by 4D commands) to actual plain text position 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VTreeTextStyle::RangeFromUniform( sLONG& inStart, sLONG& inEnd) const
{
	if (!HasSpanRefs())
		return;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (inStart > start)
			{
				sLONG delta = end-start-1; 
				inStart += delta;
				inEnd += delta;
			}
			else if (inEnd <= start)
				break;
			else
				inEnd += end-start-1;
		}
	}
}

/** convert actual plain text range to uniform range (as used by 4D commands) 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VTreeTextStyle::RangeToUniform( sLONG& inStart, sLONG& inEnd) const
{
	if (!HasSpanRefs())
		return;

	sLONG deltaStart = 0;
	sLONG deltaEnd = 0;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (inStart > start)
			{
				if (inStart < end)
					deltaStart -= inStart-start; //move to uniform start
				else 
					deltaStart -= end-(start+1); //move to uniform end
				if (inEnd < end)
					deltaEnd -= inEnd-start; //move to uniform start
				else
					deltaEnd -= end-(start+1); //move to uniform end
			} 
			else if (inEnd <= start)
				break;
			else if (inEnd < end)
				deltaEnd -= inEnd-start; //move to uniform start
			else
				deltaEnd -= end-(start+1);  //move to uniform end
		}
	}
	inStart += deltaStart;
	inEnd += deltaEnd;
}

/** adjust selection 
@remarks
	you should call this method to adjust selection if styles contain span references 
*/
void VTreeTextStyle::AdjustRange(sLONG& ioStart, sLONG& ioEnd) const
{
	if (!HasSpanRefs())
		return;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			//range should be rounded to span reference range 
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			sLONG update = 0;
			if (ioStart > start && ioStart < end)
			{
				ioStart = start;
				update++;
			}
			if (ioEnd > start && ioEnd < end)
			{
				ioEnd = end;
				update++;
			}
			if (update == 2 || ioEnd <= end)
				break;
		}
	}
}


/** adjust char pos
@remarks
	you should call this method to adjust char pos if styles contain span references 
*/
void VTreeTextStyle::AdjustPos(sLONG& ioPos, bool inToEnd) const
{
	if (!HasSpanRefs())
		return;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			//range should be rounded to span reference range 
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (ioPos > start && ioPos < end)
			{
				ioPos = inToEnd ? end : start;
				return;
			}
			if (ioPos <= end)
				break;
		}
	}
}

/** replace text with span text reference on the specified range
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VTreeTextStyle::ReplaceAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, ISpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inNoUpdateRef, 
										  void *inUserData, fnBeforeReplace inBeforeReplace, fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	xbox_assert(inSpanRef);

	//xbox_assert(!inSpanRef->GetImage() || (inSpanRef->GetImage()->GetParent() == NULL && inSpanRef->GetImage()->GetDocumentNode() == NULL)); //if image, it should not be bound to a document yet

	VTextStyle *style = new VTextStyle();
	style->SetSpanRef( inSpanRef);
	if (inNoUpdateRef)
		style->GetSpanRef()->ShowRef( kSRTT_Uniform | (style->GetSpanRef()->ShowRef() & kSRTT_4DExp_Normalized));
	if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp && style->GetSpanRef()->ShowRefNormalized() && (style->GetSpanRef()->ShowRef() & kSRTT_Src))
		style->GetSpanRef()->UpdateRefNormalized( inLC);

	VString textSpanRef = style->GetSpanRef()->GetDisplayValue(); 
	style->SetRange(0, textSpanRef.GetLength()); 
	VTreeTextStyle *styles = new VTreeTextStyle( style);
	bool result = ReplaceStyledText( ioText, ioStyles, textSpanRef, styles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, true, NULL, inUserData, inBeforeReplace, inAfterReplace) != 0;
	ReleaseRefCountable(&styles);
	return result;
}


/** insert span text reference at the specified text position 
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VTreeTextStyle::InsertAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, ISpanTextRef* inSpanRef, sLONG inPos, bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inNoUpdateRef, 
										  void *inUserData, fnBeforeReplace inBeforeReplace, fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	return ReplaceAndOwnSpanRef( ioText, ioStyles, inSpanRef, inPos, inPos, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inNoUpdateRef, inUserData, inBeforeReplace, inAfterReplace, inLC);
}



/** return span reference at the specified text position */
const VTextStyle *VTreeTextStyle::GetStyleRefAtPos(sLONG inPos) const
{
	if (!HasSpanRefs())
		return NULL;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (inPos >= start && inPos < end)
				return (*it)->GetData();
			if (inPos < end)
				break;
		}
	}
	return NULL;
}

/** return the first span reference which intersects the passed range */
const VTextStyle *VTreeTextStyle::GetStyleRefAtRange(const sLONG inStart, const sLONG _inEnd) const
{
	if (!HasSpanRefs())
		return NULL;

	sLONG inEnd = _inEnd;
	if (inEnd < 0)
	{
		sLONG start, end;
		GetData()->GetRange( start, end);
		inEnd = end;
	}

	if (inStart == inEnd)
		return GetStyleRefAtPos( inStart);

	if (inStart > inEnd)
		return NULL;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			sLONG start, end;
			(*it)->GetData()->GetRange( start, end);
			if (inStart < end && inEnd > start)
				return (*it)->GetData();
			if (inEnd <= end)
				break;
		}
	}
	return NULL;
}


/** show or hide span ref value or source
@remarks
	propagate flag to all VSpanTextRef & mark all as dirty
*/ 
void VTreeTextStyle::ShowSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs)
{
	if (!HasSpanRefs())
		return;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			ISpanTextRef *spanRef = style->GetSpanRef();
			spanRef->ShowRef( spanRef->CombinedTextTypeMaskToTextTypeMask( inShowSpanRefs));
		}
	}
}

/** highlight or not span references 
@remarks
	propagate flag to all VSpanTextRef 

	on default (if VSpanTextRef delegate is not overriden) it changes only highlight status for value
	because if reference is displayed, it is always highlighted
*/ 
void VTreeTextStyle::ShowHighlight( bool inShowHighlight)
{
	if (!HasSpanRefs())
		return;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
			(*it)->GetData()->GetSpanRef()->ShowHighlight( inShowHighlight); 
	}
}

/** call this method to update span reference style while mouse enters span ref 
@remarks
	return true if style has changed
*/
bool VTreeTextStyle::NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef)
{
	xbox_assert(inStyleSpanRef);
	if (!HasSpanRefs())
		return false;
	bool styleHasChanged = false;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
		{
			if ((*it)->GetData() == inStyleSpanRef)
				styleHasChanged = (*it)->GetData()->GetSpanRef()->OnMouseEnter() || styleHasChanged;
			else
				styleHasChanged = (*it)->GetData()->GetSpanRef()->OnMouseLeave() || styleHasChanged;
		}
	}
	return styleHasChanged;
}


/** call this method to update span reference style while mouse leaves span ref 
@remarks
	return true if style has changed
*/
bool VTreeTextStyle::NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef)
{
	if (!HasSpanRefs())
		return false;
	
	//if inStyleSpanRef is NULL, we notify all span refs to ensure status is updated

	bool styleHasChanged = false;
	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		if ((*it)->GetData()->IsSpanRef())
			styleHasChanged = (*it)->GetData()->GetSpanRef()->OnMouseLeave() || styleHasChanged;
	}
	return styleHasChanged;
}


bool VTreeTextStyle::HasExpressions() const
{
	if (!HasSpanRefs())
		return false;

	VectorOfStyle::const_iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
			return true;
	}
	return false;
}

/** reset cached 4D expressions on the specified range */
void VTreeTextStyle::ResetCacheExpressions( VDocCache4DExp *inCache4DExp, sLONG inStart, sLONG inEnd)
{
	xbox_assert(inCache4DExp);

	if (!HasSpanRefs())
		return;

	if (inStart < 0)
		inStart = 0;
	sLONG start, end;
	GetData()->GetRange(start, end);
	if (inEnd < 0 || inEnd > end)
		inEnd = end;

	VectorOfStyle::iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG refstart, refend;
			style->GetRange( refstart, refend);
			if (inEnd <= refstart)
				break;

			if (inStart < refend)
			{
				if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
				{
					//remove cached 4D expression (so it will be evaluated next time it is requested)
					inCache4DExp->Remove( style->GetSpanRef()->GetRef());
				}
			}
		}
	}
}

/** eval 4D expressions  
@remarks
	return true if any 4D expression has been evaluated
*/
bool VTreeTextStyle::EvalExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, VDocCache4DExp *inCache4DExp)
{
	if (!HasSpanRefs())
		return false;

	if (inStart < 0)
		inStart = 0;
	sLONG start, end;
	GetData()->GetRange(start, end);
	if (inEnd < 0 || inEnd > end)
		inEnd = end;

	bool done = false;
	VectorOfStyle::iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG refstart, refend;
			style->GetRange( refstart, refend);
			if (inEnd <= refstart)
				break;

			if (inStart < refend)
			{
				if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
				{
					//force to eval 4D exp & to replace plain text with evaluated string
					style->GetSpanRef()->EvalExpression( inLC, inCache4DExp);
					done = true;
				}
			}
		}
	}
	return done;
}


/** freeze expressions  
@remarks
	parse & replace any 4D expression reference with evaluated expression plain text on the passed range

	return true if any 4D expression has been replaced with plain text
*/
bool VTreeTextStyle::FreezeExpressions( VDBLanguageContext *inLC, VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart, sLONG inEnd, void *inUserData, fnBeforeReplace inBeforeReplace, fnAfterReplace inAfterReplace)
{
	if (!ioStyles.HasSpanRefs())
		return false;

	if (inStart < 0)
		inStart = 0;
	if (inEnd < 0 || inEnd > ioText.GetLength())
		inEnd = ioText.GetLength();

	bool done = false;
	VectorOfStyle::iterator it = ioStyles.fChildren.begin();
	for (;it != ioStyles.fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG refstart, refend;
			style->GetRange( refstart, refend);
			if (inEnd <= refstart)
				break;

			if (inStart < refend)
			{
				if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
				{
					//force to eval 4D exp & to replace plain text with evaluated string
					style->GetSpanRef()->ShowRef(kSRTT_Value);
					if (inLC)
						style->GetSpanRef()->EvalExpression(inLC);

					VString text = style->GetSpanRef()->GetDisplayValue(); //will reset display value dirty flag
					VTreeTextStyle *styles = &ioStyles;
					VIndex lengthReplaced = ReplaceStyledText( ioText, styles, text, NULL, refstart, refend, false, true, true, style->GetSpanRef(), inUserData, inBeforeReplace, inAfterReplace);
					style->SetSpanRef(NULL); //destroy span ref
					(*it)->NotifyClearSpanRef();

					if (lengthReplaced)
					{
						done = true;
						lengthReplaced--;
						if (inStart > refstart)
						{
							if (inStart < refend)
							{
								xbox_assert(false);
								break;
							}
							else 
								inStart = inStart-(refend-refstart)+lengthReplaced;
						}
						if (inEnd > refstart)
						{
							if (inEnd < refend)
							{
								xbox_assert(false);
								break;
							}
							else 
								inEnd = inEnd-(refend-refstart)+lengthReplaced;
						}
					}
				}
			}
		}
	}
	return done;
}


/** freeze span references  
@remarks
	reset styles span references

	this method preserves VTextStyle span reference container: it only resets VTextStyle span reference, transforming it to a normal VTextStyle
	(it does not eval 4D expressions too)
*/
void VTreeTextStyle::FreezeSpanRefs( sLONG inStart, sLONG inEnd)
{
	if (!HasSpanRefs())
		return;

	if (inStart < 0)
		inStart = 0;
	sLONG start, end;
	GetData()->GetRange( start, end);
	if (inEnd < 0 || inEnd > end)
		inEnd = end;

	VectorOfStyle::iterator it = fChildren.begin();
	for (;it != fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG refstart, refend;
			style->GetRange( refstart, refend);
			if (inEnd <= refstart)
				break;

			if (inStart < refend)
				style->SetSpanRef(NULL); //reset span ref
		}
	}
}


/** update span references plain text  
@remarks
	parse & update span references plain text according to span ref type & dirty status

	return true if plain text or styles have been updated, false otherwise
*/
bool VTreeTextStyle::UpdateSpanRefs(	VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart, sLONG inEnd, 
										void *inUserData, fnBeforeReplace inBeforeReplace, fnAfterReplace inAfterReplace, 
										VDBLanguageContext *inLC)
{
	if (!ioStyles.HasSpanRefs())
		return false;

	if (inStart < 0)
		inStart = 0;
	if (inEnd < 0 || inEnd > ioText.GetLength())
		inEnd = ioText.GetLength();

	bool done = false;
	VectorOfStyle::iterator it = ioStyles.fChildren.begin();
	for (;it != ioStyles.fChildren.end(); it++)
	{
		VTextStyle *style = (*it)->GetData();
		if (style->IsSpanRef())
		{
			sLONG refstart, refend;
			style->GetRange( refstart, refend);
			if (inEnd <= refstart)
				break;

			if (inStart < refend)
			{
				if (style->GetSpanRef()->IsDisplayValueDirty())
				{
					if (inLC
						&&
						style->GetSpanRef()->GetType() == kSpanTextRef_4DExp 
						&&
						((style->GetSpanRef()->ShowRef() & kSRTT_4DExp_Normalized) != 0) 
						&& 
						(style->GetSpanRef()->ShowRef() & kSRTT_Src) != 0)
						style->GetSpanRef()->UpdateRefNormalized( inLC);
					
					VString text = style->GetSpanRef()->GetDisplayValue(); //will reset display value dirty flag
			
					VTreeTextStyle *styles = &ioStyles;
					VIndex childCount = ioStyles.GetChildCount();
					VIndex lengthReplaced = ReplaceStyledText( ioText, styles, text, NULL, refstart, refend, false, true, true, style->GetSpanRef(), inUserData, inBeforeReplace, inAfterReplace);
					xbox_assert(childCount == ioStyles.GetChildCount()); 
					if (lengthReplaced)
					{
						done = true;
						lengthReplaced--;
						if (inStart > refstart)
						{
							if (inStart < refend)
							{
								xbox_assert(false);
								break;
							}
							else 
								inStart = inStart-(refend-refstart)+lengthReplaced;
						}
						if (inEnd > refstart)
						{
							if (inEnd < refend)
							{
								xbox_assert(false);
								break;
							}
							else 
								inEnd = inEnd-(refend-refstart)+lengthReplaced;
						}
					}
				}
			}
		}
	}
	return done;
}

/** convert CSS dimension to TWIPS */
sLONG ICSSUtil::CSSDimensionToTWIPS(const Real inDimNumber, const eCSSUnit inDimUnit, const uLONG inDPI, const sLONG inParentValue)
{
	switch (inDimUnit)
	{
	case kCSSUNIT_PERCENT:
		return (sLONG)(floor(inParentValue * inDimNumber + 0.5f));
		break;
	case kCSSUNIT_PX:
		return (sLONG)(floor(inDimNumber*1440/(inDPI ? inDPI : 72)+0.5f));
		break;
	case kCSSUNIT_PC:
		return (sLONG)(inDimNumber*12*20);
		break;
	case kCSSUNIT_PT:
		return (sLONG)(inDimNumber*20);
		break;
	case kCSSUNIT_MM:
		return (sLONG)(floor(inDimNumber*1440/25.4+0.5f));
		break;
	case kCSSUNIT_CM:
		return (sLONG)(floor(inDimNumber*1440/2.54+0.5f));
		break;
	case kCSSUNIT_IN:
		return (sLONG)(inDimNumber*1440);
		break;
	case kCSSUNIT_EM:
	case kCSSUNIT_EX:
		return (sLONG)(inDimNumber*(inParentValue ? inParentValue : 12.0f));
		break;
	case kCSSUNIT_FONTSIZE_LARGER:
		{
		sLONG fontSize = inParentValue ? inParentValue : 12.0f;
		return (sLONG)(fontSize+(0.20*fontSize));
		}
		break;
	case kCSSUNIT_FONTSIZE_SMALLER:
		{
		sLONG fontSize = inParentValue ? inParentValue : 12.0f;
		fontSize = (sLONG)(fontSize-(0.20*fontSize));
		if (fontSize <= 0)
			fontSize = (inParentValue ? inParentValue : 12.0f);
		return fontSize;
		}
		break;
	default:
		xbox_assert(false);
		return inParentValue;
		break;
	}
}

/** return 4D expression value, as evaluated from the passed 4D tokenized expression
@remarks
returns cached value if 4D exp has been evaluated yet
*/
void VDocCache4DExp::Get(const VString& inExp, VString& outValue, VDBLanguageContext *inLC)
{
	if (inLC && inLC != fDBLanguageContext)
	{
		fDBLanguageContext = inLC;
		fMapOfExp.clear();
	}

	if (!testAssert(fDBLanguageContext) || !testAssert(fSpanRef))
	{
		outValue.SetEmpty();
		return;
	}
	MapOfExp::iterator itExp = fMapOfExp.find(inExp);
	if (itExp != fMapOfExp.end())
	{
		if (fEvalAlways)
		{
			if (fEvalDeferred)
			{
				//defer exp eval
				if (fMapOfDeferredExp.size() < fMaxSize)
					fMapOfDeferredExp[inExp] = true;
			}
			else
			{
				//eval & update cached value
				fSpanRef->SetRef(inExp);
				fSpanRef->EvalExpression(fDBLanguageContext);
				itExp->second.second = fSpanRef->GetComputedValue();
			}
		}
		itExp->second.first = (uLONG)VSystem::GetCurrentTime(); //update last read access time
		outValue = itExp->second.second; //return exp value
		return;
	}

	if (fEvalDeferred)
	{
		//defer exp eval
		if (fMapOfDeferredExp.size() < fMaxSize)
			fMapOfDeferredExp[inExp] = true;
		outValue.SetEmpty();
		return;
	}

	fSpanRef->SetRef(inExp);
	fSpanRef->EvalExpression(fDBLanguageContext);
	outValue = fSpanRef->GetComputedValue();
	while (fMapOfExp.size() >= fMaxSize)
	{
		//remove the oldest entry
		MapOfExp::iterator itToRemove = fMapOfExp.begin();
		for (itExp = fMapOfExp.begin(); itExp != fMapOfExp.end(); itExp++)
		{
			if (itExp->second.first < itToRemove->second.first)
				itToRemove = itExp;
		}
		fMapOfExp.erase(itToRemove);
	}
	fMapOfExp[inExp] = ExpValue((uLONG)VSystem::GetCurrentTime(), outValue);
}

/** manually add a pair <exp,value> to the cache
@remarks
add or replace the existing value

caution: it does not check if value if valid according to the expression so it might be anything
*/
void VDocCache4DExp::Set(const VString& inExp, const VString& inValue)
{
	MapOfExp::iterator itExp = fMapOfExp.find(inExp);
	if (itExp != fMapOfExp.end())
	{
		itExp->second.first = (uLONG)VSystem::GetCurrentTime();
		itExp->second.second = inValue;
		return;
	}

	while (fMapOfExp.size() >= fMaxSize)
	{
		//remove the oldest entry
		MapOfExp::iterator itToRemove = fMapOfExp.begin();
		for (itExp = fMapOfExp.begin(); itExp != fMapOfExp.end(); itExp++)
		{
			if (itExp->second.first < itToRemove->second.first)
				itToRemove = itExp;
		}
		fMapOfExp.erase(itToRemove);
	}
	fMapOfExp[inExp] = ExpValue((uLONG)VSystem::GetCurrentTime(), inValue);
}

/** append cached expressions from the passed cache */
void VDocCache4DExp::AppendFromCache(const VDocCache4DExp *inCache)
{
	MapOfExp::const_iterator itExp = inCache->fMapOfExp.begin();
	for (; itExp != inCache->fMapOfExp.end(); itExp++)
		Set(itExp->first, itExp->second.second);
}

/** return the cache pair <exp,value> at the specified cache index (1-based) */
bool VDocCache4DExp::GetCacheExpAndValueAt(VIndex inIndex, VString& outExp, VString& outValue)
{
	MapOfExp::const_iterator itExp = fMapOfExp.begin();
	if (inIndex >= 1 && inIndex <= fMapOfExp.size())
	{
		if (inIndex > 1)
			std::advance(itExp, inIndex - 1);

		outExp = itExp->first;
		outValue = itExp->second.second;
		return true;
	}
	else
		return false;
}


/** remove the passed 4D tokenized exp from cache */
void VDocCache4DExp::Remove(const VString& inExp)
{
	MapOfExp::iterator itExp = fMapOfExp.find(inExp);
	if (itExp != fMapOfExp.end())
		fMapOfExp.erase(itExp);
}



/** eval deferred expressions & add to cache */
bool VDocCache4DExp::EvalDeferred()
{
	if (!fDBLanguageContext)
		return false;

	bool result = false;
	bool evalDeferred = fEvalDeferred;
	fEvalDeferred = false;
	MapOfDeferredExp::const_iterator itExp = fMapOfDeferredExp.begin();
	for (; itExp != fMapOfDeferredExp.end(); itExp++)
	{
		VString value;
		Get(itExp->first, value);
		if (!value.IsEmpty())
			result = true;
	}
	fMapOfDeferredExp.clear();
	fEvalDeferred = evalDeferred;
	return result;
}


sLONG IDocText::IDOC_GetReferenceAtRange(	const sLONG inStart, const sLONG inEnd, 
											sLONG *outFirstRefStart, sLONG *outFirstRefEnd, 
											VString *outFirstRefSource, VString *outFirstRefValue
										)
{
	sLONG type = -1;
	sLONG baseStart = 0;
	const VTextStyle *style = IDOC_GetStyleRefAtRange( inStart, inEnd, &baseStart);
	if (style)
	{
		type = (sLONG)style->GetSpanRef()->GetType();
		if (outFirstRefStart || outFirstRefEnd)
		{
			sLONG start, end;
			style->GetRange( start, end);
			if (outFirstRefStart)
				*outFirstRefStart = start+baseStart;
			if (outFirstRefEnd)
				*outFirstRefEnd = end+baseStart;
		}
		if (outFirstRefSource)
			*outFirstRefSource = style->GetSpanRef()->GetRef();
		if (outFirstRefValue)
			*outFirstRefValue = style->GetSpanRef()->GetComputedValue();
	}
	return type;
}
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
#include "VRect.h"
#include "vfont.h"
#include "vcolor.h"
#include "XMacStyledTextBox.h"
#include "XMacFont.h"
#include "Kernel/VKernel.h"

#if CT_USE_COCOA	
#import <Cocoa/Cocoa.h> 
#endif

XMacStyledTextBox::XMacStyledTextBox(ContextRef inContextRef, const VString& inText, const VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont, const TextRenderingMode inMode, const TextLayoutMode inLayoutMode, const GReal inCharKerning, const GReal inRefDocDPI, bool inUseNSAttributes)
: fContextRef(inContextRef), VStyledTextBox(inText, inStyles, inHwndBounds, inTextColor, inFont)
{
	fCurLayoutMode = inLayoutMode;
	fCurLayoutFrame = NULL;
	fCurLayoutWidth = inHwndBounds.GetWidth();
	fCurLayoutHeight = inHwndBounds.GetHeight();
	fCurLayoutIsDirty = true;
    fCurFont = inFont ? RetainRefCountable(inFont) : NULL;

	fNumLine = 1;

	fUseNSAttributes = inUseNSAttributes;
#if CT_USE_COCOA	
	if (fUseNSAttributes)
		CTHelper::EnableCocoa(); //use NSAttributedString but CFAttributedString (& NSColor etc...)
#endif
	
	//by default set reference DPI to 4D form DPI 
	_SetDocumentDPI(inRefDocDPI);

	DoInitialize();
	_SetText(inText);

	//JQ 20/01/2011 : properly initialize attributes from passed font, text rendering mode, text layout mode and char kerning
	xbox_assert(inFont);
	VFont *fontScaled = NULL;
	if (inFont && inRefDocDPI != 72.0f)
		//we need to scale font according to DPI (assuming input font is 4D form DPI compliant font ie font created with dpi = 72)
		//(styles fonts will be scaled by DoApplyStyle)
		fontScaled = VFont::RetainFont( inFont->GetName(), inFont->GetFace(), inFont->GetPixelSize(), inRefDocDPI);    
	CFMutableDictionaryRef dictAttributes = CTHelper::CreateStringAttributes( fontScaled ? fontScaled : inFont, inTextColor, inMode, inLayoutMode, inCharKerning);
	ReleaseRefCountable(&fontScaled);

	CFRange range = CFRangeMake(0, CFAttributedStringGetLength(fAttributedString));
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
		NSRange nsRange = { static_cast<NSUInteger>(range.location), static_cast<NSUInteger>(range.length) };
		[nsAttString setAttributes:(NSDictionary *)dictAttributes range:nsRange];
	}
	else
#endif		
		CFAttributedStringSetAttributes( fAttributedString, range, dictAttributes, true);
	CFRelease(dictAttributes);
	// useless because done yet by above method
	//_SetFont(inFont, inTextColor);
	
	if (inLayoutMode & TLM_ALIGN_RIGHT)
		fCurJustUniform = JST_Right;
	else if (inLayoutMode & TLM_ALIGN_CENTER)
		fCurJustUniform = JST_Center;
	else if (inLayoutMode & TLM_ALIGN_LEFT)
		fCurJustUniform = JST_Left;
	else if (inLayoutMode & TLM_ALIGN_JUSTIFY)
		fCurJustUniform = JST_Justify;
	else 
		fCurJustUniform = JST_Notset;

    fIsMonostyle = true;
    
	_ApplyAllStyles(inStyles, NULL);
	fInitialized = true;

//	Initialize(inText, inStyles, inTextColor, inFont);
}

XMacStyledTextBox::~XMacStyledTextBox()
{
    ReleaseRefCountable(&fCurFont);
    
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
        NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
        if (nsAttString)
            [nsAttString release];
    }
#endif
    else if (fAttributedString)
        CFRelease(fAttributedString);
    
#if CT_USE_COCOA
	if (fUseNSAttributes)
		CTHelper::DisableCocoa();
    fUseNSAttributes = false;
#endif
    
	if (fCurLayoutFrame)
		CFRelease(fCurLayoutFrame);
}


/** set document reference DPI
@remarks
	by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
	but you can override it if needed
*/
void XMacStyledTextBox::_SetDocumentDPI( const GReal inDPI)
{
	fFontSizeToDocumentFontSize = inDPI / 72.0f;
}

Boolean XMacStyledTextBox::GetRTFText(VString& outRTFText)
{
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		if (!fAttributedString)
			return NULL;
		
		CFDataRef data = CTHelper::CreateRTFFromAttributedString( fAttributedString);
		if (!data)
			return FALSE;
		CFStringRef text = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, data, kCFStringEncodingUTF8);
		if (!text)
			//normally RTF should be encoded in UTF8 by NSAttributedString::RTFFromRange but just in case try with Unicode
			text = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, data, kCFStringEncodingUnicode);
		if (!text)
		{
			CFRelease(data);
			return FALSE;
		}
		outRTFText.MAC_FromCFString(text);
		CFRelease(text);
		CFRelease(data);
		return TRUE;
	}
	else
#endif
		return FALSE;
}

Boolean	XMacStyledTextBox::SetRTFText(const VString& inRTFText)
{
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		if (inRTFText.IsEmpty())
		{
			//clear text
            NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
            if (nsAttString)
                [nsAttString release];
 			
			nsAttString = [[NSMutableAttributedString alloc] init];
			fAttributedString = (CFMutableAttributedStringRef)nsAttString;
			fCurLayoutIsDirty = true;
			return TRUE;
		}
		
		//we need to re-encode to UTF8
		VSize size = (inRTFText.GetLength()+1)*3;
		VPtr ptr = VMemory::NewPtr(size, 0);
		if (!ptr)
			return FALSE;
		inRTFText.ToBlock(ptr, size, VTC_UTF_8, true, false);
		
		CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)ptr, (CFIndex)(strlen((char *)ptr)+1), kCFAllocatorNull);
		if (!data)
		{
			VMemory::DisposePtr(ptr);
			return FALSE;
		}
		Boolean ok = FALSE;
		CFMutableAttributedStringRef attStringMutable = CTHelper::CreateAttributeStringFromRTF( data);
		if (attStringMutable)
		{
            NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
            if (nsAttString)
                [nsAttString release];
 			fAttributedString = attStringMutable;
			fCurLayoutIsDirty = true;
			ok = TRUE;
		}
		CFRelease(data);
		
		VMemory::DisposePtr(ptr);
		return ok;
	}
	else
#endif
		return FALSE;
}

//on Mac, we can set/get directly the RTF or HTML stream

Boolean	XMacStyledTextBox::SetHTMLText(VHandle inHandleHTML)
{
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		if (!inHandleHTML)
			return false;
		
		Boolean ok = FALSE;
		VSize size = VMemory::GetHandleSize(inHandleHTML);
		if (!size)
		{
			//clear text
            NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
            if (nsAttString)
                [nsAttString release];
			
			nsAttString = [[NSMutableAttributedString alloc] init];
			fAttributedString = (CFMutableAttributedStringRef)nsAttString;
			fCurLayoutIsDirty = true;
			ok = TRUE;
		}
		else 
		{
			VPtr ptr = VMemory::LockHandle(inHandleHTML);
			if (!ptr)
				return FALSE;
			CFDataRef data = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)ptr , (CFIndex)size);
			if (data)
			{
				CFMutableAttributedStringRef attStringMutable = CTHelper::CreateAttributeStringFromHTML( data);
				if (attStringMutable)
				{
                    NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
                    if (nsAttString)
                        [nsAttString release];
					fAttributedString = attStringMutable;
					fCurLayoutIsDirty = true;
					ok = TRUE;
				}
				CFRelease(data);
			}
			VMemory::UnlockHandle(inHandleHTML);
		}
		return ok;
	}
	else
#endif
		return FALSE;
}

VHandle XMacStyledTextBox::GetRTFText()
{
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		if (!fAttributedString)
			return NULL;
		
		CFDataRef data = CTHelper::CreateRTFFromAttributedString( fAttributedString);
		if (data)
		{
			uLONG length = CFDataGetLength(data);
			VHandle handle = VMemory::NewHandle(length);
			if (!handle)
				return NULL;
			VPtr ptr = VMemory::LockHandle(handle);
			if (!ptr)
			{
				VMemory::DisposeHandle(handle);
				return NULL;
			}
			CFDataGetBytes(data, CFRangeMake(0,length), (UInt8*)ptr);
			VMemory::UnlockHandle(handle);
			CFRelease(data);
			return handle;
		}
		else 
			return NULL;
	}
	else
#endif
		return NULL;
}

Boolean	XMacStyledTextBox::SetRTFText(VHandle inHandleRTF)
{
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		if (!inHandleRTF)
			return false;
		
		Boolean ok = FALSE;
		VSize size = VMemory::GetHandleSize(inHandleRTF);
		if (!size)
		{
			//clear text
            NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
            if (nsAttString)
                [nsAttString release];
			
			nsAttString = [[NSMutableAttributedString alloc] init];
			fAttributedString = (CFMutableAttributedStringRef)nsAttString;
			fCurLayoutIsDirty = true;
			ok = TRUE;
		}
		else 
		{
			VPtr ptr = VMemory::LockHandle(inHandleRTF);
			if (!ptr)
				return FALSE;
			CFDataRef data = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)ptr , (CFIndex)size);
			if (data)
			{
				CFMutableAttributedStringRef attStringMutable = CTHelper::CreateAttributeStringFromRTF( data);
				if (attStringMutable)
				{
                    NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
                    if (nsAttString)
                        [nsAttString release];
					fAttributedString = attStringMutable;
					fCurLayoutIsDirty = true;
					ok = TRUE;
				}
				CFRelease(data);
			}
			VMemory::UnlockHandle(inHandleRTF);
		}
		return ok;
	}
	else
#endif
		return FALSE;
}

/** get plain text */
Boolean XMacStyledTextBox::GetPlainText( VString& outText) const
{
	if (!fAttributedString)
		return FALSE;
	CFStringRef text = CFAttributedStringGetString(fAttributedString);
	if (!text)
		return FALSE;
	
	VString tempText;
	tempText.MAC_FromCFString(text);
	
	outText.Clear();
	outText.EnsureSize(tempText.GetLength());
	const UniChar *cin = tempText.GetCPointer();
	for(;*cin != 0;)
	{
		UniChar c = *cin++;
		if (c == 0x0A	//text set from SetRTFText or SetHTMLText might use 0x0A line endings 
						//so we need to convert it to Mac OS line ending.
			||
			c == 0x0B	//convert vertical tab to line ending
			||
			c == 0x0C)	//convert form feed to line ending
			outText.AppendUniChar(0x0D);
		else 
			outText.AppendUniChar(c);
			
	}
	return TRUE;
}

/* get number of lines */
sLONG XMacStyledTextBox::GetNumberOfLines()
{
	return fNumLine;
}

/** get uniform style on the specified range 
@remarks
	return end location of uniform range

	derived from VStyledTextBox
*/
sLONG XMacStyledTextBox::GetStyle(VTextStyle* ioStyle, sLONG rangeStart, sLONG rangeEnd)
{
	if (!fAttributedString)
	{
		ioStyle->SetRange(0, 0);
		return 0;
	}
	
	xbox_assert(ioStyle);
			
	ioStyle->Reset();
	
	//check param consistency to avoid nasty exception with CFAttributedStringGetAttributesAndLongestEffectiveRange
	CFIndex length = CFAttributedStringGetLength( fAttributedString);
	if (rangeEnd == -1)
		rangeEnd = length;

	if (rangeStart > length)
		rangeStart = length;
	if (rangeEnd > length)
		rangeEnd = length;
	if (rangeStart >= rangeEnd)
	{
		ioStyle->SetRange(rangeStart, rangeStart);
		return rangeStart;
	}
	if (rangeStart >= length)
	{
		ioStyle->SetRange(rangeStart, rangeStart);
		return rangeStart;
	}
	
	//get attributes on range
	CFRange range = {rangeStart, 0};
	CFDictionaryRef attributes = NULL;
#if CT_USE_COCOA
	NSAutoreleasePool *innerPool = NULL; //local autorelease pool
	if (CTHelper::UseCocoaAttributes())
	{
		innerPool = [[NSAutoreleasePool alloc] init]; //local autorelease pool
		
		NSRange rangeLimit = { static_cast<NSUInteger>(rangeStart), static_cast<NSUInteger>(rangeEnd-rangeStart) };
		NSRange rangeOut = { static_cast<NSUInteger>(rangeStart), 0 };
		NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
		NSDictionary *nsDict = [nsAttString attributesAtIndex:(NSUInteger)rangeStart longestEffectiveRange:(NSRangePointer)&rangeOut inRange:rangeLimit];
		range.location = rangeOut.location;
		range.length = rangeOut.length;
		attributes = (CFDictionaryRef)nsDict;
	}
	else
#endif	
		attributes = CFAttributedStringGetAttributesAndLongestEffectiveRange( fAttributedString, rangeStart, CFRangeMake(rangeStart, rangeEnd-rangeStart), &range);
	if (!attributes)
	{
		//none attributes defined map to a valid undefined VTextStyle 
#if CT_USE_COCOA
		if (CTHelper::UseCocoaAttributes())
			[innerPool release]; //here temp Obj-C references are released (to avoid leaks between Obj-C && C++)
#endif
		ioStyle->SetRange(rangeStart, rangeEnd);
		return rangeEnd;
	}

	//get font ref
	CTFontRef fontRef = (CTFontRef)CFDictionaryGetValue(attributes, kCTFontAttributeName);
	if (fontRef)
	{
		//get font name
		CFStringRef name = CTFontCopyFamilyName(fontRef);
		xbox_assert(name);
		
		VString sname;
		sname.MAC_FromCFString(name);
		CFRelease(name);
		
		ioStyle->SetFontName(sname);
		
		//get font size
		CGFloat fontSize = CTFontGetSize(fontRef);
		ioStyle->SetFontSize( fontSize/fFontSizeToDocumentFontSize);
		
		//get font style
		CFDictionaryRef traits = CTFontCopyTraits(fontRef);
		xbox_assert(traits);
		CTFontSymbolicTraits traitsSymbolic = 0;
		CTHelper::GetAttribute( traits, kCTFontSymbolicTrait, traitsSymbolic);
		CFRelease(traits);
		
		VFontFace face = XMacFont::CTFontSymbolicTraitsToFontFace( traitsSymbolic);
		if (face & KFS_ITALIC)
			ioStyle->SetItalic( TRUE);
		else
			ioStyle->SetItalic( FALSE);
		if (face & KFS_BOLD)
			ioStyle->SetBold( TRUE);
		else
			ioStyle->SetBold( FALSE);
	}
	//get underline
	int32_t value = kCTUnderlineStyleNone;
	if (CTHelper::GetAttribute( attributes, kCTUnderlineStyleAttributeName, value)
	    &&
		value != kCTUnderlineStyleNone)
		ioStyle->SetUnderline( TRUE);
	else
		ioStyle->SetUnderline( FALSE);
	
	
	//get strikeout
	CFStringRef propstrikeout = CFSTR("NSStrikethrough");
	value = kCTUnderlineStyleNone;
	if (CTHelper::GetAttribute( attributes, propstrikeout, value)
	    &&
		value != kCTUnderlineStyleNone)
		ioStyle->SetStrikeout( TRUE);
	else
		ioStyle->SetStrikeout( FALSE);
	
	//get forecolor
	VColor color;
	bool hasForeColor = false;
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
		hasForeColor = CTHelper::GetAttribute( attributes, (CFStringRef)NSForegroundColorAttributeName, color);
	else
#endif		
		hasForeColor = CTHelper::GetAttribute( attributes, kCTForegroundColorAttributeName, color);
	if (hasForeColor)
	{
		ioStyle->SetHasForeColor(true);
		ioStyle->SetColor(color.GetRGBAColor(false));
	}
	
	//get alignment
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		//CTParagraphStyle & NSParagraphStyle are not toll-free bridged
		
		NSDictionary *nsDict = (NSDictionary *)attributes;
	
		NSParagraphStyle *paraStyle = (NSParagraphStyle *)[nsDict objectForKey:NSParagraphStyleAttributeName];
		if (paraStyle)
		{
			CTTextAlignment textAlign = (CTTextAlignment)[paraStyle alignment];
			if (textAlign == kCTLeftTextAlignment)
				ioStyle->SetJustification(JST_Left);
			else if (textAlign == kCTRightTextAlignment)
				ioStyle->SetJustification(JST_Right);
			else if (textAlign == kCTCenterTextAlignment)
				ioStyle->SetJustification(JST_Center);
			else if (textAlign == kCTJustifiedTextAlignment)
				ioStyle->SetJustification(JST_Justify);
		}
	}
	else
#endif
	{
		CTParagraphStyleRef paraStyle = (CTParagraphStyleRef)CFDictionaryGetValue ( attributes, kCTParagraphStyleAttributeName);
		if (paraStyle)
		{
			CTTextAlignment textAlign = kCTNaturalTextAlignment;
			bool ok = CTParagraphStyleGetValueForSpecifier(paraStyle, kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &textAlign);
			if (ok)
			{
				if (textAlign == kCTLeftTextAlignment)
					ioStyle->SetJustification(JST_Left);
				else if (textAlign == kCTRightTextAlignment)
					ioStyle->SetJustification(JST_Right);
				else if (textAlign == kCTCenterTextAlignment)
					ioStyle->SetJustification(JST_Center);
				else if (textAlign == kCTJustifiedTextAlignment)
					ioStyle->SetJustification(JST_Justify);
			}
		}
	}
	
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
		[innerPool release]; //here temp Obj-C references are released (to avoid leaks between Obj-C && C++)
#endif
	
	rangeEnd = (range.location+range.length >= rangeEnd ? rangeEnd : range.location+range.length);
	ioStyle->SetRange(rangeStart, rangeEnd);
	
	return rangeEnd;
}


Boolean XMacStyledTextBox::DoInitialize()
{
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		NSMutableAttributedString *nsAttString = [[NSMutableAttributedString alloc] init];
		fAttributedString = (CFMutableAttributedStringRef)nsAttString;
	}
	else
#endif
		fAttributedString = CFAttributedStringCreateMutable ( kCFAllocatorDefault, 0);
	fCurLayoutIsDirty = true;
	return fAttributedString != NULL;
}


void XMacStyledTextBox::DoRelease()
{
	if (fCurLayoutFrame)
	{
		CFRelease(fCurLayoutFrame);
		fCurLayoutFrame = NULL;
	}
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
        NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
        if (nsAttString)
            [nsAttString release];
    }
#endif
    else if (fAttributedString)
        CFRelease(fAttributedString);
    fAttributedString = NULL;
#if CT_USE_COCOA	
	if (fUseNSAttributes)
		CTHelper::DisableCocoa();
    fUseNSAttributes = false;
#endif
}

void XMacStyledTextBox::_ComputeCurFrame2(GReal inMaxWidth, GReal inMaxHeight, bool inForceComputeStrikeoutLines)
{
    if (inMaxHeight != 100000.0f)
    {
        inMaxHeight += 100; //workaround to avoid CoreText to crop last line if max height is too small: as text is clipped while drawing, it is ok
                            //(the only drawback is that CoreText might format more text that is actually visible but it is better than to have invisible lines...)
        fCurLayoutOffset.SetPosTo(0, -100);
	}
    if (fCurLayoutFrame)
	{
		CFRelease(fCurLayoutFrame);
		fCurLayoutFrame = NULL;
	}

	//create frame setter
    GReal baselineOffset = 0;
    
	CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString(fAttributedString);	
	if(frameSetter)
	{
		//create path from input bounds
		CGMutablePathRef path = CGPathCreateMutable();
		CGRect bounds = CGRectMake(0.0f, 0.0f, inMaxWidth, inMaxHeight);
		CGPathAddRect(path, NULL, bounds);
	
		//create frame from frame setter & path
		fCurLayoutFrame = CTFramesetterCreateFrame(frameSetter, CFRangeMake(0, 0), path, NULL);
		CFRelease(path);
		CFRelease(frameSetter);
		if (!fCurLayoutFrame)
		{
			fCurTypoWidth = 0; 
			fCurTypoHeight = 0;
			fCurLayoutIsDirty = true;
			return;
		}

		//determine typo width & height (not always equal to layout width & height)
		CGPathRef framePath = CTFrameGetPath(fCurLayoutFrame);
		CGRect frameRect = CGPathGetBoundingBox(framePath);
		
		CFArrayRef lines = CTFrameGetLines(fCurLayoutFrame);
		CFIndex numLines = CFArrayGetCount(lines);
		fNumLine = (sLONG)numLines;
		
		CGFloat maxWidth = 0;
		CGFloat maxHeight = 0;
		
		CFIndex lastLineIndex = numLines - 1;
		for(CFIndex i = 0; i < numLines; i++)
		{
			CGFloat ascent, descent, leading, width;
			CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
			width = (CGFloat) CTLineGetTypographicBounds(line, &ascent,  &descent, &leading);
			
			if(width > maxWidth)
				maxWidth = width;
			
            if (i == 0 && fIsMonostyle && (fCurLayoutMode & TRM_LEGACY_MONOSTYLE))
            {
                //apply first line baseline decal based on font actual metrics (because CoreText seems to not determine baseline using font metrics ???)
				CGPoint firstLineOrigin;
				CTFrameGetLineOrigins(fCurLayoutFrame, CFRangeMake(0, 1), &firstLineOrigin);
                GReal curLineAscent = CGRectGetHeight(frameRect)-firstLineOrigin.y;
                VFontMetrics metrics (fCurFont, NULL);
                baselineOffset = curLineAscent-floor(metrics.GetAscent()+0.5);
                if (baselineOffset < 0)
                    baselineOffset = 0;
            }
			if(i == lastLineIndex)
			{
				//frame height = distance from the bottom of the last line to the top of the frame
				CGPoint lastLineOrigin;
				CTFrameGetLineOrigins(fCurLayoutFrame, CFRangeMake(lastLineIndex, 1), &lastLineOrigin);
				
				maxHeight =  CGRectGetHeight(frameRect) - lastLineOrigin.y + descent - baselineOffset; //add descent because line origin is on the baseline
                if (maxHeight < 0)
                    maxHeight = 0;
			}
		}

        fCurLayoutOffset.SetPosBy(0,baselineOffset);
        
		//store typo bounds & adjust for discrepancies
		fCurTypoWidth = std::ceil(maxWidth);
        //JQ 05/09/2014: fixed ACI0089328
        //(actually typo width includes trailing spaces & but text without trailing spaces never overflow the constrained width)
		if (fCurTypoWidth > fCurLayoutWidth)
            fCurTypoWidth = fCurLayoutWidth;
		fCurTypoHeight = std::ceil(maxHeight)+1; //FIXME: i guess ceil should be enough (no need to add 1) - need to make some tests to check it
		fCurLayoutIsDirty = fCurLayoutFrame == NULL;
		
		//compute strikeout lines 
		//(we need to draw strikeout style lines because CoreText native impl does not support strikeout style 
		//  - only Cocoa NSAttributedString & NSContext has support for it)
#if CT_USE_COCOA	
		bool doComputeStrikeoutLines = !CTHelper::UseCocoaAttributes() && fCurLayoutFrame;
#else
		bool doComputeStrikeoutLines = fCurLayoutFrame;
#endif		
		fCurLayoutStrikeoutLines.clear();
        bool widthIsValid = true;
		//bool widthIsValid = (fCurLayoutMode & TLM_DONT_WRAP) || inForceComputeStrikeoutLines || inMaxWidth <= 8.0f || fCurTypoWidth <= fCurLayoutWidth;
		if (widthIsValid && doComputeStrikeoutLines)
		{
			bool isFirstColor = true;
			VColor colorPrev;
			
			bool gotStart = false;
			bool gotEnd = false;
			
			sLONG inStart = 0;
			sLONG inEnd = CFAttributedStringGetLength(fAttributedString);
			
			if (numLines)
			{
				CFStringRef text = CFAttributedStringGetString(fAttributedString);
				
				//get line origins
				CGPoint lineOrigin[numLines];
				CTFrameGetLineOrigins(fCurLayoutFrame, CFRangeMake(0, 0), lineOrigin); //line origin is aligned on line baseline
				
				for(CFIndex i = 0; i < numLines; i++) //lines or ordered from the top line to the bottom line
				{
					CTLineRef line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
					CFRange lineRange = CTLineGetStringRange(line);
					
					bool isStartLine = false;
					bool isEndLine = false;
					
					//is it start line ?
					if (!gotStart)
					{
						if (i == numLines-1 
							&&
							(inStart >= lineRange.location && inStart <= lineRange.location+lineRange.length)
							)
							gotStart = true;
						else if (inStart >= lineRange.location && inStart < lineRange.location+lineRange.length)
							gotStart = true;
						else if (inStart < lineRange.location)
						{
							if (i > 0)
							{
								i--;
								line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
								lineRange = CTLineGetStringRange(line);
							}
							gotStart = true;
						}
						
						if (gotStart)
						{
							if (inStart < lineRange.location)
							{
								inStart = lineRange.location;
							} else if (inStart > lineRange.location+lineRange.length)
							{
								inStart = lineRange.location+lineRange.length;
							}
							
							isStartLine = true;
						}
					}
					
					//is it end line ?
					if (i == numLines-1 
						&&
						(inEnd >= lineRange.location && inEnd <= lineRange.location+lineRange.length)
						)
						gotEnd = true;
					else if (inEnd >= lineRange.location && inEnd < lineRange.location+lineRange.length)
						gotEnd = true;
					else if (inEnd < lineRange.location)
					{
						if (i > 0)
						{
							i--;
							line = (CTLineRef) CFArrayGetValueAtIndex(lines, i);
							lineRange = CTLineGetStringRange(line);
						}
						gotEnd = true;
					}
					
					if (gotEnd)
					{
						if (inEnd < lineRange.location)
						{
							inEnd = lineRange.location;
						} else if (inEnd > lineRange.location+lineRange.length)
						{
							inEnd = lineRange.location+lineRange.length;
						}
						
						isEndLine = true;
					}
					
					if (gotStart && lineRange.length > 0)
					{
						sLONG lineEnd = lineRange.location+lineRange.length;
						UniChar c = CFStringGetCharacterAtIndex(text, lineEnd-1);
						if (c == 13 || c == 10)
							lineEnd--; //do not draw style for end of line character
						
						if (lineEnd > lineRange.location)
						{
							CFArrayRef runs = CTLineGetGlyphRuns(line);
							CFIndex numRuns = CFArrayGetCount(runs);
							
							//we need to parse runs because ascent metrics might not be the same & so strike-out y coordinate
							for (CFIndex j = 0; j < numRuns; j++)
							{
								CTRunRef run = (CTRunRef) CFArrayGetValueAtIndex(runs, j);
								
								CFRange runRange = CTRunGetStringRange(run);
								CFIndex runStart = runRange.location;
								CFIndex runEnd = runRange.location+runRange.length;
								if (runEnd > lineEnd)
									runEnd = lineEnd;
								if (runStart >= runEnd || runStart >= inEnd || runEnd <= inStart)
									continue;
								if (runStart < inStart)
									runStart = inStart;
								if (runEnd > inEnd)
									runEnd = inEnd;
								
								CFDictionaryRef runAttrs = CTRunGetAttributes(run);
								int32_t value = kCTUnderlineStyleNone;
								if (CTHelper::GetAttribute( runAttrs, CFSTR("NSStrikethrough"), value)
									&&
									value != kCTUnderlineStyleNone)
								{
									CGFloat ascent = 0;
									GReal x = lineOrigin[i].x + CTLineGetOffsetForStringIndex( line, runStart, NULL);
									GReal width = CTRunGetTypographicBounds(run,
																			CFRangeMake(0, runEnd-runStart),
																			&ascent,
																			NULL, NULL);
									GReal y = lineOrigin[i].y + ascent*0.25;
									
									
									VColor color(0,0,0);
									CTHelper::GetAttribute( runAttrs, kCTForegroundColorAttributeName, color);
									
									CTFontRef font = (CTFontRef)CFDictionaryGetValue( runAttrs, kCTFontAttributeName);
									GReal fontSize = 12;
									if (font)
										fontSize = CTFontGetSize(font);
									
									StrikeOutLine strikeOutLine;
									strikeOutLine.fStart = VPoint(x,y);
									strikeOutLine.fEnd = VPoint(x+width,y);
									strikeOutLine.fSetColor = isFirstColor || (!(color == colorPrev));
									if (strikeOutLine.fSetColor)
										strikeOutLine.fColor = color;
									strikeOutLine.fWidth = fontSize/12.0f;
									isFirstColor = false;
									colorPrev = color;
									
									fCurLayoutStrikeoutLines.push_back(strikeOutLine);
								}
							}
						}
					}
					if (gotEnd)
						break;
				}
			}
		}
	}
	else 
	{
		//store typo bounds & adjust for discrepancies
		fCurTypoWidth = 0; 
		fCurTypoHeight = 0;
		fCurLayoutIsDirty = true;
	}
}

void XMacStyledTextBox::_ComputeCurFrame(GReal inMaxWidth, GReal inMaxHeight)
{
	if (inMaxWidth != fCurLayoutWidth)
	{
		fCurLayoutWidth = inMaxWidth;
		fCurLayoutIsDirty = true;
	}
	if (inMaxHeight != fCurLayoutHeight)
	{
		fCurLayoutHeight = inMaxHeight;
		fCurLayoutIsDirty = true;
	}

	if (!fAttributedString)
	{
		if (fCurLayoutFrame)
		{
			CFRelease(fCurLayoutFrame);
			fCurLayoutFrame = NULL;
		}
		fCurLayoutIsDirty = true;
		return;
	}

	if (!fCurLayoutIsDirty && fCurLayoutFrame)
		return;

    fCurLayoutOffset.SetPosTo(0, 0);
    _ComputeCurFrame2( fCurLayoutWidth, fCurLayoutHeight);
}

/** return adjusted layout origin 
 @remarks
 CoreText layout size might not be the same as the design layout size (due to the workaround for CoreText max width bug)
 so caller should call this method while requesting for instance for character position
 and adjust character position as determined from the CoreText layout with this offset
 (actually only Quartz graphic context metrics method might need to call this method for consistency with gc user space) 
 */
const VPoint& XMacStyledTextBox::GetLayoutOffset() const
{
	return fCurLayoutOffset;
}

Boolean XMacStyledTextBox::DoDraw(const VRect& inBounds)
{
	_ComputeCurFrame( inBounds.GetWidth(), inBounds.GetHeight());

	if (fCurLayoutFrame)
	{
		//translate frame according to text box origin (frame is already computed according to inBounds width & height)
		//(note: we do not save/restore context here because it is caller responsibility to do that while using VStyledTextBox::Draw)
		
        CGContextSaveGState(fContextRef);
        CGContextClipToRect(fContextRef, inBounds);

		CGAffineTransform trans = { 1, 0, 0, 1, inBounds.GetX()+fCurLayoutOffset.GetX(), inBounds.GetY()+fCurLayoutOffset.GetY() };
		CGContextConcatCTM( fContextRef, trans);

        CGContextSetTextMatrix( fContextRef, CGAffineTransformIdentity);
        CGContextSetTextPosition(fContextRef, 0, 0);
		
        CGContextSaveGState(fContextRef);
		CTFrameDraw( fCurLayoutFrame, fContextRef);
		CGContextRestoreGState(fContextRef);
		
		//custom draw for strikeout style
		if (!fCurLayoutStrikeoutLines.empty())
		{
			CGContextSetLineCap(fContextRef, kCGLineCapSquare);
			CGContextSetLineDash(fContextRef, 0, NULL, 0);
			
			VectorOfStrikeOutLine::const_iterator it = fCurLayoutStrikeoutLines.begin();
			for(;it != fCurLayoutStrikeoutLines.end(); it++)
			{
				if (it->fSetColor)
				{
					CGColorRef color = it->fColor.MAC_CreateCGColorRef();
					CGContextSetStrokeColorWithColor(fContextRef, color);
					CGColorRelease(color);
				}
				CGContextSetLineWidth(fContextRef, it->fWidth);
				
				CGContextBeginPath(fContextRef);
				CGContextMoveToPoint(fContextRef, it->fStart.GetX(), it->fStart.GetY());
				CGContextAddLineToPoint(fContextRef, it->fEnd.GetX(), it->fEnd.GetY());
				CGContextStrokePath(fContextRef);
			}
		}
        CGContextRestoreGState(fContextRef);
		
		return true;
	}	
	return false;
}


void XMacStyledTextBox::DoGetSize(GReal &outWidth, GReal &outHeight,bool inForPrinting)
{
	GReal maxHeight = outHeight ? outHeight : 100000.0f; //if height is 0, max height is assumed to be infinite
	_ComputeCurFrame( outWidth, maxHeight);
	outWidth = fCurTypoWidth;
	outHeight = fCurTypoHeight;
}



void XMacStyledTextBox::_SetFont(VFont *font, const VColor &textColor)
{
    CopyRefCountable(&fCurFont, font);
    
	CTFontRef fontref = font->GetFontRef();
	CFRange range = CFRangeMake(0, CFAttributedStringGetLength(fAttributedString));

	CFAttributedStringSetAttribute (fAttributedString,range,kCTFontAttributeName,fontref);
	
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		//NSColor & CGColor are not toll-free bridged so use NSColor here
		
		NSColor *color = (NSColor *)CTHelper::CreateNSColor(textColor);
		NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
		NSRange nsRange = { static_cast<NSUInteger>(range.location), static_cast<NSUInteger>(range.length) };
		[nsAttString addAttribute:(NSString *)NSForegroundColorAttributeName value:color range:nsRange];
		[color release];
	}
	else
#endif
	{
		CGFloat redcomponent = (Real)(textColor.GetRed())/255.0;
		CGFloat greencomponent = (Real)(textColor.GetGreen())/255.0;
		CGFloat bluecomponent = (Real)(textColor.GetBlue())/255.0;
		CGFloat colors[4];
		CGColorSpaceRef colorspace;
		
		colors[0] = redcomponent;
		colors[1] = greencomponent;
		colors[2] = bluecomponent;
		colors[3] = 1.0;
		colorspace = CGColorSpaceCreateDeviceRGB();
		CGColorRef vforecolor = CGColorCreate(colorspace, colors);
		
		CFAttributedStringSetAttribute (fAttributedString,range,kCTForegroundColorAttributeName,vforecolor);
		
		CGColorSpaceRelease(colorspace);
		CFRelease(vforecolor);
	}

	fCurLayoutIsDirty = true;
}


void XMacStyledTextBox::_SetText(const VString& inText)
{
	CFStringRef cfString = inText.MAC_RetainCFStringCopy();
	if (cfString != NULL)
	{
		CFAttributedStringReplaceString(fAttributedString, CFRangeMake(0, 0) , cfString);
	
		CFRelease( cfString);
		fCurLayoutIsDirty = true;
	}
}


/** insert text at the specified position */
void XMacStyledTextBox::InsertPlainText( sLONG inPos, const VString& inText)
{
	CFIndex length = CFAttributedStringGetLength(fAttributedString);
	if (inPos > length)
		inPos = length;
	CFStringRef cfText = inText.MAC_RetainCFStringCopy();
	CFAttributedStringReplaceString(fAttributedString, CFRangeMake(inPos, 0), cfText);
	CFRelease(cfText);
	
	fCurLayoutIsDirty = true;
}

           
/** delete text range */
void XMacStyledTextBox::DeletePlainText( sLONG rangeStart, sLONG rangeEnd)
{
	CFIndex length = CFAttributedStringGetLength(fAttributedString);
	if (rangeEnd == -1)
		rangeEnd = length;
	if (rangeStart > length)
		rangeStart = length;
	if (rangeEnd > length)
		rangeEnd = length;
	if (rangeStart >= rangeEnd)
		return;
	
	CFAttributedStringReplaceString(fAttributedString, CFRangeMake(rangeStart, rangeEnd-rangeStart), CFSTR(""));
	
	fCurLayoutIsDirty = true;
}


/** replace text range */
void XMacStyledTextBox::ReplacePlainText( sLONG rangeStart, sLONG rangeEnd, const VString& inText)
{
	CFIndex length = CFAttributedStringGetLength(fAttributedString);
	if (rangeEnd == -1)
		rangeEnd = length;

	if (rangeStart > length)
		rangeStart = length;
	if (rangeEnd > length)
		rangeEnd = length;

	CFStringRef cfText = inText.MAC_RetainCFStringCopy();
	CFAttributedStringReplaceString(fAttributedString, CFRangeMake(rangeStart, rangeEnd-rangeStart), cfText);
	CFRelease(cfText);
	
	fCurLayoutIsDirty = true;
}


/** apply style (use style range) */
void XMacStyledTextBox::ApplyStyle( const VTextStyle* inStyle)
{
	DoApplyStyle(inStyle, NULL);
}


bool XMacStyledTextBox::CFDictionaryFromVTextStyle(CFDictionaryRef inActualAttributes, CFMutableDictionaryRef inNewAttributes, const VTextStyle* inStyle, VTextStyle *inStyleInherit)
{
	if (!inStyle)
	{
		return false;
	}
	
	bool needsUpdate = false;
	bool fontNeedsUpdate = false;
	bool fontFamilyHasChanged = false;
	
	//test tokens Cocoa & CoreText
	/*
	//not same (!)
	CFShow(kCTForegroundColorAttributeName);
	CFShow((CFStringRef)NSForegroundColorAttributeName);
	
	//both are same 
	CFShow(kCTFontAttributeName);
	CFShow((CFStringRef)NSFontAttributeName);

	//both are same
	CFShow(kCTUnderlineStyleAttributeName);
	CFShow((CFStringRef)NSUnderlineStyleAttributeName);
	*/
	
	CTFontRef fontRef = (CTFontRef)CFDictionaryGetValue(inActualAttributes, kCTFontAttributeName);
	CFStringRef cfName = fontRef ? CTFontCopyFamilyName(fontRef) : NULL;
	VString fontFamilyName;
	if (cfName)
	{
		fontFamilyName.MAC_FromCFString(cfName);
		CFRelease(cfName);
	}
	
	//get actual font traits
	CGFloat fontSize = 12.0f;
	CTFontSymbolicTraits fontFaceValue = 0;
	CTFontSymbolicTraits fontFaceMask = 0; 
	if (fontRef)
	{
		CFDictionaryRef baseTraitsAtt = CTFontCopyTraits(fontRef);
		fontSize = CTFontGetSize(fontRef);
		xbox_assert(baseTraitsAtt);
		CTHelper::GetAttribute( baseTraitsAtt, kCTFontSymbolicTrait, fontFaceValue);
		fontFaceValue &= ~kCTFontClassMaskTrait; //do not take account class
		fontFaceMask = 0; 
		CFRelease(baseTraitsAtt);
	}
	
	//update with new style
	
	sLONG isbold = inStyle->GetBold();
	if (isbold != -1)
	{
		fontNeedsUpdate = true;
		fontFaceMask |= kCTFontBoldTrait;
		if (isbold != 0)
		{
			fontFaceValue |= kCTFontBoldTrait;
		}
		else
		{
			fontFaceValue &= ~kCTFontBoldTrait;
		}
	}
	else if (inStyleInherit)
	{
		//as some fonts cannot support italic or bold style, 
		//we get current italic or bold status from the inherited style but from the current font traits
		
		isbold = inStyleInherit->GetBold();
		if (isbold != -1)
		{
			fontFaceMask |= kCTFontBoldTrait;
			if (isbold != 0)
			{
				fontFaceValue |= kCTFontBoldTrait;
			}
			else
			{
				fontFaceValue &= ~kCTFontBoldTrait;
			}
		}
	}

	
	sLONG isitalic = inStyle->GetItalic();
	if (isitalic != -1)
	{
		fontNeedsUpdate = true;
		fontFaceMask |= kCTFontItalicTrait;
		if (isitalic != 0)
		{
			fontFaceValue |= kCTFontItalicTrait;
		}
		else
		{
			fontFaceValue &= ~kCTFontItalicTrait;
		}
	}
	else if (inStyleInherit)
	{
		//as some fonts cannot support italic or bold style, 
		//we get current italic or bold status from the inherited style
		
		isitalic = inStyleInherit->GetItalic();
		if (isitalic != -1)
		{
			fontFaceMask |= kCTFontItalicTrait;
			if (isitalic != 0)
			{
				fontFaceValue |= kCTFontItalicTrait;
			}
			else
			{
				fontFaceValue &= ~kCTFontItalicTrait;
			}
		}
	}
	
	sLONG isUnderline = inStyle->GetUnderline();
	if(isUnderline !=(-1))
	{
		needsUpdate = true;
		CTUnderlineStyle underLineStyle;
		underLineStyle = isUnderline != 0 ? kCTUnderlineStyleSingle :  kCTUnderlineStyleNone;
		CFNumberRef underlineref = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &underLineStyle);
		CFStringRef propunderline = kCTUnderlineStyleAttributeName;
		CFDictionarySetValue (inNewAttributes,kCTUnderlineStyleAttributeName,underlineref);
	}
	
	sLONG isStrikeOut = inStyle->GetStrikeout();
	if(isStrikeOut !=(-1))
	{
		needsUpdate = true;
		CTUnderlineStyle strikeOutStyle;
		strikeOutStyle = isStrikeOut != 0 ? kCTUnderlineStyleSingle :  kCTUnderlineStyleNone;
		CFNumberRef strikeoutref = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &strikeOutStyle);
		CFStringRef propstrikeout = CFSTR("NSStrikethrough");
		
		CFDictionarySetValue (inNewAttributes,propstrikeout,strikeoutref);
		
		CFRelease(propstrikeout);
	}
	
	VString fontname = inStyle->GetFontName();
	if (fontname.GetLength() > 0)
	{
		fontNeedsUpdate = true;
		fontFamilyHasChanged = true;
		fontFamilyName.FromString(fontname);
	}
	
	GReal fontsize = inStyle->GetFontSize();
	if (fontsize != -1)
	{
		fontNeedsUpdate = true;
		fontSize = fontsize*fFontSizeToDocumentFontSize;
	}
	else if (inStyleInherit)
	{
		fontsize = inStyleInherit->GetFontSize();
		if (fontsize != -1)
			fontSize = fontsize*fFontSizeToDocumentFontSize;
	}
	
	/* //background color is not supported by CoreText
	bool istransparent = inStyle->GetTransparent();
	if (!istransparent)
	{
		needsUpdate = true;
	}
	*/
	
	bool hasForeColor = inStyle->GetHasForeColor();
	if(hasForeColor)
	{
		needsUpdate = true;
		XBOX::VColor forecolor;
		forecolor.FromRGBAColor(inStyle->GetColor());
#if CT_USE_COCOA
		if (CTHelper::UseCocoaAttributes())
			CTHelper::SetAttribute(inNewAttributes, (CFStringRef)NSForegroundColorAttributeName, forecolor);
		else
#endif			
			CTHelper::SetAttribute(inNewAttributes, kCTForegroundColorAttributeName, forecolor);
	}
	if (fontNeedsUpdate && !fontFamilyName.IsEmpty())
	{ 
		needsUpdate = true;
		//try first to derive from current font ref
		CTFontRef newFont = fontFamilyHasChanged ? NULL : CTFontCreateCopyWithSymbolicTraits(fontRef, fontSize, NULL, fontFaceValue, fontFaceMask);
		if (newFont == NULL)
		{
			//backfall to font with current font base family name
			VFont* font = VFont::RetainFont(fontFamilyName, XMacFont::CTFontSymbolicTraitsToFontFace(fontFaceValue), fontSize);
			newFont = font->GetImpl().RetainCTFontRef();
			font->Release();
			xbox_assert(newFont);
		}
		
		CFDictionarySetValue(inNewAttributes, kCTFontAttributeName, newFont);
		CFRelease(newFont);
	}
	
	return needsUpdate;
}


/** set paragraph line height
 
 if positive, it is fixed line height in point
 if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
 if -1, it is normal line height
 */
void XMacStyledTextBox::SetLineHeight( const GReal inLineHeight, sLONG inStart, sLONG inEnd)
{
	//prevent crash: CoreText might crash if we pass invalid start or end indexs (nasty bug indeed...)
	CFIndex textLength = CFAttributedStringGetLength(fAttributedString);
    if (inEnd > textLength)
		inEnd = textLength;
	if (inStart > textLength)
		inStart = textLength;
    if (inEnd < 0)
        inEnd = textLength;
    
    CGFloat lineHeight = inLineHeight;
    CGFloat lineHeightMultiple = 0.0;
    if (lineHeight < 0)
    {
        lineHeight = 0; //default for min/max line height = normal line height;
        lineHeightMultiple = -inLineHeight;
    }
    
    sLONG rangeStart = inStart;
    while (rangeStart < inEnd)
    {
        CFRange range = { rangeStart, inEnd-rangeStart };
        CFRange rangeOut = { rangeStart, inEnd-rangeStart };
        CTParagraphStyleRef vparagraph = NULL;
       
        CTParagraphStyleRef paraActual = (CTParagraphStyleRef)CFAttributedStringGetAttributeAndLongestEffectiveRange( fAttributedString, range.location, kCTParagraphStyleAttributeName, range, &rangeOut);
        
        if (paraActual)
        {
            xbox_assert(range.location == rangeOut.location);
            range.length = rangeOut.length;
            
            //merge with actual para settings
            
            CTParagraphStyleSetting settings[6];
            int count = 0;
            CTParagraphStyleSetting *setting = &(settings[0]);
            
            //set fixed line height
            setting->spec = kCTParagraphStyleSpecifierMaximumLineHeight;
            setting->value = &lineHeight;
            setting->valueSize = sizeof(CGFloat);
            count++;setting++;
            setting->spec = kCTParagraphStyleSpecifierMinimumLineHeight;
            setting->value = &lineHeight;
            setting->valueSize = sizeof(CGFloat);
            count++;setting++;
            setting->spec = kCTParagraphStyleSpecifierLineHeightMultiple;
            setting->value = &lineHeightMultiple;
            setting->valueSize = sizeof(CGFloat);
            count++;setting++;
           
            //inherit other settings
            
            CTTextAlignment textAlign = kCTNaturalTextAlignment;
            CTLineBreakMode lineBreakMode = kCTLineBreakByWordWrapping; //by default, break line on word
            CTWritingDirection writingDirection = kCTWritingDirectionNatural; //by default, use bidirectional default algorithm
            
            if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &textAlign))
            {   //set new alignment
                setting->spec = kCTParagraphStyleSpecifierAlignment;
                setting->value = &textAlign;
                setting->valueSize = sizeof(CTTextAlignment);
                count++;setting++;
            }
            if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierLineBreakMode, sizeof(CTLineBreakMode), &lineBreakMode))
            {
                setting->spec = kCTParagraphStyleSpecifierLineBreakMode;
                setting->value = &lineBreakMode;
                setting->valueSize = sizeof(CTLineBreakMode);
                count++;setting++;
            }
            if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierBaseWritingDirection, sizeof(CTWritingDirection), &writingDirection))
            {
                setting->spec = kCTParagraphStyleSpecifierBaseWritingDirection;
                setting->value = &writingDirection;
                setting->valueSize = sizeof(CTWritingDirection);
                count++;setting++;
            }
            xbox_assert(count <= 6);
            vparagraph = CTParagraphStyleCreate(&(settings[0]), count);
        }
        else
        {
            //init para settings from scratch
            CTParagraphStyleSetting settings[3];
            CTParagraphStyleSetting *setting = &(settings[0]);
            
            //set fixed line height
            {
                setting->spec = kCTParagraphStyleSpecifierMaximumLineHeight;
                setting->value = &lineHeight;
                setting->valueSize = sizeof(CGFloat);
                setting++;
                setting->spec = kCTParagraphStyleSpecifierMinimumLineHeight;
                setting->value = &lineHeight;
                setting->valueSize = sizeof(CGFloat);
                setting++;
                setting->spec = kCTParagraphStyleSpecifierLineHeightMultiple;
                setting->value = &lineHeightMultiple;
                setting->valueSize = sizeof(CGFloat);
             }
            
            vparagraph = CTParagraphStyleCreate(&(settings[0]), 2);
        }
        CFAttributedStringSetAttribute (fAttributedString,range,kCTParagraphStyleAttributeName,vparagraph);
        CFRelease(vparagraph);
        
        rangeStart = range.location+range.length;
    }
    
	fCurLayoutIsDirty = true;
}


void XMacStyledTextBox::DoApplyStyle(const VTextStyle* inStyle, VTextStyle *inStyleInherit)
{
	Real fontsize;
	XBOX::VColor forecolor, backcolor;
	XBOX::VString fontname;
	sLONG begin, end;
	sLONG curbegin, curend;
	inStyle->GetRange(begin, end);

	//prevent crash: CoreText might crash if we pass invalid start or end indexs (nasty bug indeed...)
	CFIndex textLength = CFAttributedStringGetLength(fAttributedString);
	if (end > textLength)
		end = textLength;
	if (begin > textLength)
		begin = textLength;
		
	if (end <= begin)
		return;
	
	
#if CT_USE_COCOA
	NSAutoreleasePool *innerPool = NULL; //local autorelease pool
	NSMutableAttributedString *nsAttString = (NSMutableAttributedString *)fAttributedString;
	if (CTHelper::UseCocoaAttributes())
	{
		innerPool = [[NSAutoreleasePool alloc] init]; //local autorelease pool
		[nsAttString beginEditing];
	}
	else 
#endif
		CFAttributedStringBeginEditing(fAttributedString);		
	
	eStyleJust justisset = inStyle->GetJustification();
	if (justisset != JST_Notset && begin <= 0 && end >= textLength)
	{
		fCurJustUniform = justisset;
		
		if(justisset != JST_Notset)
		{
#if CT_USE_COCOA
			if (CTHelper::UseCocoaAttributes())
			{
				//CTParagraphStyle & NSParagraphStyle are not toll-free bridged
				
                CTTextAlignment valign = kCTNaturalTextAlignment;
				if(justisset == JST_Left)
					valign = kCTLeftTextAlignment;
				if(justisset == JST_Right)
					valign = kCTRightTextAlignment;
				if(justisset == JST_Center)
					valign = kCTCenterTextAlignment;
				if(justisset == JST_Justify)
					valign = kCTJustifiedTextAlignment;
				if (valign != kCTNaturalTextAlignment)
				{
                    sLONG rangeBegin = begin;
                    while (rangeBegin < end)
                    {
                        NSRange nsRange = { rangeBegin, end-rangeBegin };
                        NSRange nsRangeOut = { rangeBegin, end-rangeBegin };
                        NSMutableParagraphStyle *paraStyle = NULL;
                        NSParagraphStyle *paraStyleActual = [nsAttString attribute:NSParagraphStyleAttributeName atIndex:nsRange.location longestEffectiveRange:&nsRangeOut inRange:nsRange];
                        if (paraStyleActual)
                            //merge with actual para settings
                            paraStyle = [paraStyleActual mutableCopyWithZone:NULL];
                        else
                            //init para settings from scratch
                            paraStyle = [[NSMutableParagraphStyle alloc] init];
                        
                        [paraStyle setAlignment:(NSTextAlignment)valign];
                        [nsAttString addAttribute:NSParagraphStyleAttributeName value:paraStyle range:nsRangeOut];
                        
                        [paraStyle release];
                        
                        rangeBegin = nsRangeOut.location+nsRangeOut.length;
                    }
				}
			}
			else
#endif
			{
                sLONG rangeBegin = begin;
                while (rangeBegin < end)
                {
                    CFRange range = { rangeBegin, end-rangeBegin };
                    CTParagraphStyleSetting vsetting;
                    CTTextAlignment valign = kCTNaturalTextAlignment;
                    vsetting.spec = kCTParagraphStyleSpecifierAlignment;
                    vsetting.valueSize = sizeof(CTTextAlignment);
                    vsetting.value = &valign;
                    
                    if(justisset == JST_Left)
                        valign = kCTLeftTextAlignment;
                    if(justisset == JST_Right)
                        valign = kCTRightTextAlignment;
                    if(justisset == JST_Center)
                        valign = kCTCenterTextAlignment;
                    if(justisset == JST_Justify)
                        valign = kCTJustifiedTextAlignment;
                    
                    CFRange rangeOut = { range.location, range.length };
                    CTParagraphStyleRef vparagraph = NULL;
                    CTParagraphStyleRef paraActual = (CTParagraphStyleRef)CFAttributedStringGetAttributeAndLongestEffectiveRange( fAttributedString, range.location, kCTParagraphStyleAttributeName, range, &rangeOut);
                    if (paraActual)
                    {
                        //merge with actual para settings
                        
                        CTParagraphStyleSetting settings[6];
                        int count = 0;
                        CTParagraphStyleSetting *setting = &(settings[0]);
                        
                        CGFloat lineMinHeight = (CGFloat) 0.0;
                        CGFloat lineMaxHeight = (CGFloat) 0.0;
                        CGFloat lineMultipleHeight = (CGFloat) 0.0;
                        CTLineBreakMode lineBreakMode = kCTLineBreakByWordWrapping; //by default, break line on word
                        CTWritingDirection writingDirection = kCTWritingDirectionNatural; //by default, use bidirectional default algorithm
                        
                        {   //set new alignment
                            setting->spec = kCTParagraphStyleSpecifierAlignment;
                            setting->value = &valign;
                            setting->valueSize = sizeof(CTTextAlignment);
                            count++;setting++;
                        }
                        //inherit other settings (these are set only while initializing first the attributed string according to layout mode)
                        //(CTHelper::CreateStringAttributes should remain consistent with the following settings)
                        if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierMaximumLineHeight, sizeof(CGFloat), &lineMaxHeight))
                        {
                            setting->spec = kCTParagraphStyleSpecifierMaximumLineHeight;
                            setting->value = &lineMaxHeight;
                            setting->valueSize = sizeof(CGFloat);
                            count++;setting++;
                        }
                        if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierMinimumLineHeight, sizeof(CGFloat), &lineMinHeight))
                        {
                            setting->spec = kCTParagraphStyleSpecifierMinimumLineHeight;
                            setting->value = &lineMinHeight;
                            setting->valueSize = sizeof(CGFloat);
                            count++;setting++;
                        }
                        if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierLineHeightMultiple, sizeof(CGFloat), &lineMultipleHeight))
                        {
                            setting->spec = kCTParagraphStyleSpecifierLineHeightMultiple;
                            setting->value = &lineMultipleHeight;
                            setting->valueSize = sizeof(CGFloat);
                            count++;setting++;
                        }
                        if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierLineBreakMode, sizeof(CTLineBreakMode), &lineBreakMode))
                        {
                            setting->spec = kCTParagraphStyleSpecifierLineBreakMode;
                            setting->value = &lineBreakMode;
                            setting->valueSize = sizeof(CTLineBreakMode);
                            count++;setting++;
                        }
                        if (CTParagraphStyleGetValueForSpecifier(paraActual, kCTParagraphStyleSpecifierBaseWritingDirection, sizeof(CTWritingDirection), &writingDirection))
                        {
                            setting->spec = kCTParagraphStyleSpecifierBaseWritingDirection;
                            setting->value = &writingDirection;
                            setting->valueSize = sizeof(CTWritingDirection);
                            count++;setting++;
                        }
                        xbox_assert(count <= 6);
                        vparagraph = CTParagraphStyleCreate(&(settings[0]), count);
                    }
                    else
                    {
                        //init para settings from scratch
                        rangeOut.location = range.location;
                        rangeOut.length = range.length;
                        vparagraph = CTParagraphStyleCreate(&vsetting ,1);
                    }
                    CFAttributedStringSetAttribute (fAttributedString,rangeOut,kCTParagraphStyleAttributeName,vparagraph);
                    CFRelease(vparagraph);
                    
                    rangeBegin = rangeOut.location+rangeOut.length;
                }
			}
		}
	}
	justisset = JST_Notset;
	
	curbegin = begin;
	curend = end;
	    
	CFDictionaryRef actualAttributes = NULL;
	do //iterate on uniform styles
	{
		
	//JQ 10/08/2011: we must iterate on actual uniform styles 
	//				 otherwise changing for instance only font style size would change also font family on all range...
	CFRange range = { curbegin, 0 };	
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		NSRange rangeLimit = { static_cast<NSUInteger>(curbegin), static_cast<NSUInteger>(end-curbegin) };
		NSRange rangeOut = { static_cast<NSUInteger>(curbegin), 0 };
		NSDictionary *nsDict = [nsAttString attributesAtIndex:(NSUInteger)curbegin longestEffectiveRange:(NSRangePointer)&rangeOut inRange:rangeLimit];
		range.location = rangeOut.location;
		range.length = rangeOut.length;
		actualAttributes = (CFDictionaryRef)nsDict;
	}
	else
#endif
		actualAttributes = CFAttributedStringGetAttributesAndLongestEffectiveRange( fAttributedString, curbegin, CFRangeMake(curbegin, end-curbegin), &range);
		
	curend = range.location+range.length;	
		
	CFMutableDictionaryRef newAttributes;
#if CT_USE_COCOA
	NSMutableDictionary* nsNewAttributes = NULL;
	if (CTHelper::UseCocoaAttributes())
	{
		nsNewAttributes = [[NSMutableDictionary alloc] init];
		newAttributes = (CFMutableDictionaryRef)nsNewAttributes;
	}
	else
#endif
	{
		newAttributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	}
	if (CFDictionaryFromVTextStyle(actualAttributes, newAttributes, inStyle, inStyleInherit))
	{
#if CT_USE_COCOA
		if (CTHelper::UseCocoaAttributes())
		{
			NSRange nsRange = { static_cast<NSUInteger>(range.location), static_cast<NSUInteger>(range.length) };
			[nsAttString setAttributes:(NSDictionary *)nsNewAttributes range:nsRange];
		}
		else
#endif		
			CFAttributedStringSetAttributes(fAttributedString,range,newAttributes,false);
	}
	CFRelease(newAttributes);
	
	curbegin = curend;	
	} while (curbegin < end);
	
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		[nsAttString endEditing];
		[innerPool release];
	}
	else 
#endif
		CFAttributedStringEndEditing(fAttributedString);

#if CT_USE_COCOA
    if (!CTHelper::UseCocoaAttributes())
#endif
    if (begin > 0 || end < textLength-1)
    {
        //check if text is still monostyle or not
        CFRange range = { 0, 0 };
        actualAttributes = CFAttributedStringGetAttributesAndLongestEffectiveRange( fAttributedString, 0, CFRangeMake(0, textLength), &range);
        
        if (range.location+range.length < textLength-1)
        {
            fIsMonostyle = false;
        }
    }
    
    if (fIsMonostyle && !inStyle->GetFontName().IsEmpty())
    {
        //monostyle & font name has changed -> update baseline decal
        VFont *font = VFont::RetainFont(inStyle->GetFontName(), 0, 12, 72);
        CopyRefCountable(&fCurFont, font);
        ReleaseRefCountable(&font);
    }
    
    
	fCurLayoutIsDirty = true;

	_DoApplyStyleRef( inStyle);
}


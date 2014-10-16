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
#include "VFont.h"

#if CT_USE_COCOA	
#import <Cocoa/Cocoa.h> 

/** true for some tasks like using Cocoa RTF or HTML import/export 
 @remarks
	should be FALSE (default) if attributes are used for drawing in a CGContext (otherwise many context errors)
 */
bool CTHelper::fUseCocoaAttributes = false;
VCriticalSection CTHelper::fMutex;

/** enable Cocoa support for CoreText attributes 
 @remarks
	after this method is called, CFAttributedString will be created from NSAttributedString & will store NSColor & NSParagraphStyle but CGColor & CTParagraphStyle
 
	caution: caller must call DisableCocoa before caller method exits because this method lock/unlock current thread
 */
void CTHelper::EnableCocoa()
{
	fMutex.Lock();
	fUseCocoaAttributes = true;
}


/** disable Cocoa support for CoreText attributes 
 @remarks
	after this method is called, CFAttributedString will not be created from NSAttributedString & will store CGColor & CTParagraphStyle but NSColor & NSParagraphStyle
  */
void CTHelper::DisableCocoa()
{
	fUseCocoaAttributes = false;
	fMutex.Unlock();
}
#endif



/** create and retain Core Text string attributes using default attributes from the specified font, text rendering mode and layout mode */
CFMutableDictionaryRef CTHelper::CreateStringAttributes( const VFont *inFont, const VColor& inColorFill, TextRenderingMode inMode, TextLayoutMode inLayoutMode, GReal inCharKerning)
{
	CFMutableDictionaryRef attributes = inFont->GetImpl().RetainStringAttributes();
	xbox_assert(attributes);
	if (attributes == NULL)
		return NULL;
	CFMutableDictionaryRef attributesCopy = NULL;
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
		attributesCopy = (CFMutableDictionaryRef)[[NSMutableDictionary alloc] initWithDictionary:(NSDictionary *)attributes];
	else
#endif			
		attributesCopy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, attributes);
	CFRelease(attributes);
	attributes = attributesCopy;
	
	//set foreground color
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
		CTHelper::SetAttribute(attributes, (CFStringRef)NSForegroundColorAttributeName, inColorFill);
	else
#endif			
		CTHelper::SetAttribute(attributes, kCTForegroundColorAttributeName, inColorFill);
	
	if (inMode & TRM_CONSTANT_SPACING)
	{
		//disable kerning and ligatures
		CTHelper::SetAttribute(attributes, kCTKernAttributeName, inCharKerning);
		CTHelper::SetAttribute(attributes, kCTLigatureAttributeName, (int32_t)0);
	}
	if (inLayoutMode & TLM_VERTICAL)
		//use vertical-oriented glyphs
		CTHelper::SetAttribute(attributes, kCTVerticalFormsAttributeName, true);
	else
		//use horizontal-oriented glyphs
		CTHelper::SetAttribute(attributes, kCTVerticalFormsAttributeName, false);
		
	
#if CT_USE_COCOA
	if (CTHelper::UseCocoaAttributes())
	{
		//CTParagraphStyle & NSParagraphStyle are not toll-free bridged
		//so use NSParagraphStyle here
		
		//if (inLayoutMode & (TLM_VERTICAL|TLM_PARAGRAPH_STYLE_MASK))
		{
			//build paragraph settings
			bool needUpdate = false;
			
			CTTextAlignment textAlignment = kCTNaturalTextAlignment;
			CTLineBreakMode lineBreakMode = kCTLineBreakByWordWrapping; //by default, break line on word 
			CTWritingDirection writingDirection = kCTWritingDirectionNatural; //by default, use bidirectional default algorithm 
			
			if (inLayoutMode & TLM_PARAGRAPH_STYLE_ALIGN_MASK)
			{
				//set horizontal alignment
				if (inLayoutMode & TLM_ALIGN_LEFT)
					textAlignment = kCTLeftTextAlignment;
				else if (inLayoutMode & TLM_ALIGN_RIGHT)
					textAlignment = kCTRightTextAlignment;
				else if (inLayoutMode & TLM_ALIGN_CENTER)
					textAlignment = kCTCenterTextAlignment;
				else if (inLayoutMode & TLM_ALIGN_JUSTIFY)
					textAlignment = kCTJustifiedTextAlignment;
			}
			needUpdate = textAlignment != kCTNaturalTextAlignment;
			
			if (inLayoutMode & TLM_VERTICAL)
				needUpdate = true;
			
			if (inLayoutMode & TLM_DONT_WRAP)
			{
				//set line break mode
				if (inLayoutMode & TLM_TRUNCATE_END_IF_NECESSARY)
					lineBreakMode = kCTLineBreakByTruncatingTail;
				else if (inLayoutMode & TLM_TRUNCATE_MIDDLE_IF_NECESSARY)
					lineBreakMode = kCTLineBreakByTruncatingMiddle;
				else
					lineBreakMode = kCTLineBreakByClipping;
				needUpdate = true;
			}
			
            //JQ 03/06/2013: for compliancy with other platform impl like GDI/GDIPlus & DWrite impls, on default direction is left to right if not explictly set to right to left
            //(otherwise it would lead to inconsistent paragraph layout due to base direction determined per paragraph based on default bidi algorithm which is not also Word-like or Web-like consistent)
			if (!(inLayoutMode & TLM_RIGHT_TO_LEFT))
			{
				//set base writing direction to left to right 
				writingDirection = kCTWritingDirectionLeftToRight;
				needUpdate = true;
			}
			else
			{
				//set base writing direction to right to left 
				writingDirection = kCTWritingDirectionRightToLeft;
				needUpdate = true;
			}
			
			if (needUpdate)
			{
				NSMutableDictionary *nsAttributes = (NSMutableDictionary *)attributes;
				NSMutableParagraphStyle *paraStyle = [[NSMutableParagraphStyle alloc] init];
				
				if (textAlignment != kCTNaturalTextAlignment)
					[paraStyle setAlignment:(NSTextAlignment)textAlignment];
				
				if (inLayoutMode & TLM_VERTICAL)
				{
					//Core Text is buggy in vertical mode:
					//by default, line height is wrong 
					//so as a workaround, clamp line height to font line height
					//TODO: wait for API fix from Apple
					
					GReal lineHeight = inFont->GetImpl().GetAscent()+inFont->GetImpl().GetDescent()+inFont->GetImpl().GetLeading();
					[paraStyle setMaximumLineHeight:lineHeight];
				}
				if (lineBreakMode != kCTLineBreakByWordWrapping)
					[paraStyle setLineBreakMode:(NSLineBreakMode)lineBreakMode];
				if (writingDirection != kCTWritingDirectionNatural)
					[paraStyle setBaseWritingDirection:(NSWritingDirection)writingDirection];
				
				[nsAttributes setObject:paraStyle forKey:NSParagraphStyleAttributeName];
				[paraStyle release];
			}
		}
		
	}
	else
#endif
	//if (inLayoutMode & (TLM_VERTICAL|TLM_PARAGRAPH_STYLE_MASK))
	{
		//build paragraph settings
		CGFloat lineHeight = (CGFloat) 0.0;
		CTLineBreakMode lineBreakMode = kCTLineBreakByWordWrapping; //by default, break line on word 
		CTWritingDirection writingDirection = kCTWritingDirectionNatural; //by default, use bidirectional default algorithm 
		CTTextAlignment textAlignment = kCTNaturalTextAlignment; //by default, text alignment is based on writing direction

		CTParagraphStyleSetting *paraSettings = new  CTParagraphStyleSetting[10];
		CFIndex numSetting = 0;
		CTParagraphStyleSetting *setting = paraSettings;
		if (inLayoutMode & TLM_VERTICAL)
		{
			//Core Text is buggy in vertical mode:
			//by default, line height is wrong 
			//so as a workaround, clamp line height to font line height
			//TODO: wait for API fix from Apple
			
			lineHeight = inFont->GetImpl().GetAscent()+inFont->GetImpl().GetDescent()+inFont->GetImpl().GetLeading();
			setting->spec = kCTParagraphStyleSpecifierMaximumLineHeight;
			setting->value = &lineHeight;
			setting->valueSize = sizeof(CGFloat);
			
			setting++;
			numSetting++;
		}
		if (inLayoutMode & TLM_PARAGRAPH_STYLE_ALIGN_MASK)
		{
			//set horizontal alignment
			if (inLayoutMode & TLM_ALIGN_LEFT)
				textAlignment = kCTLeftTextAlignment;
			else if (inLayoutMode & TLM_ALIGN_RIGHT)
				textAlignment = kCTRightTextAlignment;
			else if (inLayoutMode & TLM_ALIGN_CENTER)
				textAlignment = kCTCenterTextAlignment;
			else if (inLayoutMode & TLM_ALIGN_JUSTIFY)
				textAlignment = kCTJustifiedTextAlignment;
			else if (inLayoutMode & TLM_RIGHT_TO_LEFT)
				textAlignment = kCTRightTextAlignment;
			else
				textAlignment = kCTLeftTextAlignment;

			setting->spec = kCTParagraphStyleSpecifierAlignment;
			setting->value = &textAlignment;
			setting->valueSize = sizeof(CTTextAlignment);
			
			setting++;
			numSetting++;
		}
		if (inLayoutMode & TLM_DONT_WRAP)
		{
			//set line break mode
			if (inLayoutMode & TLM_TRUNCATE_END_IF_NECESSARY)
				lineBreakMode = kCTLineBreakByTruncatingTail;
			else if (inLayoutMode & TLM_TRUNCATE_MIDDLE_IF_NECESSARY)
				lineBreakMode = kCTLineBreakByTruncatingMiddle;
			else
				lineBreakMode = kCTLineBreakByClipping;
			setting->spec = kCTParagraphStyleSpecifierLineBreakMode;
			setting->value = &lineBreakMode;
			setting->valueSize = sizeof(CTLineBreakMode);
			
			setting++;
			numSetting++;
		}
        //JQ 03/06/2013: for compliancy with other platform impl like GDI/GDIPlus & DWrite impls, on default direction is left to right if not explictly set to right to left
        //(otherwise it would lead to inconsistent paragraph layout due to base direction determined per paragraph based on default bidi algorithm which is not also Word-like or Web-like consistent)
        if (!(inLayoutMode & TLM_RIGHT_TO_LEFT))
		{
			//set base writing direction to left to right 
			writingDirection = kCTWritingDirectionLeftToRight;
			setting->spec = kCTParagraphStyleSpecifierBaseWritingDirection;
			setting->value = &writingDirection;
			setting->valueSize = sizeof(CTWritingDirection);
			
			setting++;
			numSetting++;
		}
		else 
		{
			//set base writing direction to right to left 
			writingDirection = kCTWritingDirectionRightToLeft;
			setting->spec = kCTParagraphStyleSpecifierBaseWritingDirection;
			setting->value = &writingDirection;
			setting->valueSize = sizeof(CTWritingDirection);
			
			setting++;
			numSetting++;
		}
		
		xbox_assert(numSetting <= 10);
		
		if (numSetting > 0)
		{
			//set paragraph style attribute
			CTParagraphStyleRef paraStyle = CTParagraphStyleCreate( paraSettings, numSetting);
			CFDictionarySetValue(attributes, kCTParagraphStyleAttributeName, paraStyle);
			CFRelease(paraStyle);
		}
		delete [] paraSettings;
	}

	return attributes;
}

/** create and return attributed string from the specified text string and attributes */
CFAttributedStringRef CTHelper::CreateAttributedString( const VString& inText, CFDictionaryRef inAttributes, CFDictionaryRef inAttributesLatin, CTUserStyleRangeVector *inLatinStyleRanges)
{
	if (inAttributes == NULL)
		return NULL;
	
	//create attributed string from string+attributes
	CFStringRef cfString = inText.MAC_RetainCFStringCopy();
	xbox_assert(CFDictionaryGetCount( inAttributes) >= 1); //at least font attribute must be specified
	
	//create attributed string
	CFAttributedStringRef attString = NULL;
	
#if CT_USE_COCOA	
	if (CTHelper::UseCocoaAttributes())
	{
		NSAttributedString *nsAttString = [[NSAttributedString alloc] initWithString:(NSString *)cfString attributes:(NSDictionary *)inAttributes];
		attString = (CFAttributedStringRef)nsAttString;
	}
	else 
#endif
		attString = CFAttributedStringCreate( kCFAllocatorDefault, cfString, inAttributes);

	
	CFRelease( cfString);
	
	if (inAttributesLatin && inLatinStyleRanges)
	{
		//string is using vertical layout and has characters with mixed orientation:
		//replace latin characters style with halfwidth latin style for vertical layout
		
		//create attributed string
		CFMutableAttributedStringRef attStringCopy = CFAttributedStringCreateMutableCopy (kCFAllocatorDefault, 0, attString);
		CFRelease(attString);
		
		//modify latin characters style attributes
		CFAttributedStringBeginEditing( attStringCopy);
		CTUserStyleRangeVector::const_iterator it = inLatinStyleRanges->begin();
		for (; it != inLatinStyleRanges->end(); it++)
		{
			CFRange range = CFRangeMake( (*it).first, (*it).second);
			CFAttributedStringSetAttributes( attStringCopy, range, inAttributesLatin, true);
		}
		CFAttributedStringEndEditing( attStringCopy);
		
		return attStringCopy;
	}
	else
		return attString;
}


/** create Core Text line from the specified text string and string attributes */
CTLineRef CTHelper::CreateLine(const VString& inText, CFDictionaryRef inAttributes, CFDictionaryRef inAttributesLatin, CTUserStyleRangeVector *inLatinStyleRanges)
{
	if (inAttributes == NULL)
		return NULL;
	
	//create attributed string
	CFAttributedStringRef attString = CTHelper::CreateAttributedString( inText, inAttributes, inAttributesLatin, inLatinStyleRanges);
	if (attString)
	{
		//create Core text line from attributed string
		CTLineRef line = CTLineCreateWithAttributedString(attString);
		CFRelease(attString);
		return line;
	}
	return NULL;
}

/** create Core Text frame from the specified text string, string attributes and frame rect */
CTFrameRef CTHelper::CreateFrame(const VString& inText, CFDictionaryRef inAttributes, const VRect& inBounds, CFDictionaryRef inAttributesLatin, CTUserStyleRangeVector *inLatinStyleRanges)
{
	if (inAttributes == NULL)
		return NULL;
	
	//create attributed string from string+attributes
	CFAttributedStringRef attString = CTHelper::CreateAttributedString( inText, inAttributes, inAttributesLatin, inLatinStyleRanges);
	if (attString == NULL)
		return NULL;
	
	//create frame setter
	CTFramesetterRef frameSetter = CTFramesetterCreateWithAttributedString(attString);	
	CFRelease(attString);

	//create path from input bounds
	CGMutablePathRef path = CGPathCreateMutable();
	CGRect bounds = CGRectMake(inBounds.GetX(), inBounds.GetY(), inBounds.GetWidth(), inBounds.GetHeight());
	CGPathAddRect(path, NULL, bounds);
	
	//create frame from frame setter
	CTFrameRef frame = CTFramesetterCreateFrame(frameSetter, CFRangeMake(0, 0), path, NULL);
	
	CFRelease(path);
	CFRelease(frameSetter);
	
	return frame;
}


/** get line width 
 @remarks
	here line is assumed to be drawed horizontally
 */
GReal CTHelper::GetLineTypographicWidth( CTLineRef inLine)
{
	xbox_assert(inLine);
	
	VRect bounds;
	GetLineTypographicBounds( inLine, bounds, false);
	return bounds.GetWidth();
}

/** get line bounds 
 @remarks
	here line is assumed to be drawed horizontally
 */
void CTHelper::GetLineTypographicBounds( CTLineRef inLine, VRect& outBounds, bool inIncludeLeading)
{
	xbox_assert(inLine);
	
	//get line bounds
	CGFloat ascent,descent,leading;
	double width = CTLineGetTypographicBounds(inLine, &ascent, &descent, &leading);
	
	outBounds.SetX(0);
	outBounds.SetY(0);
	outBounds.SetWidth( (GReal) width);
	outBounds.SetHeight(ascent+descent+(inIncludeLeading ? leading : 0));
}

/** get frame number of lines */
uint32_t CTHelper::GetFrameLineCount( CTFrameRef inFrame)
{
	const CFArrayRef arrayLines = CTFrameGetLines( inFrame);
	CFIndex numLines = CFArrayGetCount( arrayLines);
	return numLines;
}

/** get frame typographic bounds 
 @remarks
	note that it is not equal to frame path bounds but 
	to the union of lines typographic bounds
 
 @note
	Core Text CTFrame is buggy in vertical mode:
    line typographic height and so frame typographic height is wrong while using vertical-oriented glyphs
    so in vertical mode, it is recommended to estimate frame height by using frame count of lines
    - CTHelper::GetFrameLineCount - and font line height (= font ascent+descent+leading)
	which should work in most of the cases
	TODO: wait for a API fix from Apple
 */
void CTHelper::GetFrameTypographicBounds( CTFrameRef inFrame, VRect& outBounds)
{
	//JQ 26/01/2010: fixed ACI0064205
	
	CGPathRef framePath = CTFrameGetPath(inFrame);
	CGRect frameRect = CGPathGetBoundingBox(framePath);

	CFArrayRef lines = CTFrameGetLines(inFrame);
	CFIndex numLines = CFArrayGetCount(lines);

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

		if(i == lastLineIndex)
		{
			//frame height = distance from the bottom of the last line to the top of the frame
			CGPoint lastLineOrigin;
			CTFrameGetLineOrigins(inFrame, CFRangeMake(lastLineIndex, 1), &lastLineOrigin);

			//maxHeight =  CGRectGetHeight(frameRect) - lastLineOrigin.y + descent; //add descent because line origin is on the baseline
			maxHeight =  CGRectGetMaxY(frameRect) - lastLineOrigin.y + descent; //add descent because line origin is on the baseline
		}
	}

	outBounds.SetX(0);
	outBounds.SetY(0);
	//JQ 16/08/2010: fixed ACI0067891
	// sc 19/08/2010 ACI0067928 width and height must be rounded
	outBounds.SetWidth((CGFloat)ceil(maxWidth)); //normalize to limit discrepencies 
	outBounds.SetHeight((CGFloat)ceil(maxHeight));
//	outBounds.SetWidth(maxWidth); 
//	outBounds.SetHeight(maxHeight);

	/*
	GReal width = 0;
	GReal height = 0;
	const CFArrayRef arrayLines = CTFrameGetLines( inFrame);
	CFIndex numLines = CFArrayGetCount( arrayLines);
	
	for (int i = 0; i < numLines; i++)
	{
		const CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(arrayLines, i); 
		VRect lineBounds;
		if (i == numLines-1)
			GetLineTypographicBounds(line, lineBounds, false);
		else 
			GetLineTypographicBounds(line, lineBounds, true);
		height += lineBounds.GetHeight();
		width = std::max(width, lineBounds.GetWidth());
	}
	outBounds = VRect(0,0,width,height);
	*/
}


/** get frame path bounds  
 @remarks
	it is equal to the bounds of the path used to create the frame
 */
void CTHelper::GetFramePathBounds( CTFrameRef inFrame, VRect& outBounds)
{
	CGPathRef path = CTFrameGetPath(inFrame);
	CGRect bounds = CGPathGetBoundingBox(path);
	outBounds.MAC_FromCGRect(bounds);
}


/** set Core Text attribute of type Real */
void CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, GReal inValue)
{
	CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, GREAL_IS_DOUBLE ? kCFNumberDoubleType : kCFNumberFloatType, &inValue);
	CFDictionarySetValue(inAttributes, inKey, number);
	CFRelease(number);
}

/** get Core Text attribute of type Real */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, GReal &outValue)
{
	CFNumberRef number = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&number))
	{
		CFNumberGetValue(number, GREAL_IS_DOUBLE ? kCFNumberDoubleType : kCFNumberFloatType, &outValue);
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type UINT32 */
void CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, uint32_t inValue)
{
	CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
	CFDictionarySetValue(inAttributes, inKey, number);
	CFRelease(number);
}

/** get Core Text attribute of type UINT32 */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, uint32_t &outValue)
{
	CFNumberRef number = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&number))
	{
		outValue = 0;
		CFNumberGetValue(number, kCFNumberSInt32Type, &outValue);
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type SINT32 */
void CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, int32_t inValue)
{
	CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
	CFDictionarySetValue(inAttributes, inKey, number);
	CFRelease(number);
}

/** get Core Text attribute of type SINT32 */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, int32_t &outValue)
{
	CFNumberRef number = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&number))
	{
		outValue = 0;
		CFNumberGetValue(number, kCFNumberSInt32Type, &outValue);
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type UINT8 */
void CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, uint8_t inValue)
{
	CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt8Type, &inValue);
	CFDictionarySetValue(inAttributes, inKey, number);
	CFRelease(number);
}

/** get Core Text attribute of type UINT8 */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, uint8_t &outValue)
{
	CFNumberRef number = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&number))
	{
		outValue = 0;
		CFNumberGetValue(number, kCFNumberSInt8Type, &outValue);
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type SINT8 */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, int8_t inValue)
{
	CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt8Type, &inValue);
	CFDictionarySetValue(inAttributes, inKey, number);
	CFRelease(number);
}

/** get Core Text attribute of type SINT8 */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, int8_t &outValue)
{
	CFNumberRef number = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&number))
	{
		outValue = 0;
		CFNumberGetValue(number, kCFNumberSInt8Type, &outValue);
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type CFBoolean specified as bool */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, bool inValue)
{
	CFDictionarySetValue(inAttributes, inKey, inValue ? kCFBooleanTrue : kCFBooleanFalse);
}

/** get Core Text attribute of type CFBoolean as a bool */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, bool &outValue)
{
	CFBooleanRef value = NULL;	
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&value))
	{
		outValue = CFBooleanGetValue(value) != 0;
		return true;
	}
	else 
		return false;
}


/** set Core Text attribute of type CFStringRef */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, CFStringRef inValue)
{
	CFDictionarySetValue(inAttributes, inKey, inValue);
}

/** get Core Text attribute of type CFStringRef 
 @remarks
	caller does not own returned value
    (so do not call CFRelease on returned value)
 */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, CFStringRef &outValue)
{
	return CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&outValue);
}



/** set Core Text attribute of type VString */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, const VString& inValue)
{
	CFStringRef value = inValue.MAC_RetainCFStringCopy();
	CFDictionarySetValue(inAttributes, inKey, value);
	CFRelease(value);
}

/** get Core Text attribute of type VString */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, VString &outValue)
{
	CFStringRef value = NULL;
	if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&value))
	{
		outValue.MAC_FromCFString(value);
		return true;
	}
	else
		return false;
}


/** set Core Text attribute of type VColor */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, const VColor& inValue)
{
	CTHelper::SetAttribute( inAttributes, inKey, inValue.GetRed()/255.0f, inValue.GetGreen()/255.0f, inValue.GetBlue()/255.0f, inValue.GetAlpha()/255.0f);
}


/** set Core Text attribute of type CGColorRef with the specified red, green, blue and alpha components */
void  CTHelper::SetAttribute( CFMutableDictionaryRef inAttributes, CFStringRef inKey, CGFloat inRed, CGFloat inGreen, CGFloat inBlue, CGFloat inAlpha)
{
#if CT_USE_COCOA	
	if (UseCocoaAttributes())
	{
		//here we need to use NSColor * but CGColorRef to ensure RTF methods will not crash
		//(because NSColor & CGColor are not toll-free bridged)
		
		//ensure Obj-C temp objects are released when method is closed to avoid leaks between Cocoa & Carbon
		NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
		
		NSMutableDictionary *nsDict = (NSMutableDictionary *)inAttributes;
		
		NSColor *color = [NSColor colorWithDeviceRed:(CGFloat)inRed green:(CGFloat)inGreen blue:(CGFloat)inBlue alpha:(CGFloat)inAlpha];
		[nsDict setObject:color forKey:(NSString *)inKey];
		
		[innerPool release]; //here color is released (because colorWithDeviceRed returns a autoreleased instance - only instances created with new or alloc+init are not autoreleased)
	}
	else
#endif
	{
		CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
		CGFloat components[] = { inRed, inGreen, inBlue, inAlpha };
		CGColorRef colorValue = CGColorCreate(rgbColorSpace, components);
		CGColorSpaceRelease(rgbColorSpace);
		
		CFDictionarySetValue(inAttributes, inKey, colorValue);
		
		CGColorRelease(colorValue);
	}
}

/** get Core Text attribute of type VColor */
bool CTHelper::GetAttribute( CFDictionaryRef inAttributes, CFStringRef inKey, VColor &outValue)
{
#if CT_USE_COCOA	
	if (UseCocoaAttributes())
	{
		//here we need to use NSColor * but CGColorRef to ensure RTF or HTML methods will not crash
		//(because NSColor & CGColor are not toll-free bridged)
		
		NSDictionary *nsDict = (NSDictionary *)inAttributes;
		
		//ensure Obj-C temp objects are released when method is closed to avoid leaks between Cocoa & Carbon
		NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
		
		NSColor *color = (NSColor *)[nsDict objectForKey:(NSString *)inKey];
		bool ok = color ? CTHelper::GetColor( (void *)color, outValue) : false;
		
		[innerPool release]; //here color is released (because objectForKey returns a autoreleased instance - only instances created with new or alloc+init are not autoreleased)
		return ok;
	}
	else
#endif
	{
		CGColorRef colorValue = NULL;
		if (CFDictionaryGetValueIfPresent(inAttributes, inKey, (const void**)&colorValue))
		{
			const CGFloat *components = CGColorGetComponents(colorValue);
			outValue = VColor( (uBYTE)std::min(components[0]*255.0,255.0), (uBYTE)std::min(components[1]*255.0,255.0), (uBYTE)std::min(components[2]*255.0,255.0), (uBYTE)std::min(components[3]*255.0,255.0));
			return true;
		}
		else
			return false;
	}
}

/** get NSColor * from VColor value   */
void *CTHelper::CreateNSColor( const VColor& inColor)
{
#if CT_USE_COCOA	
	//ensure Obj-C temp objects are released when method is closed to avoid leaks between Cocoa & Carbon
	
    NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];

	NSColor *color = [NSColor colorWithCalibratedRed:(CGFloat)(inColor.GetRed()/255.0f) green:(CGFloat)(inColor.GetGreen()/255.0f) blue:(CGFloat)(inColor.GetBlue()/255.0f) alpha:(CGFloat)(inColor.GetAlpha()/255.0f)];
	xbox_assert(color);
	
    [color retain];
    
    [innerPool release];
    
    return (void *)color;
#else
	return NULL;
#endif
}

/** get VColor from NSColor */
bool CTHelper::GetColor( void *inNSColor, VColor& outColor)
{
    bool result = false;
#if CT_USE_COCOA	
	NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
    
	NSColor *color = (inNSColor == NULL) ? NULL : [((NSColor *)inNSColor) colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
    if (!color)
        color = ((NSColor *)inNSColor); //maybe conversion has failed -> ensure we use uncalibrated color at least
    
	if (color && [color numberOfComponents] == 4)
	{
		CGFloat components[4];
		[color getComponents:components];
		outColor.FromRGBAColor((uBYTE)std::min(components[0]*255.0,255.0), (uBYTE)std::min(components[1]*255.0,255.0), (uBYTE)std::min(components[2]*255.0,255.0), (uBYTE)std::min(components[3]*255.0,255.0));
		result = true;
	}
    
  	[innerPool release]; //here autoreleased color (returned by colorUsingColorSpaceName) is released
#endif	
    return result;
}	

#if CT_USE_COCOA	

/** class used to ensure NSAttributedString::initWithHTML is called in main thread
	because initWithHTML internally uses WebKit to parse HTML & WebKit methods must be called in main thread otherwise crash...
	
	(bullshit because it should be initWithHTML responsibility to deal with WebKit constraint: perhaps Apple will make a fix one day...?)
 */
class VMessage_InitWithHTML : public VMessage
{
public:
	VMessage_InitWithHTML( NSMutableAttributedString *inAttString, NSData *inDataHTML)
	: fAttString( inAttString), fDataHTML( inDataHTML)
	{
	}
	
	
protected:
	
	virtual ~VMessage_InitWithHTML()
	{
	}

	virtual void	DoExecute ()
	{
		NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
		[fAttString initWithHTML:fDataHTML documentAttributes:NULL];
		[pool release];
	}
	
private:
	NSMutableAttributedString* fAttString;
	NSData *fDataHTML;
};

#endif

/** convert a HTML stream to a CFAttributedStringRef */
CFMutableAttributedStringRef CTHelper::CreateAttributeStringFromHTML( CFDataRef inDataHTML)
{
#if CT_USE_COCOA	
	NSMutableAttributedString *nsAttString = [NSMutableAttributedString alloc];
	if (!nsAttString)
		return NULL;
	
	if (VTask::GetCurrent()->IsMain())
		[nsAttString initWithHTML:(NSData *)inDataHTML documentAttributes:NULL];
	else 
	{
		VMessage_InitWithHTML *message = new VMessage_InitWithHTML(nsAttString, (NSData *)inDataHTML);
		message->SendTo(VTask::GetMain());
		ReleaseRefCountable( &message);
	}
	return (CFMutableAttributedStringRef) nsAttString;
#else
	return NULL;
#endif
}

/** convert a RTF stream to a CFAttributedStringRef */
CFMutableAttributedStringRef CTHelper::CreateAttributeStringFromRTF( CFDataRef inDataRTF)
{
#if CT_USE_COCOA
	NSMutableAttributedString *nsAttString = [[NSMutableAttributedString alloc] initWithRTF:(NSData *)inDataRTF documentAttributes:NULL];
	return (CFMutableAttributedStringRef) nsAttString;
#else
	return NULL;
#endif
}
	
/** convert a CFAttributedStringRef to a RTF stream */
CFDataRef CTHelper::CreateRTFFromAttributedString( CFAttributedStringRef inAttString)
{
#if CT_USE_COCOA	
	if (UseCocoaAttributes())
	{
		NSAutoreleasePool *innerPool = [[NSAutoreleasePool alloc] init];
	
		NSAttributedString *nsAttString = (NSAttributedString *)inAttString;
		NSRange range = { 0, [nsAttString length] };
		NSData *data = NULL;
#if VERSIONDEBUG
		sLONG retainCount = 0;
#endif
		if ([nsAttString respondsToSelector: @selector( RTFFromRange: documentAttributes:)])
		{
			data = [nsAttString RTFFromRange:range documentAttributes:NULL];
			if (data)
				[data retain];
#if VERSIONDEBUG
			if (data)
				retainCount = [data retainCount];
#endif
		}
		
		[innerPool release]; //release autoreleased data
#if VERSIONDEBUG
		if (data)
			retainCount = [data retainCount];
		xbox_assert(retainCount == 1 || retainCount == 0);
#endif
		return (CFDataRef)data;
	}
	else
#endif
		return NULL;
}



//
// class XMacFont
//

XMacFont::XMacFont( VFont* inFont, const VString& inFontFamilyName, const VFontFace& inFace, CGFloat inSize):fFont( inFont)
{
	fCTFontRef = NULL;
	fCTStringAttributes = NULL;
	
	if (inFace == 0 && inSize == 12.0)
	{
		//create base font ref with default size and typeface
		
		//JQ 22/06/2011: CoreText fails to create font in some strange cases because it does not handle well
		//PostScript names so we need to manually modify PostScript names in order to make CoreText happy...
		//(for instance 'Helvetica Neue Condensed Black' is mapped to the proper font 
		// but 'Helvetica Neue Black Condensed' is mapped to a completly different font (Lucida Grande...): 
		// very nasty because both are valid PostScript names (!))
		//FIXME: remove code marked with TEMP FIX when Apple have fixed this nasty bug
		//TEMP FIX BEGIN...
        VString condensedShort = " Cond";
        VString condensedShort2 = " Cond ";
		VString BoldCondensed = "Bold Condensed";
		VString BlackCondensed = "Black Condensed";
		if (inFontFamilyName.EndsWith(BoldCondensed, true))
		{
			VString xfamilyName;
			inFontFamilyName.GetSubString(1, inFontFamilyName.GetLength()-BoldCondensed.GetLength(),xfamilyName);
			xfamilyName.AppendCString("Condensed Bold");
			CFStringRef familyName = xfamilyName.MAC_RetainCFStringCopy();
			fCTFontRef = CTFontCreateWithName(familyName, (CGFloat) 12.0, NULL);
			CFRelease(familyName);
		}
		else if (inFontFamilyName.EndsWith(BlackCondensed, true))
		{
			VString xfamilyName;
			inFontFamilyName.GetSubString(1, inFontFamilyName.GetLength()-BlackCondensed.GetLength(),xfamilyName);
			xfamilyName.AppendCString("Condensed Black");
			CFStringRef familyName = xfamilyName.MAC_RetainCFStringCopy();
			fCTFontRef = CTFontCreateWithName(familyName, (CGFloat) 12.0, NULL);
			CFRelease(familyName);
		}
		else if (inFontFamilyName.EndsWith(condensedShort))
        {
            //replace " Cond" with " Condensed" (CoreText knows only "Condensed")
			VString xfamilyName;
			inFontFamilyName.GetSubString(1, inFontFamilyName.GetLength()-condensedShort.GetLength(),xfamilyName);
			xfamilyName.AppendCString(" Condensed");
			CFStringRef familyName = xfamilyName.MAC_RetainCFStringCopy();
			fCTFontRef = CTFontCreateWithName(familyName, (CGFloat) 12.0, NULL);
			CFRelease(familyName);
        }
		else if (inFontFamilyName.Find(condensedShort2,1,true) >= 1)
        {
            //replace " Cond " with " Condensed " (CoreText knows only "Condensed")
			VString xfamilyName = inFontFamilyName;
            xfamilyName.Exchange(condensedShort2, VString(" Condensed "));
			CFStringRef familyName = xfamilyName.MAC_RetainCFStringCopy();
			fCTFontRef = CTFontCreateWithName(familyName, (CGFloat) 12.0, NULL);
			CFRelease(familyName);
        }
        else
		//...TEMP FIX END
		{
			CFStringRef familyName = inFontFamilyName.MAC_RetainCFStringCopy();
			fCTFontRef = CTFontCreateWithName(familyName, (CGFloat) 12.0, NULL);
			CFRelease(familyName);
		}
		if (!fCTFontRef)
		{
			//backfall to serif Times New Roman
			fCTFontRef = CTFontCreateWithName( CFSTR("Times New Roman"), (CGFloat) 12.0, NULL);
		}
		if (!fCTFontRef)
		{
			fCTFontRef = NULL;
		}
		xbox_assert(fCTFontRef);
	}
	else
	{
		//create base-related font with custom size and typeface
		
		//get first base font with default size and typeface
		VFont *fontBase = fFont->RetainFont( inFontFamilyName, 0, (CGFloat) 12.0);
		xbox_assert( fontBase);
		if (!fontBase)
			return;
		CTFontRef fontBaseRef = fontBase->GetImpl().RetainCTFontRef();
		xbox_assert( fontBaseRef);
		if (!fontBaseRef)
		{
			fontBase->Release();
			return;
		}
		
        //backup base font traits (might be not null as font name passed to this method might be a postcript name - i.e. with stylistic name)
        CTFontSymbolicTraits baseFontTraits = CTFontGetSymbolicTraits(fontBaseRef);
        
		//determine symbolic traits from face
		CTFontSymbolicTraits fontTraits = FontFaceToCTFontSymbolicTraits( inFace);
        
		//create new font desc with desired size and/or traits
		if (fontTraits)
		{
            //fixed ACI0080070: never return a substituted font with another family name but try to return a font in the same family which matches most desired traits
            //                  (consistent with system text edit behavior)
            
			
            CTFontSymbolicTraits desiredTraits = fontTraits | baseFontTraits;
            
            //try to create derived font with new traits
			fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits, desiredTraits);

            if (fCTFontRef == NULL)
                 if (desiredTraits & (kCTFontExpandedTrait|kCTFontCondensedTrait))
                    //try without expanded/condensed traits
                    fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits & ~(kCTFontExpandedTrait|kCTFontCondensedTrait), desiredTraits);
            
            if (fCTFontRef == NULL)
            {
                if ((desiredTraits & (kCTFontItalicTrait|kCTFontBoldTrait)) == (kCTFontItalicTrait|kCTFontBoldTrait))
                {
                    //try removing bold trait (keep only italic)
                    fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits & ~kCTFontBoldTrait, desiredTraits);
                    
                    if (fCTFontRef == NULL)
                        //try removing italic trait (keep only bold)
                        fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits & ~kCTFontItalicTrait, desiredTraits);
                    
                    if (fCTFontRef == NULL && (desiredTraits & (kCTFontExpandedTrait|kCTFontCondensedTrait)))
                    {
                        //try again without expanded/condensed traits & italic and bold combinations
                        
                        //try removing bold trait (keep only italic)
                        fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits & ~(kCTFontExpandedTrait|kCTFontCondensedTrait|kCTFontBoldTrait), desiredTraits);
                        
                        if (fCTFontRef == NULL)
                            //try removing italic trait (keep only bold)
                            fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, desiredTraits & ~(kCTFontExpandedTrait|kCTFontCondensedTrait|kCTFontItalicTrait), desiredTraits);
                    }
               }
            }
            
            if (fCTFontRef == NULL)
                //derive current font with only base traits & new size (should succeed most of time)
                fCTFontRef = CTFontCreateCopyWithSymbolicTraits(fontBaseRef, inSize, NULL, baseFontTraits, baseFontTraits);
            /*
			if (fCTFontRef == NULL)
			{
				//try again using only true family name & new font traits
				//(for instance for font with full name 'Helvetica Neue Condensed Black' it is 'Helvetica Neue')
				
				CFStringRef familyName = CTFontCopyFamilyName(fontBaseRef);
				VString sName;
				sName.MAC_FromCFString(familyName);
				CFRelease(familyName);
				
				VFont *fontBaseFamily = fFont->RetainFont( sName, 0, (CGFloat) 12.0);
				xbox_assert( fontBaseFamily);
				if (fontBaseFamily)
				{	
					CTFontRef newfontBaseRef = fontBaseFamily->GetImpl().RetainCTFontRef();
					xbox_assert( newfontBaseRef);
					fontBaseFamily->Release();
					if (newfontBaseRef)
					{
						fCTFontRef = CTFontCreateCopyWithSymbolicTraits(newfontBaseRef, inSize, NULL, fontTraits | baseFontTraits, fontTraits | baseFontTraits); //we need to keep base font traits 
						CFRelease(fontBaseRef);
						fontBaseRef=newfontBaseRef;
					}
				}
				if (fCTFontRef == NULL)
				{
 					//get base font stylistic class (serif, sans-serif, monospace, script or symbolic styles)
					CFDictionaryRef baseTraitsAtt = CTFontCopyTraits(fontBaseRef);
					xbox_assert(baseTraitsAtt);
#if VERSIONDEBUG
					CFShow(baseTraitsAtt);
#endif
					uint32_t baseTraitsStyleClass = 0;
					CTHelper::GetAttribute( baseTraitsAtt, kCTFontSymbolicTrait, baseTraitsStyleClass);
					baseTraitsStyleClass &= kCTFontClassMaskTrait; //keep only style class 
					CFRelease(baseTraitsAtt);
				
					//try to find a similar font with the desired traits (but no matter the family this time)
					CFMutableDictionaryRef descAtt = CFDictionaryCreateMutable(	kCFAllocatorDefault, 0,
																			   &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
					xbox_assert(descAtt);
					
					//set font style name
					//CFStringRef nameStyle = CTFontCopyFamilyName(fontBaseRef);
					//CFDictionarySetValue( descAtt, kCTFontStyleNameAttribute, nameStyle);
					
					//set new font requested traits
					CFMutableDictionaryRef traitsAtt = CFDictionaryCreateMutable(	kCFAllocatorDefault, 0,
																				 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
					xbox_assert(traitsAtt);
					
					//search for font with same class style (serif, sans-serif, etc... ) as base font and new symbolic style (italic, bold, etc...) 
					CTHelper::SetAttribute( traitsAtt, kCTFontSymbolicTrait, (uint32_t)(fontTraits|baseTraitsStyleClass));
					CFDictionarySetValue( descAtt, kCTFontTraitsAttribute, traitsAtt);
					
					CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(descAtt);
					xbox_assert(descriptor);
					fCTFontRef = CTFontCreateWithFontDescriptor(descriptor, inSize, 0);
					
					CFRelease(descriptor);
					CFRelease(traitsAtt);
					//CFRelease(nameStyle);     
					CFRelease(descAtt);
				}
 			}
            */
		}
			
		if (fCTFontRef == NULL)
		{
			//at least, return derived font with new size (this should always work)
		
			CFStringRef familyName2 = CTFontCopyPostScriptName(fontBaseRef);
			xbox_assert(familyName2);
			fCTFontRef = CTFontCreateWithName(familyName2, inSize, NULL);
			CFRelease(familyName2);
		}
		CFRelease(fontBaseRef);
		fontBase->Release();
		xbox_assert(fCTFontRef);
	}
	
    if (fCTFontRef)
	{
		//create default string attributes (a string that is drawed with this font must use at least these attributes)
		fCTStringAttributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
														&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		xbox_assert(fCTStringAttributes);
		
		//set font 
		CFDictionarySetValue( fCTStringAttributes, kCTFontAttributeName, fCTFontRef);
			
		//set underline style
		if ((inFace & KFS_UNDERLINE) != 0)
			CTHelper::SetAttribute( fCTStringAttributes, kCTUnderlineStyleAttributeName, (int32_t)kCTUnderlineStyleSingle);
			
		//set strikeout style
		if ((inFace & KFS_STRIKEOUT) != 0)
			CTHelper::SetAttribute( fCTStringAttributes, CFSTR("NSStrikethrough"), (int32_t)kCTUnderlineStyleSingle);
        
#if WITH_DEBUGMSG
        VString msg = "\ndesired font: name:";
        msg.AppendString(inFontFamilyName);
        msg.AppendCString(" face:");
        msg.AppendLong(inFace);
        msg.AppendCString(" size:");
        msg.AppendReal( (Real)inSize);
        msg.AppendCString("\n");
        XBOX::DebugMsg(msg);
        
        XBOX::DebugMsg("font successfully created:\npostScript name:\n");
        CFStringRef name = CTFontCopyPostScriptName(fCTFontRef);
        CFShow(name);
        CFRelease(name);
        XBOX::DebugMsg("family name:\n");
        name = CTFontCopyFamilyName(fCTFontRef);
        CFShow(name);
        CFRelease(name);
        XBOX::DebugMsg("display name:\n");
        name = CTFontCopyFullName(fCTFontRef);
        CFShow(name);
        CFRelease(name);
#endif        
	}
}


/** convert 4D font traits to Core Text font descriptor symbolic traits */
CTFontSymbolicTraits XMacFont::FontFaceToCTFontSymbolicTraits( const VFontFace& inFace)
{
	CTFontSymbolicTraits traits = 0;
	
	if (inFace & KFS_ITALIC)
		traits |= kCTFontItalicTrait;
	
	if (inFace & KFS_BOLD)
		traits |= kCTFontBoldTrait;
	
	if (inFace & KFS_EXTENDED)
		traits |= kCTFontExpandedTrait;
	
	if (inFace & KFS_CONDENSED)
		traits |= kCTFontCondensedTrait;
	
	return traits;
}

VFontFace XMacFont::CTFontSymbolicTraitsToFontFace( const CTFontSymbolicTraits& inTraits)
{
	VFontFace face = 0;
	
	if (inTraits & kCTFontItalicTrait)
		face |= KFS_ITALIC;
	
	if (inTraits & kCTFontBoldTrait)
		face |= KFS_BOLD;
	
	if (inTraits & kCTFontExpandedTrait)
		face |= KFS_EXTENDED;
	
	if (inTraits & kCTFontCondensedTrait)
		face |= KFS_CONDENSED;
	
	return face;
}

/** retain and return CTFont reference */
CTFontRef XMacFont::RetainCTFontRef () const
{ 
	xbox_assert(fCTFontRef);
	if (fCTFontRef)
	{
		CFRetain(fCTFontRef); 
		return fCTFontRef; 
	}
	else
		return NULL;
}


/** create and retain dictionnary with Core Text descriptor attributes 
 fully describing current font */
CFMutableDictionaryRef  XMacFont::RetainStringAttributes() const
{
	xbox_assert(fCTStringAttributes);
	if (fCTStringAttributes)
	{
		CFRetain(fCTStringAttributes); 
		return fCTStringAttributes; 
	}
	else
		return NULL;
}


void XMacFont::ApplyToPort(PortRef inPort, CGFloat inScale) 
{
	//deprecated with Core Text implementation 
	//(Core Text font reference is encoded in line or frame string attributes)
	//but keep method for backward compatibility
}


void XMacFont::GetPostScriptName(VString& outName) const
{
	CFStringRef cfName = CTFontCopyPostScriptName( fCTFontRef);
	if (cfName != NULL)
	{
		outName.MAC_FromCFString( cfName);
		::CFRelease( cfName);
	}
	else
	{
		outName.Clear();
	}
}

void XMacFont::GetFamilyName(VString& outName) const
{
	CFStringRef cfName = CTFontCopyFamilyName( fCTFontRef);
	if (cfName != NULL)
	{
		outName.MAC_FromCFString( cfName);
		::CFRelease( cfName);
	}
	else
	{
		outName.Clear();
	}
}

Style XMacFont::FontFaceToStyle(const VFontFace& inFace)
{
	Style	face = normal;
	
	if (inFace & KFS_ITALIC)
		face |= italic;
	
	if (inFace & KFS_BOLD)
		face |= bold;
	
	if (inFace & KFS_UNDERLINE)
		face |= underline;
	
	if (inFace & KFS_EXTENDED)
		face |= extend;
	
	if (inFace & KFS_CONDENSED)
		face |= condense;
	
	if (inFace & KFS_SHADOW)
		face |= shadow;
	
	if (inFace & KFS_OUTLINE)
		face |= outline;
	
	return face;
}


void XMacFont::StyleToFontFace(Style inFace, VFontFace& outFace)
{
	outFace = KFS_NORMAL;
	
	if (inFace & italic)
		outFace |= KFS_ITALIC;
	
	if (inFace & bold)
		outFace |= KFS_BOLD;
	
	if (inFace & underline)
		outFace |= KFS_UNDERLINE;
	
	if (inFace & extend)
		outFace |= KFS_EXTENDED;
	
	if (inFace & condense)
		outFace |= KFS_CONDENSED;
	
	if (inFace & shadow)
		outFace |= KFS_SHADOW;
	
	if (inFace & outline)
		outFace |= KFS_OUTLINE;
}


FMFontStyle XMacFont::FontFaceToFMFontStyle(const VFontFace& inFace)
{
	return FontFaceToStyle(inFace);
}


void XMacFont::FMFontStyleToFontFace(FMFontStyle inFace, VFontFace& outFace)
{
	Style	face = inFace & 0xFF;
	StyleToFontFace(face, outFace);
}

#if WITH_QUICKDRAW
sWORD XMacFont::FontNameToFontNum(const VString& inFontName)
{
	Str255	macName;
	inFontName.ToBlock(&macName[0], sizeof(macName), VTC_MacOSAnsi, false, true);
	
	return ::FMGetFontFamilyFromName(macName);
}


void XMacFont::FontNumToFontName(sWORD inFontID, VString& outFontName)
{
	Str255	macName = "\p";
	::FMGetFontFamilyName(inFontID, macName);
	
	outFontName.FromBlock(macName + 1, macName[0], VTC_MacOSAnsi);
}
#endif

#pragma mark-




//
//font metrics management
//

void XMacFontMetrics::GetMetrics( CGFloat& outAscent, CGFloat& outDescent, CGFloat& outLeading) const
{
	const VFont* font = fMetrics->GetFont();
    outAscent = font->GetImpl().GetAscent();
    outDescent = font->GetImpl().GetDescent();
    outLeading = font->GetImpl().GetLeading();
}


void XMacFontMetrics::GetMetrics( CGFloat& outAscent, CGFloat& outDescent, CGFloat& outInternalLeading, CGFloat& outExternalLeading,CGFloat& outLineHeight) const
{
	const VFont* font = fMetrics->GetFont();
    outAscent = font->GetImpl().GetAscent();
    outDescent = font->GetImpl().GetDescent();
	outInternalLeading	= (CGFloat) 0.0;
    outExternalLeading = font->GetImpl().GetLeading();
    outLineHeight = outAscent + outDescent + outExternalLeading;
}

GReal XMacFontMetrics::GetCharWidth(UniChar inChar) 
{
	VStr<1> s( inChar);
	return GetTextWidth( s);
}


//
//text factory management
//

/** set dirty state for text attributes */ 
void XMacTextFactory::SetDirty()
{
	if (fTextAttributes)
		CFRelease(fTextAttributes);
	fTextAttributes = NULL;
	
	if (fTextLine)
		CFRelease( fTextLine);
	fTextLine = NULL;
	if (fTextFrame)
		CFRelease( fTextFrame);
	fTextFrame = NULL;
}

/** update text string attributes using font default attributes and text rendering+layout */ 
void XMacTextFactory::_UpdateTextAttributes()
{
	SetDirty();
	VColor colorText;
	fMetrics->GetFontMetrics()->GetContext()->GetTextColor(colorText);
	fTextAttributes = CTHelper::CreateStringAttributes( fMetrics->GetFontMetrics()->GetFont(), colorText, GetRenderingMode(), GetLayoutMode(),
														fMetrics->GetFontMetrics()->GetContext()->GetCharActualKerning());
}


/** return true if char is ideographic character 
	(CJK, katakana, hiragana, hangul, bopomofo or Yi unicode character) 
	or fullwidth latin character
 
 @remarks
	based on unicode 5.1 standard reference (UTF16 bytecodes only)
 */
bool XMacTextFactory::_IsUniCharIdeographicOrLatinFullWidth( UniChar inChar)
{
	if (inChar < 0x1100)
		return false;
	if (inChar <= 0x11ff)
		//hangul jamo
		return true;
	if (inChar < 0x2E80)
		return false;
	if (inChar <= 0x2EFF)
		//CJK Radical Supplement
		return true;
	if (inChar <= 0x2FDF)
		//Kangxi radicals
		return true;
	if (inChar < 0x2FF0)
		return false;
	if (inChar <= 0x2FFF)
		//Ideographic description characters
		return true;
	if (inChar <= 0x303F)
		//CJK symbols and punctuation
		return true;
	if (inChar <= 0x309F)
		//hiragana
		return true;
	if (inChar <= 0x30FF)
		//katakana
		return true;
	if (inChar <= 0x312F)
		//Bopomofo
		return true;
	if (inChar <= 0x318F)
		//Hangul compatibility Jamo
		return true;
	if (inChar <= 0x319F)
		//Kanbun
		return true;
	if (inChar <= 0x31BF)
		//Bopomofo extended
		return true;
	if (inChar <= 0x31EF)
		//CJK strokes
		return true;
	if (inChar <= 0x31FF)
		//Katakana phonetic extensions
		return true;
	if (inChar <= 0x32FF)
		//Enclosed CJK Letters and Months
		return true;
	if (inChar <= 0x33FF)
		//CJK compatibility
		return true;
	if (inChar <= 0x4DBF)
		//CJK Unified Ideographs Extension A
		return true;
	if (inChar <= 0x4DFF)
		//Yijing Hexagram Symbols
		return true;
	if (inChar <= 0x9FFF)
		//CJK Unified Ideographs
		return true;
	if (inChar <= 0xA48F)
		//Yi Syllables
		return true;
	if (inChar <= 0xA4CF)
		//Yi Radicals
		return true;
	if (inChar < 0xAC00)
		return false;
	if (inChar <= 0xD7AF)
		//Hangul Syllables
		return true;
	if (inChar < 0xF900)
		return false;
	if (inChar <= 0xFAFF)
		//CJK Compatibility Ideographs
		return true;
	if (inChar < 0xFE30)
		return false;
	if (inChar <= 0xFE4F)
		//CJK Compatibility Forms
		return true;
	if (inChar >= 0xFF00 && inChar <= 0xFFEF)
		//latin fullwidth and ideographic halfwidth forms
		return true;
	return false;
}


/** build halfwidth latin character ranges and return retained attributes */
CFDictionaryRef XMacTextFactory::_BuildHalfwidthLatinCharacterRanges()
{
	//TODO: for now, Core Text - like ATSUI - is buggy while mixing character orientation in a string
	//		(this is used in chinese/japanese documents with vertical layout to rotate 90 latin characters)
	//		but you can remove the following line when Apple makes the fix - if they do...
	//		so for now, halfwidth latin characters are oriented vertically like fullwidth characters
	return NULL;
	
	//first scan string and build latin range vector
	fLatinStyleRanges.clear();
	const UniChar *pCar = fTextString.GetCPointer();
	SInt32 indexFirst = -1;
	SInt32 indexCur = 0;
	while (*pCar)
	{
		if (*pCar != ' ')
		{
			if (indexFirst == -1)
			{
				if (!_IsUniCharIdeographicOrLatinFullWidth( *pCar))
					//start latin sub-range
					indexFirst = indexCur;
			}
			else
			{
				if (_IsUniCharIdeographicOrLatinFullWidth( *pCar))
				{
					//end latin sub-range and store range
					fLatinStyleRanges.push_back( CTUserStyleRange( indexFirst, indexCur-indexFirst));
					indexFirst = -1;
				}
			}
		}
		indexCur++;
		pCar++;
	}
	if (indexFirst >= 0)
	{
		//end latin sub-range and store range
		fLatinStyleRanges.push_back( CTUserStyleRange( indexFirst, indexCur-indexFirst));
	}
	
	if (fLatinStyleRanges.empty())
		//text has only ideographic or fullwidth latin characters:
		//use only default attributes
		return NULL;
	else
	{
		//text has mixed halfwidth latin and ideographic or fullwidth latin characters:
		//create attributes for halfwidth latin orientation
	
		VColor colorText;
		fMetrics->GetFontMetrics()->GetContext()->GetTextColor(colorText);
		CFDictionaryRef attributes = CTHelper::CreateStringAttributes( fMetrics->GetFontMetrics()->GetFont(), colorText, GetRenderingMode(), GetLayoutMode() & (~TLM_VERTICAL),
																		fMetrics->GetFontMetrics()->GetContext()->GetCharActualKerning());
		return attributes;
	}
}


/** create line from text, current attributes and current line layout */
CTLineRef	XMacTextFactory::CreateLine( const VString& inText)
{
	if (fTextAttributes == NULL)
		_UpdateTextAttributes();
	
	if (fTextLine == NULL || inText.EqualToString( fTextString, true) == false)
	{
		//create new line only if different from current line
		
		fTextString = inText;
		if (fTextLine)
			CFRelease( fTextLine);
		
		if ((!inText.IsEmpty()) && (GetLayoutMode() & TLM_VERTICAL))
		{
			//we need to deal with mixed halfwidth latin and ideographic or fullwidth latin characters
			//because orientation is not the same and so the style
			
			CFDictionaryRef attributesLatin = _BuildHalfwidthLatinCharacterRanges();
			fTextLine = CTHelper::CreateLine( fTextString, fTextAttributes, attributesLatin, &fLatinStyleRanges);
			if (attributesLatin)
				CFRelease(attributesLatin);
		}
		else
			fTextLine = CTHelper::CreateLine( fTextString, fTextAttributes);
		
		if (fTextFrame)
			CFRelease( fTextFrame);
		fTextFrame = NULL;
	}
	
	if (fTextLine != NULL)
	{
		CFRetain( fTextLine);
		return fTextLine;
	}
	else
		return NULL;
}

/** create frame from text, current attributes and current frame layout */
CTFrameRef	XMacTextFactory::CreateFrame( const VString& inText, const VRect& inBounds)
{
	if (fTextAttributes == NULL)
		_UpdateTextAttributes();
	
	if (fTextFrame == NULL 
		|| 
		inText.EqualToString( fTextString, true) == false 
		|| 
		inBounds != fTextFrameBounds
		)
	{
		//create new frame only if different from current frame
		
		//JQ 29/03/2010: fixed text string normalization
		
		//normalize text string 
		fTextString.SetEmpty();
		fTextString.EnsureSize(inText.GetLength());
		const UniChar *pChar = inText.GetCPointer();
		while (*pChar)
		{
			if (!(*pChar == 13 && *(pChar+1) == 10))
			{
				if (*pChar == 10)
					fTextString.AppendUniChar( 13);
				else 
					fTextString.AppendUniChar(*pChar);
			}
			pChar++;
		}
		fTextFrameBounds = inBounds;
		
		if (fTextFrame)
			CFRelease( fTextFrame);
		
		if ((!inText.IsEmpty()) && (GetLayoutMode() & TLM_VERTICAL))
		{
			//we need to deal with mixed halfwidth latin and ideographic or fullwidth latin characters
			//because orientation is not the same and so the style
			
			CFDictionaryRef attributesLatin = _BuildHalfwidthLatinCharacterRanges();
			fTextFrame = CTHelper::CreateFrame( fTextString, fTextAttributes, fTextFrameBounds, attributesLatin, &fLatinStyleRanges);
			if (attributesLatin)
				CFRelease(attributesLatin);
		}
		else
			fTextFrame = CTHelper::CreateFrame(fTextString, fTextAttributes, fTextFrameBounds);
		
		if (fTextLine)
			CFRelease( fTextLine);
		fTextLine = NULL;
	}
	
	if (fTextFrame != NULL)
	{
		CFRetain( fTextFrame);
		return fTextFrame;
	}
	else
		return NULL;
}


GReal XMacTextFactory::GetTextWidth( const VString& inString, bool inRTL)
{
    TextLayoutMode layoutMode = fLayoutMode;
    if (inRTL)
    {
        fLayoutMode |= TLM_RIGHT_TO_LEFT;
        if (fLayoutMode != layoutMode)
            SetDirty();
    }
    else
    {
        //note: if TLM_RIGHT_TO_LEFT is not defined, direction is strongly left to right (for compliancy with GDI, GDIPlus & DWrite frameworks)
        fLayoutMode &= ~TLM_RIGHT_TO_LEFT;
        if (fLayoutMode != layoutMode)
            SetDirty();
    }
    
	CTLineRef line = CreateLine(inString);
	GReal width = CTHelper::GetLineTypographicWidth( line);
	
	CFRelease(line);
	
	return width;
}


void XMacTextFactory::MeasureText( const VString& inString, std::vector<GReal>& outOffsets, bool inRTL)
{
    TextLayoutMode layoutMode = fLayoutMode;
    if (inRTL)
    {
        fLayoutMode |= TLM_RIGHT_TO_LEFT;
        if (fLayoutMode != layoutMode)
            SetDirty();
    }
    else
    {
        //note: if TLM_RIGHT_TO_LEFT is not defined, direction is strongly left to right (for compliancy with GDI, GDIPlus & DWrite frameworks)
        fLayoutMode &= ~TLM_RIGHT_TO_LEFT;
        if (fLayoutMode != layoutMode)
            SetDirty();
    }
    
	CTLineRef line = CreateLine(inString);
	
	for( VIndex i = 1 ; i <= inString.GetLength() ; ++i)
	{
		CGFloat secondaryOffset;
		CGFloat primaryOffset = CTLineGetOffsetForStringIndex( line, i, &secondaryOffset);
		outOffsets.push_back( primaryOffset);
	}
	if (inRTL)
    {
        //for RTL, we need offsets to be relative to RTL start position which is on the right
		CGFloat secondaryOffset;
        CGFloat width = CTLineGetOffsetForStringIndex( line, 0, &secondaryOffset);
        std::vector<GReal>::iterator it = outOffsets.begin();
        for(;it != outOffsets.end(); it++)
            *it = width-(*it);
    }
	CFRelease(line);
}


bool XMacTextFactory::MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos) 
{
	CTLineRef line = CreateLine(inString);
	
	//get line character index from mouse position
	CGPoint offset = { (float)(inMousePos - ioPixPos), (float)(0.0) };
	CFIndex indexChar = CTLineGetStringIndexForPosition(line, offset);
	if (indexChar == kCFNotFound)
		//text search is not supported: exit (normally it should never occur)
		return false;
	
	//map character index to caret pixel offset
	CGFloat secondaryOffset;
	CGFloat primaryOffset = CTLineGetOffsetForStringIndex(line, indexChar, &secondaryOffset);
	
	//output pixel and character offset
	ioPixPos += primaryOffset;
	ioTextPos += (sLONG)indexChar;
	
	CFRelease(line);
	
	return indexChar < inString.GetLength();
}

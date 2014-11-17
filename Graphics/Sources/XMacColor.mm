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
#include <AppKit/AppKit.h>
#include "VColor.h"


void VColor::MAC_FromCGColorRef( CGColorRef inColor)
{
	if (inColor != NULL)
	{
		CGColorSpaceRef colorSpace = CGColorGetColorSpace( inColor);
		CGColorSpaceModel colorSpaceModel = CGColorSpaceGetModel( colorSpace);
		const CGFloat *components = CGColorGetComponents( inColor);
		size_t numberOfComponents = CGColorGetNumberOfComponents( inColor);

		if (colorSpaceModel == kCGColorSpaceModelRGB)
		{
			fRed = (uBYTE)std::min<CGFloat>(components[0]*255,255);
			fGreen = (uBYTE)std::min<CGFloat>(components[1]*255,255);
			fBlue = (uBYTE)std::min<CGFloat>(components[2]*255,255);
			fAlpha = (uBYTE)std::min<CGFloat>(components[3]*255,255);
		}
		else
		{
			NSAutoreleasePool *localpool = [[NSAutoreleasePool alloc] init];
			
			// convert to NSColor to call colorUsingColorSpace (didn't find simpler way to convert colorspace)
			NSColorSpace *nsColorSpace = [[[NSColorSpace alloc]initWithCGColorSpace:colorSpace] autorelease];
			NSColor *nsColor = [NSColor colorWithColorSpace:nsColorSpace components:components count:numberOfComponents];
			
			CGFloat red, green, blue, alpha;
			//[ [nsColor colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]] getRed:&red green:&green blue:&blue alpha:&alpha];
			NSColor *color = [nsColor colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
			if ([color respondsToSelector:@selector(getComponents:)])
			{
				CGFloat colors[4];
				[color getComponents:colors];
				red = colors[0];
				green = colors[1];
				blue = colors[2];
				alpha = colors[3];
			}
			else if ([color respondsToSelector:@selector(getRed:green:blue:alpha:)])
				[color getRed:&red green:&green blue:&blue alpha:&alpha];
			
			fRed = (uBYTE)std::min<CGFloat>(red*255,255);
			fGreen = (uBYTE)std::min<CGFloat>(green*255,255);
			fBlue = (uBYTE)std::min<CGFloat>(blue*255,255);
			fAlpha = (uBYTE)std::min<CGFloat>(alpha*255,255);
			
			[localpool release];	
		}
	}
}


CGColorRef VColor::MAC_CreateCGColorRef() const
{
	return CGColorCreateGenericRGB( (CGFloat)fRed/255, (CGFloat)fGreen/255, (CGFloat)fBlue/255, (CGFloat)fAlpha/255);
}

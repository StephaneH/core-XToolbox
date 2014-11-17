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
#include "VColor.h"


const VColor	VColor::sWhiteColor(COL_WHITE);
const VColor	VColor::sBlackColor(COL_BLACK);


VColor::VColor(uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha)
{
	fRed = inRed;
	fGreen = inGreen;
	fBlue = inBlue;
	fAlpha = inAlpha;
}


VColor::VColor(const VColor& inOriginal)
{
	fRed = inOriginal.fRed;
	fGreen = inOriginal.fGreen;
	fBlue = inOriginal.fBlue;
	fAlpha = inOriginal.fAlpha;
}


VColor::VColor(const VColor* inOriginal)
{
	if (inOriginal != NULL)
	{
		fRed = inOriginal->fRed;
		fGreen = inOriginal->fGreen;
		fBlue = inOriginal->fBlue;
		fAlpha = inOriginal->fAlpha;
	}
	else
	{
		fRed = 0;
		fGreen = 0;
		fBlue = 0;
		fAlpha = 0xFF;
	}
}


VColor& VColor::operator = (const VColor &inVColor)
{
	fRed = inVColor.fRed;
	fGreen = inVColor.fGreen;
	fBlue = inVColor.fBlue;
	fAlpha = inVColor.fAlpha;
	return *this;
}


bool VColor::operator == (const VColor &inVColor) const
{
	return (fRed == inVColor.fRed && fGreen == inVColor.fGreen && fBlue == inVColor.fBlue && fAlpha == inVColor.fAlpha);
}


bool VColor::operator != (const VColor &inVColor) const
{
	return (fRed != inVColor.fRed || fGreen != inVColor.fGreen || fBlue != inVColor.fBlue || fAlpha != inVColor.fAlpha);
}

void VColor::GetColor( uWORD *outRed, uWORD *outGreen, uWORD *outBlue, uWORD *outAlpha ) const
{
	// to avoid having FF transformed into FF00
	// FF -> FF
	// A0 -> A000
	if (outRed != NULL)
		*outRed = ( fRed << 8) | (( fRed & 0xF) << 4) | (fRed & 0xF);
	if (outGreen != NULL)
		*outGreen = ( fGreen << 8) | (( fGreen & 0xF) << 4) | ( fGreen & 0xF);
	if (outBlue != NULL)
		*outBlue = ( fBlue << 8) | (( fBlue & 0xF) << 4) | ( fBlue & 0xF);
	if (outAlpha != NULL)
		*outAlpha = ( fAlpha << 8) | (( fAlpha & 0xF) << 4) | ( fAlpha & 0xF);
}

void VColor::FromRGBAColor (uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha)
{
	fRed = inRed;
	fGreen = inGreen;
	fBlue = inBlue;
	fAlpha = inAlpha;
}


void VColor::FromRGBAColor (uLONG inColor)
{
	fRed = (uBYTE) ((inColor >> 16) & 0x000000FF);
	fGreen = (uBYTE) ((inColor >> 8 ) & 0x000000FF);
	fBlue = (uBYTE) (inColor & 0x000000FF); 
	fAlpha = (uBYTE) ((inColor >> 24) & 0x000000FF);
}


uLONG VColor::GetRGBAColor (bool inPremultiply) const
{
	if(!inPremultiply)
		return ((uLONG)fBlue | ((uLONG)fGreen) << 8 | ((uLONG)fRed) << 16 | ((uLONG)fAlpha) << 24);
	else
	{
		uLONG result;
		result = ((uLONG)fAlpha) << 24;
		result |= ((fAlpha * fRed +127 ) /255)<<16;
		result |= ((fAlpha * fGreen +127) /255)<<8;
		result |= (fAlpha * fBlue + 127 ) /255;
		return result;
	}
}


void VColor::FromStdColor (StdColor inStdColor)
{
	fAlpha = 0xFF;
	
	switch (inStdColor)
	{
		case COL_WHITE:
			fRed = fGreen = fBlue = 0xFF;
			break;
			
		case COL_GRAY5:
			fRed = fGreen = fBlue = 0xF0;
			break;
			
		case COL_GRAY10:
			fRed = fGreen = fBlue = 0xDF;
			break;
			
		case COL_GRAY20:
			fRed = fGreen = fBlue = 0xCC;
			break;
			
		case COL_GRAY30:
			fRed = fGreen = fBlue = 0xB3;
			break;
			
		case COL_GRAY40:
			fRed = fGreen = fBlue = 0x99;
			break;
			
		case COL_GRAY50:
			fRed = fGreen = fBlue = 0x80;
			break;
			
		case COL_GRAY60:
			fRed = fGreen = fBlue = 0x68;
			break;
			
		case COL_GRAY70:
			fRed = fGreen = fBlue = 0x4D;
			break;
			
		case COL_GRAY80:
			fRed = fGreen = fBlue = 0x34;
			break;
			
		case COL_GRAY90:
			fRed = fGreen = fBlue = 0x1A;
			break;
			
		case COL_BLACK:
			fRed = fGreen = fBlue = 0x00;
			break;
			
		case COL_RED:
			fRed = 0xFF;
			fGreen = 0x00;
			fBlue = 0x00;
			break;
			
		case COL_GREEN:
			fRed = 0x00;
			fGreen = 0xFF;
			fBlue = 0x00;
			break;
			
		case COL_BLUE:
			fRed = 0x00;
			fGreen = 0x00;
			fBlue = 0xFF;
			break;
			
		case COL_YELLOW:
			fRed = 0xFF;
			fGreen = 0xFF;
			fBlue = 0x00;
			break;
			
		case COL_MAGENTA:
			fRed = 0xFF;
			fGreen = 0x00;
			fBlue = 0xFF;
			break;
			
		case COL_CYAN:
			fRed = 0x00;
			fGreen = 0xFF;
			fBlue = 0xFF;
			break;
			
		default:
			// You should use LookHandler::GetStdColor
//			assert(false);
			break;
	}
}


void VColor::GetHSL(sWORD& outHue, sWORD& outSaturation, sWORD& outLuminosity ) const
{
	if ( fRed == 0 && fGreen == 0 && fBlue == 0 )	// sc 21/03/2007, crashing bug
	{
		outHue = 0;
		outSaturation = 0;
		outLuminosity = 0;
	}
	else
	{
		GReal r, g, b;
		GReal mincolor, maxcolor;
		GReal luminosity, saturation, hue;
		GReal diff, sum;
		int c;

		r = (GReal) fRed   / 255;
		g = (GReal) fGreen / 255;
		b = (GReal) fBlue  / 255;

		if ( r > g )
		{
			if ( r > b )
			{
				maxcolor = r;
				c = 0;
				if ( g > b )
					mincolor = b;
				else
					mincolor = g;
			}
			else
			{
				c = 2;
				maxcolor = b;
				mincolor = g;
			}
		}
		else
		{
			if ( g > b )
			{
				maxcolor = g;
				c = 1;
				if ( r > b )
					mincolor = b;
				else
					mincolor = r;
			}
			else
			{
				c = 2;
				maxcolor = b;
				mincolor = r;
			}
		}

		sum  = maxcolor + mincolor;
		diff = maxcolor - mincolor;

		if(sum==0)
			sum=1;

		luminosity = sum / 2;
		
		if ( luminosity < (GReal) 0.5 )
			saturation = diff / sum;
		else
			saturation = diff / ( 2 - diff );

		if ( diff == 0 )
			hue = 0;
		else
		{
			if ( c == 0 )
				hue = ( g - b ) / diff;
			else if ( c == 1 )
				hue = 2 + ( b - r ) / diff;
			else
				hue = 4 + ( r - g ) / diff;
		}

		outHue        = (sWORD) ( hue        * 60 );
		outSaturation = (sWORD) ( saturation * 100 );
		outLuminosity = (sWORD) ( luminosity * 100 );
	}
}

GReal VColor::_MakeHue(GReal m1,GReal m2,GReal h)
{
	if( h < 0 ) h += 1;
	if( h > 1 ) h -= 1;
	if( 6*h < 1 )
		return (m1+(m2-m1)*h*6);
	if( 2*h < 1 )
		return m2;
	if( 3*h < 2 )
		return (m1+(m2-m1)*(((GReal)2/(GReal)3)-h)*(GReal)6);
	return m1;
}

void VColor::Combine( const VColor& inColor1, const VColor& inColor2, VColor& outResult )
{
/*	sWORD hue1, hue2;
	sWORD sat1, sat2;
	sWORD lum1, lum2;

	inColor1.GetHSL( hue1, sat1, lum1 );
	inColor2.GetHSL( hue2, sat2, lum2 );

	outResult.FromHSL( ( hue1 + hue2 ) / 2, ( sat1 + sat2 ) / 2, ( lum1 + lum2 ) / 2 );
*/
	outResult.fRed   = ( inColor1.fRed   + inColor2.fRed   ) / 2;
	outResult.fGreen = ( inColor1.fGreen + inColor2.fGreen ) / 2;
	outResult.fBlue  = ( inColor1.fBlue  + inColor2.fBlue  ) / 2;
}


void VColor::FromHSL( sWORD inHue, sWORD inSaturation, sWORD inLuminosity )
{
	GReal hue        = (GReal) inHue        / 360;
	GReal luminosity = (GReal) inLuminosity / 100;
	GReal saturation = (GReal) inSaturation / 100;
	GReal temp1, temp2, rtemp3, gtemp3, btemp3, diff;
	GReal red, green, blue;

	if ( 0 == inSaturation )
		fRed = fGreen = fBlue = (uBYTE) ( luminosity * 255 );
	else
	{
		if ( luminosity < (GReal) 0.5 )
			temp2 = luminosity * ( 1 + saturation );
		else
			temp2 = ( luminosity + saturation ) - ( luminosity * saturation );

		temp1 = ( 2 * luminosity ) - temp2;

		red = _MakeHue(temp1,temp2,hue+1/(GReal)3);
		green = _MakeHue(temp1,temp2,hue);
		blue = _MakeHue(temp1,temp2,hue-1/(GReal)3);
		#if 0
		rtemp3 = hue + ( 1.0 / 3.0 );
		gtemp3 = hue;
		btemp3 = hue - ( 1.0 / 3.0 );
		if ( rtemp3 > 1.0 )
			rtemp3 -= 1.0;
		if ( btemp3 < 1.0 )
			btemp3 += 1.0;

		diff = temp2 - temp1; 

		// red
		if ( 6.0 * rtemp3 < 1.0 )
			red = temp1 + diff * 6.0 * rtemp3;
		else if ( 2.0 * rtemp3 < 1.0 )
			red = temp2;
		else if ( 3.0 * rtemp3 < 2.0 )
			red = temp1 + diff * ( ( 2.0 / 3.0 ) - rtemp3 ) * 6.0;
		else
			red = temp1;

		// green
		if ( 6.0 * gtemp3 < 1.0 )
			green = temp1 + diff * 6.0 * gtemp3;
		else if ( 2.0 * gtemp3 < 1.0 )
			green = temp2;
		else if ( 3.0 * gtemp3 < 2.0 )
			green = temp1 + diff * ( ( 2.0 / 3.0 ) - gtemp3 ) * 6.0;
		else
			green = temp1;

		// blue
		if ( 6.0 * btemp3 < 1.0 )
			blue = temp1 + diff * 6.0 * btemp3;
		else if ( 2.0 * btemp3 < 1.0 )
			blue = temp2;
		else if ( 3.0 * btemp3 < 2.0 )
			blue = temp1 + diff * ( ( 2.0 / 3.0 ) - btemp3 ) * 6.0;
		else
			blue = temp1;
		#endif
		fRed   = (uBYTE) ( red   * 255 );
		fGreen = (uBYTE) ( green * 255 );
		fBlue  = (uBYTE) ( blue  * 255 );
	}
}

void VColor::FromWebColor( const VString& inWebColor )
{
	if ( inWebColor.GetLength() == 7 )
	{
		VString red, green, blue;

		inWebColor.GetSubString( 2, 2, red );
		inWebColor.GetSubString( 4, 2, green );
		inWebColor.GetSubString( 6, 2, blue );

		fRed   = (uBYTE) red.GetHexLong();
		fGreen = (uBYTE) green.GetHexLong();
		fBlue  = (uBYTE) blue.GetHexLong();

		fAlpha = 0xFF;
	}
}

void VColor::MakeWebSafe()
{
	fAlpha = 0xFF;

	if (fRed <= 0x19)
		fRed = 0x00;
	else if (fRed <= 0x4C)
		fRed = 0x33;
	else if (fRed <= 0x7F)
		fRed = 0x66;
	else if (fRed <= 0xB2)
		fRed = 0x99;
	else if (fRed <= 0xE5)
		fRed = 0xCC;
	else
		fRed = 0xFF;

	if (fGreen <= 0x19)
		fGreen = 0x00;
	else if (fGreen <= 0x4C)
		fGreen = 0x33;
	else if (fGreen <= 0x7F)
		fGreen = 0x66;
	else if (fGreen <= 0xB2)
		fGreen = 0x99;
	else if (fGreen <= 0xE5)
		fGreen = 0xCC;
	else
		fGreen = 0xFF;

	if (fBlue <= 0x19)
		fBlue = 0x00;
	else if (fBlue <= 0x4C)
		fBlue = 0x33;
	else if (fBlue <= 0x7F)
		fBlue = 0x66;
	else if (fBlue <= 0xB2)
		fBlue = 0x99;
	else if (fBlue <= 0xE5)
		fBlue = 0xCC;
	else
		fBlue = 0xFF;
}


void VColor::ToGrey()
{
     uBYTE	grey = (uBYTE)((GReal) 0.299 * fRed + (GReal) 0.587 * fGreen + (GReal) 0.114 * fBlue);
	 fRed = fGreen = fBlue = grey;
}


void VColor::SetLuminosity(GReal inRatio)
{
	if (inRatio < 0) return;
	
	sWORD	hue, saturation, luminosity;
	GetHSL(hue, saturation, luminosity);
	
	luminosity = (sWORD)(((GReal) luminosity) * inRatio);
	if (luminosity > 255)
	{
		saturation = (sWORD)(((GReal) saturation) / ((GReal) luminosity) * 255);
		luminosity = 255;
	}
	
	FromHSL(hue, saturation, luminosity);
}


VError VColor::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	if (inStream == NULL) return VE_INVALID_PARAMETER;

	if (inStream->GetByte(fRed) != VE_OK)
		fRed = 0;
	
	if (inStream->GetByte(fGreen) != VE_OK)
		fGreen = 0;
	
	if (inStream->GetByte(fBlue) != VE_OK)
		fBlue = 0;
	
	if (inStream->GetByte(fAlpha) !=VE_OK)
		fAlpha = 0xFF;
	
	return inStream->GetLastError();
}


VError VColor::WriteToStream(VStream* inStream, sLONG /*inParam*/) const
{
	if (inStream == NULL) return VE_INVALID_PARAMETER;
	
	inStream->PutByte(fRed);
	inStream->PutByte(fGreen);
	inStream->PutByte(fBlue);
	inStream->PutByte(fAlpha);
	
	return inStream->GetLastError();
}


VError VColor::SaveToBag(VValueBag& ioBag, VString& outKind) const
{
	ioBag.SetLong(L"red", fRed);
	ioBag.SetLong(L"green", fGreen);
	ioBag.SetLong(L"blue", fBlue);
	ioBag.SetLong(L"alpha", fAlpha);
	
	outKind = L"color";
	return VE_OK;
}


VError VColor::LoadFromBag(const VValueBag& inBag)
{
	sLONG	colorComponent;

	if (inBag.GetLong(L"red", colorComponent))
	{
		assert(colorComponent >= 0 && colorComponent <= 255);
		fRed = (uBYTE) colorComponent;
	}
	else fRed = 0;

	if (inBag.GetLong(L"green", colorComponent))
	{
		assert(colorComponent >= 0 && colorComponent <= 255);
		fGreen = (uBYTE) colorComponent;
	}
	else fGreen = 0;

	if (inBag.GetLong(L"blue", colorComponent))
	{
		assert(colorComponent >= 0 && colorComponent <= 255);
		fBlue = (uBYTE) colorComponent;
	}
	else fBlue = 0;

	if (inBag.GetLong(L"alpha", colorComponent))
	{
		assert(colorComponent >= 0 && colorComponent <= 255);
		fAlpha = (uBYTE) colorComponent;
	}
	else fAlpha = 0xFF;
	
	return VE_OK;
}


uLONG VColor::GrayToLongColor(uBYTE inGray)
{
	return (((uLONG)inGray)<<16) | (((uLONG)inGray)<<8) | ((uLONG)inGray);
}


uLONG VColor::RGBToLongColor(uBYTE inRed, uBYTE inGreen, uBYTE inBlue)
{
	return (((uLONG)inRed)<<16) | (((uLONG)inGreen)<<8) | ((uLONG)inBlue);
}


void VColor::LongColorToRGB(uLONG inRGB, uBYTE *outRed, uBYTE *outGreen, uBYTE *outBlue)
{
	*outRed = (inRGB & 0x00FF0000) >> 16;
	*outGreen = (inRGB & 0x0000FF00) >> 8;
	*outBlue = inRGB & 0x000000FF;
}


#if VERSIONWIN

void VColor::WIN_LongColorToCOLORREF(uLONG inARGB, COLORREF& outColor)
{
	outColor = inARGB;
}


uLONG VColor::WIN_COLORREFToLongColor(const COLORREF& inColor)
{
	return inColor;
}

void VColor::WIN_FromCOLORREF( COLORREF inColorRef)
{
	SetColor((uBYTE)(inColorRef)&0xFF, (uBYTE)(inColorRef>>8)&0xFF, (uBYTE)(inColorRef>>16)&0xFF);
}

COLORREF VColor::WIN_ToCOLORREF() const
{
	return (COLORREF)((fBlue<<16) | (fGreen<<8) | fRed);
}
			
void VColor::WIN_FromGdiplusColor( const Gdiplus::Color& inGdiplusColor)
{
	fAlpha = inGdiplusColor.GetAlpha();
	fRed = inGdiplusColor.GetRed();
	fGreen = inGdiplusColor.GetGreen();
	fBlue = inGdiplusColor.GetBlue();
}

Gdiplus::Color VColor::WIN_ToGdiplusColor() const
{
	return Gdiplus::Color( fAlpha, fRed, fGreen, fBlue);
}

#endif


/*
The conversion algorithms for these color spaces are originally from the book 
Fundamentals of Interactive Computer Graphics by Foley and van Dam (c 1982, Addison-Wesley).
Chapter 17 describes color spaces and shows their relationships via easy-to-follow diagrams.
NB this is a wonderful book, but if you are going to get a copy, I suggest you look for the latest edition. 

RGB - HSL
	Convert the RBG values to the range 0-1
	Example: from the video colors page, colorbar red has R=83%, B=7%, G=7%, or in this scale, R=.83, B=.07, G=.07

	Find min and max values of R, B, G
	In the example, maxcolor = .83, mincolor=.07

	L = (maxcolor + mincolor)/2 
	For the example, L = (.83+.07)/2 = .45

	If the max and min colors are the same (ie the color is some kind of grey), 
	S is defined to be 0, and H is undefined but in programs usually written as 0

	Otherwise, test L. 
	If L < 0.5, S=(maxcolor-mincolor)/(maxcolor+mincolor)
	If L >=0.5, S=(maxcolor-mincolor)/(2.0-maxcolor-mincolor)
	For the example, L=0.45 so S=(.83-.07)/(.83+.07) = .84

	If R=maxcolor, H = (G-B)/(maxcolor-mincolor)
	If G=maxcolor, H = 2.0 + (B-R)/(maxcolor-mincolor)
	If B=maxcolor, H = 4.0 + (R-G)/(maxcolor-mincolor)
	For the example, R=maxcolor so H = (.07-.07)/(.83-.07) = 0

	To use the scaling shown in the video color page, convert L and S back to percentages,
	and H into an angle in degrees (ie scale it from 0-360). From the computation in step 6,
	H will range from 0-6. RGB space is a cube, and HSL space is a double hexacone, 
	where L is the principal diagonal of the RGB cube.
	Thus corners of the RGB cube; red, yellow, green, cyan, blue, and magenta,
	become the vertices of the HSL hexagon. Then the value 0-6 for H tells you which section 
	of the hexgon you are in. H is most commonly given as in degrees, so to convert

	H = H*60.0
	If H is negative, add 360 to complete the conversion. 

HSL - RGB
	If S=0, define R, G, and B all to L

	Otherwise, test L.
	If L < 0.5, temp2=L*(1.0+S)
	If L >= 0.5, temp2=L+S - L*S
	In the colorbar example for colorbar green, H=120, L=52, S=79, 
	so converting to the range 0-1, L=.52, so
	temp2=(.52+.79) - (.52*.79) = .899

	temp1 = 2.0*L - temp2
	In the example, temp1 = 2.0*.52 - .899 = .141

	Convert H to the range 0-1
	In the example, H=120/360 = .33

	For each of R, G, B, compute another temporary value, temp3, as follows:
	for R, temp3=H+1.0/3.0
	for G, temp3=H
	for B, temp3=H-1.0/3.0
	if temp3 < 0, temp3 = temp3 + 1.0
	if temp3 > 1, temp3 = temp3 - 1.0
	In the example, Rtemp3=.33+.33 = .66, Gtemp3=.33, Btemp3=.33-.33=0

	For each of R, G, B, do the following test:
	If 6.0*temp3 < 1, color=temp1+(temp2-temp1)*6.0*temp3
	Else if 2.0*temp3 < 1, color=temp2
	Else if 3.0*temp3 < 2, color=temp1+(temp2-temp1)*((2.0/3.0)-temp3)*6.0
	Else color=temp1
	In the example,
	3.0*Rtemp3 < 2 so R=.141+(.899-.141)*((2.0/3.0-.66)*6.0=.141
	2.0*Gtemp3 < 1 so G=.899
	6.0*Btemp3 < 1 so B=.141+(.899-.141)*6.0*0=.141


	Scale back to the range 0-100 to use the scaling shown in the video color page
	For the example, R=14, G=90, B=14
*/

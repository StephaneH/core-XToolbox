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
#ifndef __VColor__
#define __VColor__

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VColor : public VObject, public IStreamable
{
public:
							VColor():fRed(0),fGreen(0),fBlue(0),fAlpha(255) {};
							VColor( uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha = 255);
							VColor( const VColor& inOriginal);
							VColor( const VColor* inOriginal);
	explicit				VColor( StdColor inStdColor)				{ FromStdColor(inStdColor); }

#if VERSIONWIN
	explicit				VColor( const Gdiplus::Color& inColor)		{ WIN_FromGdiplusColor( inColor); }
#endif

#if VERSIONMAC
	explicit				VColor( CGColorRef inColor)					{ MAC_FromCGColorRef( inColor);}
#endif

	virtual					~VColor()									{;}

	// Operators
			VColor&			operator = (const VColor &inVColor);
			bool			operator == (const VColor &inVColor) const;
			bool			operator != (const VColor &inVColor) const;
	
			void			FromStdColor (StdColor inStdColor);

			void			FromRGBAColor( uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha = 0xFF);
			void			FromRGBAColor( uLONG inColor);	// Format is 0xAARRGGBB
			uLONG			GetRGBAColor( bool inPremultiply=false) const;

			void			FromRGBColor (uLONG inColor)			{ FromRGBAColor(inColor | 0xFF000000); };
			uLONG			GetRGBColor() const						{ return ((uLONG)fBlue) | ((uLONG)fGreen) << 8 | ((uLONG)fRed) << 16;}

			// Hue: 0->360 - Saturation and Luminosity: 0->100
			void			FromHSL (sWORD inHue, sWORD inSaturation, sWORD inLuminosity);
			void			GetHSL (sWORD& outHue, sWORD& outSaturation, sWORD& outLuminosity) const;

	static	 void			Combine( const VColor& inColor1, const VColor& inColor2, VColor& outResult );

			// Accessors
			void 			SetColor( uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha = 0xFF) { fRed = inRed; fGreen = inGreen; fBlue = inBlue; fAlpha = inAlpha; }
			void			SetRed( uBYTE inRed)			{ fRed = inRed; }
			void			SetGreen( uBYTE inGreen)		{ fGreen = inGreen; }
			void			SetBlue( uBYTE inBlue)			{ fBlue = inBlue; }
			void			SetAlpha( uBYTE inAlpha)		{ fAlpha = inAlpha; }

			void			GetColor( uBYTE& outRed, uBYTE& outGreen, uBYTE& outBlue ) const	{ outRed = fRed; outGreen = fGreen; outBlue = fBlue; }
			void			GetColor( uWORD *outRed, uWORD *outGreen, uWORD *outBlue, uWORD *outAlpha = NULL) const;

			uBYTE			GetRed() const					{ return fRed; }
			uBYTE			GetGreen() const				{ return fGreen; }
			uBYTE			GetBlue() const					{ return fBlue; }
			uBYTE			GetAlpha() const				{ return fAlpha; }

			// Utilities
			void			FromWebColor( const VString& inWebColor );	// color expressed with a text expression like #CDAE07
			void			MakeWebSafe();
			void			ToGrey();
			void			SetLuminosity( GReal inRatio);

	// Stream storage
	virtual	VError			ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual	VError			WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
	virtual	VError			SaveToBag (VValueBag& ioBag, VString& outKind) const;
	virtual	VError			LoadFromBag (const VValueBag& inBag);

	// Class utilities
	static	uLONG			GrayToLongColor (uBYTE inGray);
	static	uLONG			RGBToLongColor (uBYTE inRed, uBYTE inGreen, uBYTE inBlue);
	static	void			LongColorToRGB (uLONG inRGB, uBYTE *outRed, uBYTE *outGreen, uBYTE *outBlue);


#if VERSIONMAC
			void			MAC_FromCGColorRef( CGColorRef inColor);
			CGColorRef		MAC_CreateCGColorRef() const;
#endif

#if VERSIONWIN
			void			WIN_FromCOLORREF( COLORREF inColorRef);
			COLORREF		WIN_ToCOLORREF() const;
			
			void			WIN_FromGdiplusColor( const Gdiplus::Color& inGdiplusColor);
			Gdiplus::Color	WIN_ToGdiplusColor() const;

	static	void			WIN_LongColorToCOLORREF (uLONG inRGB, COLORREF& outColor);
	static	uLONG			WIN_COLORREFToLongColor (const COLORREF& inColor);
#endif

	static	const VColor	sWhiteColor;
	static	const VColor	sBlackColor;

private:
			GReal			_MakeHue(GReal m1,GReal m2,GReal h);

			uBYTE			fRed;
			uBYTE			fGreen;
			uBYTE			fBlue;
			uBYTE			fAlpha;
};

END_TOOLBOX_NAMESPACE

#endif
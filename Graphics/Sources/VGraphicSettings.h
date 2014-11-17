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
#ifndef __VGraphicSettings__
#define __VGraphicSettings__

#include "Graphics/Sources/VColor.h"
#include "Graphics/Sources/VFont.h"
#include "Graphics/Sources/VPoint.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VPattern;


class XTOOLBOX_API VGraphicSettings : public VObject
{
public:
			VGraphicSettings ();
			VGraphicSettings (const VGraphicSettings& inOriginal);
	virtual	~VGraphicSettings ();
	
	// Pen management
	void	SetFillColor (const VColor& inColor) { fFillColor = inColor; };
	void	SetLineColor (const VColor& inColor) { fLineColor = inColor; };
	void	SetTextColor (const VColor& inColor) { fTextColor = inColor; };
	void	SetLineWidth (GReal inWidth) { fLineWidth = inWidth; };

	const VColor&	GetFillColor () const { return fFillColor; };
	const VColor&	GetLineColor () const { return fLineColor; };
	const VColor&	GetTextColor () const { return fTextColor; };
	GReal			GetLineWidth () const { return fLineWidth; };
	
	void	SetFillPattern (const VPattern* inPattern);	// inPattern is retained
	void	SetLinePattern (const VPattern* inPattern);	// inPattern is retained
	VPattern*	RetainFillPattern () const;
	VPattern*	RetainLinePattern () const;
	const VPattern*	GetFillPattern () const { return fFillPattern; };
	const VPattern*	GetLinePattern () const { return fLinePattern; };
	
	DrawingMode	SetDrawingMode (DrawingMode inMode);
	DrawingMode	GetDrawingMode () const { return fDrawingMode; };

	void	SavePenState ();
	void	RestorePenState ();
	void	NormalizePenState ();

	// Text management
	void	SetFont (VFont* inFont);	// inFont is retained
	VFont*	RetainFont () const;
	VFont*	GetFont () const { return fFont; };
	
	void	SetTextPosBy (GReal inHoriz, GReal inVert) { fTextPos.SetPosBy(inHoriz, inVert); };
	void	SetTextPosTo (const VPoint& inPos) { fTextPos = inPos; };
	void	SetTextAngle (GReal inAngle) { fTextAngle = inAngle; };
	void	GetTextPos (VPoint& outPos) const { outPos = fTextPos; };
	GReal	GetTextAngle () const { return fTextAngle; };
	
	DrawingMode	SetTextDrawingMode (DrawingMode inMode);
	DrawingMode	GetTextDrawingMode () const { return fTextDrawingMode; };

	TextRenderingMode	SetTextRenderingMode( TextRenderingMode inMode);
	TextRenderingMode	GetTextRenderingMode() const	{ return fTextRenderingMode;}
	TextMeasuringMode	SetTextMeasuringMode( TextMeasuringMode inMode);
	TextMeasuringMode	GetTextMeasuringMode() const	{ return fTextMeasuringMode;}
	
	void	SetSpaceKerning (GReal inSpaceExtra) { fSpaceExtra = inSpaceExtra; };
	GReal	GetSpaceKerning () const { return fSpaceExtra; };
	void	SetCharKerning (GReal inSpaceExtra) { fCharExtra = inSpaceExtra; };
	GReal	GetCharKerning () const { return fCharExtra; };
	void	NormalizeKerning ();

	void	SaveTextState ();
	void	RestoreTextState ();
	void	NormalizeTextState ();
	
	uBYTE	SetAlphaBlend (uBYTE inAlphaBlend);
	uBYTE	GetAlphaBlend () const { return fAlphaBlend; };

	void	SavePixelState ();
	void	RestorePixelState ();
	void	NormalizePixelState ();

	// Inherited from VObject
	VError	LoadFromBag (const VValueBag& inBag);
	VError	SaveToBag (VValueBag& ioBag, VString &outKind) const;
	VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;

protected:
	GReal				fLineWidth;			// Pen State
	VColor				fLineColor;
	VColor				fFillColor;
	VPattern*			fLinePattern;
	VPattern*			fFillPattern;
	DrawingMode			fDrawingMode;
	VColor				fTextColor;			// Text State
	VFont*				fFont;
	GReal				fSpaceExtra;
	GReal				fCharExtra;
	VPoint				fTextPos;
	GReal				fTextAngle;
	DrawingMode			fTextDrawingMode;
	TextRenderingMode	fTextRenderingMode;
	TextMeasuringMode	fTextMeasuringMode;
	VPtrStream*			fPixelState;		// Context storing
	VPtrStream*			fPenState;
	VPtrStream*			fTextState;
	uBYTE				fAlphaBlend;
	
private:
	// Private initialization
	void	_Init ();
	void	_Dispose ();
};

END_TOOLBOX_NAMESPACE

#endif
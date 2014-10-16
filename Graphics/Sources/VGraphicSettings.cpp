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
#include "VGraphicSettings.h"
#include "VPattern.h"


VGraphicSettings::VGraphicSettings()
{
	_Init();
}


VGraphicSettings::VGraphicSettings(const VGraphicSettings& inOriginal)
{
	_Init();
	
	IRefCountable::Copy(&fFont, inOriginal.fFont);
	IRefCountable::Copy(&fLinePattern, inOriginal.fLinePattern);
	IRefCountable::Copy(&fFillPattern, inOriginal.fFillPattern);
	fLineWidth = inOriginal.fLineWidth;
	fFillColor = inOriginal.fFillColor;
	fLineColor = inOriginal.fLineColor;
	fDrawingMode = inOriginal.fDrawingMode;
	fTextDrawingMode = inOriginal.fTextDrawingMode;
	fTextRenderingMode = inOriginal.fTextRenderingMode;
	fTextMeasuringMode = inOriginal.fTextMeasuringMode;
	fSpaceExtra = inOriginal.fSpaceExtra;
	fCharExtra = inOriginal.fCharExtra;
	fTextPos = inOriginal.fTextPos;
	fTextAngle = inOriginal.fTextAngle;
	fAlphaBlend = inOriginal.fAlphaBlend;
}


VGraphicSettings::~VGraphicSettings()
{
	_Dispose();
}


void VGraphicSettings::_Init()
{
	fLineWidth = 1;
	fFont = VFont::RetainStdFont(STDF_CAPTION);
	fFillPattern = VPattern::RetainStdPattern(PAT_FILL_PLAIN);
	fLinePattern = VPattern::RetainStdPattern(PAT_LINE_PLAIN);
	fFillColor.FromStdColor(COL_WHITE);
	fLineColor.FromStdColor(COL_BLACK);
	fDrawingMode = DM_NORMAL;
	fTextDrawingMode = DM_PLAIN;
	fTextRenderingMode = TRM_NORMAL;
	fTextMeasuringMode = TMM_NORMAL;
	fSpaceExtra = (GReal) 0.0;
	fCharExtra = (GReal) 0.0;
	fTextAngle = (GReal) 0.0;
	fPenState = NULL;
	fTextState = NULL;
	fPixelState = NULL;
	fAlphaBlend = 0;
}


void VGraphicSettings::_Dispose()
{
	delete fPenState;
	delete fTextState;
	delete fPixelState;
	
	fFont->Release();
	fFillPattern->Release();	// sc 16/05/2007 memory leaks
	fLinePattern->Release();	// sc 16/05/2007 memory leaks
}


#pragma mark -

void VGraphicSettings::SetFillPattern (const VPattern* inPattern)
{
	if (inPattern == NULL)
	{
		VPattern*	defaultPattern = VPattern::RetainStdPattern(PAT_FILL_PLAIN);
		
		fFillPattern->Release();
		fFillPattern = defaultPattern;
	}
	else
	{
		IRefCountable::Copy(&fFillPattern, const_cast<VPattern*>(inPattern));
	}
}


void VGraphicSettings::SetLinePattern (const VPattern* inPattern)
{
	if (inPattern == NULL)
	{
		VPattern*	defaultPattern = VPattern::RetainStdPattern(PAT_LINE_PLAIN);
		
		fLinePattern->Release();
		fLinePattern = defaultPattern;
	}
	else
	{
		IRefCountable::Copy(&fLinePattern, const_cast<VPattern*>(inPattern));
	}
}


VPattern* VGraphicSettings::RetainFillPattern () const
{
	fFillPattern->Retain();		// sc 16/05/2007 was Release()
	return fFillPattern;
}


VPattern* VGraphicSettings::RetainLinePattern () const
{
	fLinePattern->Retain();		// sc 16/05/2007 was Release()
	return fLinePattern;
}

	
DrawingMode VGraphicSettings::SetDrawingMode(DrawingMode inMode)
{
	DrawingMode	oldMode = fDrawingMode;
	fDrawingMode = inMode;
	return oldMode;
}


void VGraphicSettings::SavePenState()
{
	if (fPenState == NULL)
		fPenState = new VPtrStream;
	
	if (fPenState != NULL)
	{
		fPenState->OpenWriting();
		
		fPenState->OpenWriting();
		fPenState->PutReal(fLineWidth);
		fLineColor.WriteToStream(fPenState);
		fFillColor.WriteToStream(fPenState);
		fPenState->PutLong(fLinePattern->GetKind());
		fLinePattern->WriteToStream(fPenState);
		fPenState->PutLong(fFillPattern->GetKind());
		fFillPattern->WriteToStream(fPenState);
		fPenState->PutLong(fDrawingMode);
		fPenState->CloseWriting();
		
		xbox_assert(fPenState->GetLastError() == VE_OK);
	}
}


void VGraphicSettings::RestorePenState()
{
	if (fPenState != NULL)
	{
		fLinePattern->Release();
		fFillPattern->Release();
		
		fPenState->OpenReading();
		fLineWidth = (GReal) fPenState->GetReal();
		fLineColor.ReadFromStream(fPenState);
		fFillColor.ReadFromStream(fPenState);
		fLinePattern = VPattern::RetainPatternFromStream(fPenState, fPenState->GetLong());
		fFillPattern = VPattern::RetainPatternFromStream(fPenState, fPenState->GetLong());
		fDrawingMode = (DrawingMode) fPenState->GetLong();
		fPenState->CloseReading();
		
		xbox_assert(fPenState->GetLastError() == VE_OK);
	}
}


void VGraphicSettings::NormalizePenState()
{
	fLinePattern->Release();
	fFillPattern->Release();
	
	fLineWidth = (GReal) 1.0;
	fLineColor.FromStdColor(COL_BLACK);
	fFillColor.FromStdColor(COL_WHITE);
	fLinePattern = VPattern::RetainStdPattern(PAT_LINE_PLAIN);
	fFillPattern = VPattern::RetainStdPattern(PAT_FILL_PLAIN);
	fDrawingMode = DM_NORMAL;
}


#pragma mark -

void VGraphicSettings::SetFont(VFont* inFont)
{
	if (inFont == NULL)
	{
		VFont*	defaultFont = VFont::RetainStdFont(STDF_CAPTION);
		
		fFont->Release();
		fFont = defaultFont;
	}
	else
	{
		IRefCountable::Copy(&fFont, inFont);
	}
}


VFont* VGraphicSettings::RetainFont() const
{
	fFont->Retain();
	return fFont;
}


DrawingMode VGraphicSettings::SetTextDrawingMode(DrawingMode inMode)
{
	DrawingMode	oldMode = fTextDrawingMode;
	fTextDrawingMode = inMode;
	return oldMode;
}


TextRenderingMode VGraphicSettings::SetTextRenderingMode( TextRenderingMode inMode)
{
	TextRenderingMode	oldMode = fTextRenderingMode;
	fTextRenderingMode = inMode;
	return oldMode;
}

TextMeasuringMode VGraphicSettings::SetTextMeasuringMode( TextMeasuringMode inMode)
{
	TextMeasuringMode	oldMode = fTextMeasuringMode;
	fTextMeasuringMode = inMode;
	return oldMode;
}


void VGraphicSettings::NormalizeKerning ()
{
	fSpaceExtra = (GReal) 0.0;
	fCharExtra = (GReal) 0.0;
}


void VGraphicSettings::SaveTextState()
{
	if (fTextState == NULL)
		fTextState = new VPtrStream;
	
	if (fTextState != NULL)
	{
		fTextState->OpenWriting();
		fTextColor.WriteToStream(fTextState);
		fFont->WriteToStream(fTextState);
		fTextState->PutReal(fSpaceExtra);
		fTextState->PutReal(fCharExtra);
		fTextState->PutReal(fTextPos.GetX());
		fTextState->PutReal(fTextPos.GetY());
		fTextState->PutReal(fTextAngle);
		fTextState->PutLong(fTextDrawingMode);
		fTextState->PutLong(fTextRenderingMode);
		//fTextState->PutLong(fTextMeasuringMode);
		fTextState->CloseWriting();
		
		xbox_assert(fTextState->GetLastError() == VE_OK);
	}
}


void VGraphicSettings::RestoreTextState()
{
	if (fTextState != NULL)
	{
		fFont->Release();
		
		fTextState->OpenReading();
		fTextColor.ReadFromStream(fTextState);
		fFont = VFont::RetainFontFromStream(fTextState);
		fSpaceExtra = (GReal) fTextState->GetReal();
		fCharExtra = (GReal) fTextState->GetReal();
		fTextPos.SetX((GReal) fTextState->GetReal());
		fTextPos.SetY((GReal) fTextState->GetReal());
		fTextAngle = (GReal) fTextState->GetReal();
		fTextDrawingMode = (DrawingMode) fTextState->GetLong();
		fTextRenderingMode = (TextRenderingMode) fTextState->GetLong();
		//fTextMeasuringMode = (TextMeasuringMode) fTextState->GetLong();
		fTextState->CloseReading();
		
		xbox_assert(fTextState->GetLastError() == VE_OK);
	}
}


void VGraphicSettings::NormalizeTextState()
{
	fTextColor.FromStdColor(COL_BLACK);
	fSpaceExtra = (GReal) 0.0;
	fCharExtra = (GReal) 0.0;
	fTextAngle = (GReal) 0.0;
	fTextDrawingMode = DM_PLAIN;
	fTextRenderingMode = TRM_NORMAL;
	fTextMeasuringMode = TMM_NORMAL;
	SetFont(NULL);
}


#pragma mark -


uBYTE VGraphicSettings::SetAlphaBlend(uBYTE inAlphaBlend)
{
	uBYTE	oldBlend = fAlphaBlend;
	fAlphaBlend = inAlphaBlend;
	return oldBlend;
}


void VGraphicSettings::NormalizePixelState()
{
	fAlphaBlend = 0;
}


void VGraphicSettings::SavePixelState()
{
	if (fPixelState == NULL)
		fPixelState = new VPtrStream;
	
	if (fPixelState != NULL)
	{
		fPixelState->OpenWriting();
		fPixelState->PutByte(fAlphaBlend);
		fPixelState->PutLong(TM_COPY);
		fPixelState->CloseWriting();
		
		xbox_assert(fPixelState->GetLastError() == VE_OK);
	}
}


void VGraphicSettings::RestorePixelState()
{
	if (fPixelState != NULL)
	{
		fPixelState->OpenReading();
		fAlphaBlend = fPixelState->GetByte();
		fPixelState->CloseReading();
		
		xbox_assert(fPixelState->GetLastError() == VE_OK);
	}
}


#pragma mark-

VError VGraphicSettings::SaveToBag(VValueBag& ioBag, VString &outKind) const
{	
	// Fill color specs Bags
	VStr63	tempKind;
	
	VValueBag*	fillSpec = new VValueBag();
	VValueBag*	colorSpec = new VValueBag();
	fFillColor.SaveToBag(*fillSpec, tempKind);
	colorSpec->SetString(L"kind", VString(L"fill"));
	colorSpec->AddElement(L"color", fillSpec);
	ioBag.AddElement(L"color_spec", colorSpec);
	colorSpec->Release();
	fillSpec->Release();
	
	VValueBag*	lineSpec = new VValueBag();
	colorSpec = new VValueBag();
	fLineColor.SaveToBag(*lineSpec, tempKind);
	colorSpec->SetString(L"kind", VString(L"line"));
	colorSpec->AddElement(L"color", lineSpec);
	ioBag.AddElement(L"color_spec", colorSpec);
	colorSpec->Release();
	lineSpec->Release();
	
	VValueBag*	textSpec = new VValueBag();
	colorSpec = new VValueBag();
	fTextColor.SaveToBag(*textSpec, tempKind);
	colorSpec->SetString(L"kind", VString(L"text"));
	colorSpec->AddElement(L"color", textSpec);
	ioBag.AddElement(L"color_spec", colorSpec);
	colorSpec->Release();
	textSpec->Release();

	// Create font spec
	VValueBag*	fontSpec = new VValueBag();
	fontSpec->SetString(L"name", fFont->GetName());
	fontSpec->SetLong(L"size", (sLONG)fFont->GetSize());

	VFontFace	face = fFont->GetFace();
	fontSpec->SetBoolean(L"bold", (face & KFS_BOLD) != 0);
	fontSpec->SetBoolean(L"italic", (face & KFS_ITALIC) != 0);
	fontSpec->SetBoolean(L"underline", (face & KFS_UNDERLINE) != 0);
	fontSpec->SetBoolean(L"condensed", (face & KFS_CONDENSED) != 0);
	fontSpec->SetBoolean(L"extended", (face & KFS_EXTENDED) != 0);
	fontSpec->SetBoolean(L"shadow", (face & KFS_SHADOW) != 0);
	fontSpec->SetBoolean(L"outline", (face & KFS_OUTLINE) != 0);
	fontSpec->SetBoolean(L"strikeout", (face & KFS_STRIKEOUT) != 0);
	
	// Save font spec
	ioBag.AddElement(L"font_spec", fontSpec);
	fontSpec->Release();
	
	// Save attributes
	//ioBag.SetBoolean(L"line_witdh", );
	//ioBag.SetBoolean(L"space_extra", );
	//ioBag.SetBoolean(L"char_extra", );
	//ioBag.SetBoolean(L"text_pos", );
	//ioBag.SetBoolean(L"text_angle", );
	//ioBag.SetBoolean(L"text_transfert", );
	//ioBag.SetBoolean(L"pixel_transfert", );
	//ioBag.SetBoolean(L"pixel_forecolor", );
	//ioBag.SetBoolean(L"pixel_backcolor", );
	//ioBag.SetBoolean(L"pixel_alpha", );
	
	outKind = L"graphic_settings";
	return VE_OK;
}


VError VGraphicSettings::LoadFromBag(const VValueBag& inBag)
{	
	// Load colors
	VStr255	tempKind;
	const VBagArray*	colorSpecs = inBag.RetainElements(L"color_spec");
	
	for (sLONG index = colorSpecs->GetCount(); index > 0; index--)
	{
		const VValueBag*	curSpec = colorSpecs->RetainNth(index);
		if (curSpec->GetString(L"kind", tempKind))
		{
			const VValueBag*	colorBag = curSpec->RetainUniqueElement(L"color");
			if (testAssert(colorBag != NULL))
			{
				VColor color;
				color.LoadFromBag(*colorBag);
				
				if (tempKind == L"fill")
					SetFillColor(color);
				else if (tempKind == L"line")
					SetLineColor(color);
				else if (tempKind == L"text")
					SetTextColor(color);
					
				colorBag->Release();
			}
		}
		
		curSpec->Release();
	}
	
	colorSpecs->Release();

	// Load font spec
	const VValueBag*	fontSpec = inBag.RetainUniqueElement(L"font_spec");
	if (fontSpec != NULL)
	{
	#if VERSIONWIN
		const wchar_t*	skipTag = L"mac_only";
	#else
		const wchar_t*	skipTag = L"win_only";
	#endif

		Boolean	flag;
		if (!fontSpec->GetBoolean(skipTag, flag) || !flag)
		{
			// Font name
			if (!fontSpec->GetString(L"name", tempKind))
				tempKind = fFont->GetName();

			// Font size
			sLONG size;
			if (!fontSpec->GetLong(L"size", size))
				size = (sLONG)fFont->GetSize();
			
			// Font face
			Boolean	setStyle;
			VFontFace	style = KFS_NORMAL;
			
			if (fontSpec->GetBoolean(L"bold", setStyle) && setStyle)
				style |= KFS_BOLD;
				
			if (fontSpec->GetBoolean(L"italic", setStyle) && setStyle)
				style |= KFS_ITALIC;
				
			if (fontSpec->GetBoolean(L"underline", setStyle) && setStyle)
				style |= KFS_UNDERLINE;
				
			if (fontSpec->GetBoolean(L"condensed", setStyle) && setStyle)
				style |= KFS_CONDENSED;
				
			if (fontSpec->GetBoolean(L"extended", setStyle) && setStyle)
				style |= KFS_EXTENDED;
				
			if (fontSpec->GetBoolean(L"shadow", setStyle) && setStyle)
				style |= KFS_SHADOW;
				
			if (fontSpec->GetBoolean(L"outline", setStyle) && setStyle)
				style |= KFS_OUTLINE;
				
			if (fontSpec->GetBoolean(L"strikeout", setStyle) && setStyle)
				style |= KFS_STRIKEOUT;
			
			VFont*	theFont = VFont::RetainFont(tempKind, style, size);
			SetFont(theFont);
			theFont->Release();
		}
		
		fontSpec->Release();
	}
	
	// Load attributes
	
	return VE_OK;
}


VError VGraphicSettings::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{	
	// See pen/text/pixel storage procs
	ioStream->PutReal(fLineWidth);
	fLineColor.WriteToStream(ioStream);
	fFillColor.WriteToStream(ioStream);
	fTextColor.WriteToStream(ioStream);
	ioStream->PutLong(fLinePattern->GetKind());
	fLinePattern->WriteToStream(ioStream);
	ioStream->PutLong(fFillPattern->GetKind());
	fFillPattern->WriteToStream(ioStream);
	fFont->WriteToStream(ioStream);
	ioStream->PutReal(fSpaceExtra);
	ioStream->PutReal(fCharExtra);
	ioStream->PutReal(fTextPos.GetX());
	ioStream->PutReal(fTextPos.GetY());
	ioStream->PutReal(fTextAngle);
	
	ioStream->PutLong(fDrawingMode);
	ioStream->PutLong(fTextDrawingMode);
	ioStream->PutLong(fTextRenderingMode);
	//ioStream->PutLong(fTextMeasuringMode);
	
	//obsolete transfer mode
	ioStream->PutLong(0);
	
	ioStream->PutByte(fAlphaBlend);
	
	return ioStream->GetLastError();
}


VError VGraphicSettings::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	fFont->Release();
	fLinePattern->Release();
	fFillPattern->Release();
		
	// See pen/text/pixel storage procs
	fLineWidth = (GReal) ioStream->GetReal();
	fLineColor.ReadFromStream(ioStream);
	fFillColor.ReadFromStream(ioStream);
	fTextColor.ReadFromStream(ioStream);
	fLinePattern = VPattern::RetainPatternFromStream(ioStream, ioStream->GetLong());
	fFillPattern = VPattern::RetainPatternFromStream(ioStream, ioStream->GetLong());
	fFont = VFont::RetainFontFromStream(ioStream);
	fSpaceExtra = (GReal) ioStream->GetReal();
	fCharExtra = (GReal) ioStream->GetReal();
	fTextPos.SetX((GReal) ioStream->GetReal());
	fTextPos.SetY((GReal) ioStream->GetReal());
	fTextAngle = (GReal) ioStream->GetReal();
	
	fDrawingMode = (DrawingMode) ioStream->GetLong();
	fTextDrawingMode = (DrawingMode) ioStream->GetLong();
	fTextRenderingMode = (TextRenderingMode) ioStream->GetLong();
	//fTextMeasuringMode = (TextMeasuringMode) ioStream->GetLong();
	
	//for compat with obsolete transfer mode
	sLONG dummy = ioStream->GetLong();

	fAlphaBlend = ioStream->GetByte();
	
	return ioStream->GetLastError();
}

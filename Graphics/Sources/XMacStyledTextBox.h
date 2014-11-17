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
#ifndef __XMACSTYLEDTEXTBOX_H_
#define __XMACSTYLEDTEXTBOX_H_

#include "VStyledTextBox.h"

BEGIN_TOOLBOX_NAMESPACE

class XMacStyledTextBox : public VStyledTextBox
{
public:
	XMacStyledTextBox(ContextRef inContextRef, const VString& inText, const VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont, const TextRenderingMode inMode = TRM_NORMAL, const TextLayoutMode inLayoutMode = TLM_NORMAL, const GReal inCharKerning = 0.0f, const GReal inRefDocDPI = 72.0f, bool inUseNSAttributes = false);
	
	CFMutableAttributedStringRef GetAttributedString()
	{
		return fAttributedString;
	}

	CTFrameRef GetCurLayoutFrame(GReal inMaxWidth, GReal inMaxHeight = 100000.0f)
	{
		_ComputeCurFrame( inMaxWidth, inMaxHeight);
		return fCurLayoutFrame;
	}

	CTFrameRef GetCurLayoutFrame()
	{
		_ComputeCurFrame( fCurLayoutWidth, fCurLayoutHeight);
		return fCurLayoutFrame;
	}

	/** get plain text */
	virtual Boolean GetPlainText( VString& outText) const;
	
	/** get uniform style on the specified range 
	@remarks
		return end location of uniform range
		
		derived from VStyledTextBox
	*/
	sLONG GetStyle(VTextStyle* ioStyle, sLONG rangeStart = 0, sLONG rangeEnd = -1);
	
	Boolean GetRTFText(/*[out, retval]*/ VString& outRTFText);
	Boolean	SetRTFText(/*[in]*/const VString& inRTFText);
	
	//on Mac, we can set/get directly the RTF or HTML stream
	
	Boolean	SetHTMLText(VHandle inHandleHTML);
	
	VHandle GetRTFText();
	Boolean	SetRTFText(VHandle inHandleRTF);
	
	/** insert text at the specified position */
	void InsertPlainText( sLONG inPos, const VString& inText);

	/** delete text range */
	void DeletePlainText( sLONG rangeStart = 0, sLONG rangeEnd = -1);
	
	/** replace text range */
	void ReplacePlainText( sLONG rangeStart, sLONG rangeEnd, const VString& inText);
	
	/** set paragraph line height
     
     if positive, it is fixed line height in point
     if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
     if -1, it is normal line height
     */
	void SetLineHeight( const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1);
    
	/** apply style (use style range) */
	void ApplyStyle( const VTextStyle* inStyle);
	
	/** change drawing context 
	@remarks
		by default, drawing context is the one used to initialize instance

		you can draw to another context but it assumes new drawing context is compatible with the context used to initialize the text box
		(it is caller responsibility to check new context compatibility: normally it is necessary only on Windows because of GDI resolution dependent DPI)
	*/
	void SetDrawContext( ContextRef inContextRef)
	{
		fContextRef = inContextRef;
	}
	
	
	/** return adjusted layout origin 
	@remarks
	    CoreText layout size might not be the same as the design layout size (due to the workaround for CoreText max width bug)
		so caller should call this method while requesting for instance for character position
		and adjust character position as determined from the CoreText layout with this offset
	    (actually only Quartz graphic context metrics method might need to call this method for consistency with gc user space) 
	*/
	const VPoint& GetLayoutOffset() const;
	
	/* get number of lines */
	sLONG GetNumberOfLines();

protected:
	Boolean DoInitialize();
	void DoRelease();
	Boolean DoDraw(const VRect& inBounds);
	void DoGetSize(GReal &outWidth, GReal &outHeight,bool inForPrinting=false);
	void DoApplyStyle(const VTextStyle* inStyle, VTextStyle *inStyleInherit = NULL);
	void _SetFont(VFont *font, const VColor &textColor);
	void _SetText(const VString& inText);
	bool CFDictionaryFromVTextStyle(CFDictionaryRef inActualAttributes, CFMutableDictionaryRef inNewAttributes, const VTextStyle* inStyle, VTextStyle *inStyleInherit = NULL);

	/** set document reference DPI
	@remarks
		by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
		but you can override it if needed
	*/
	void _SetDocumentDPI( const GReal inDPI);

	/** compute current layout frame according to the specified max width & max height */
	void _ComputeCurFrame(GReal inMaxWidth = 100000.0f, GReal inMaxHeight = 100000.0f);

	void _ComputeCurFrame2(GReal inMaxWidth = 100000.0f, GReal inMaxHeight = 100000.0f, bool inForceComputeStrikeoutLines = false);
	
	/** ratio to apply to fontsize in order to take account reference document DPI
	@remarks
		by default it is set to 1.0f : set it to document DPI/72.0f if document DPI is not equal to 72.0f
	*/
	GReal fFontSizeToDocumentFontSize;
	
private:
	virtual ~XMacStyledTextBox();

	CFMutableAttributedStringRef fAttributedString;
	bool fUseNSAttributes;
	ContextRef fContextRef;

	/** cached layout impl */
	TextLayoutMode fCurLayoutMode;
	CTFrameRef fCurLayoutFrame;
	GReal fCurLayoutWidth, fCurLayoutHeight;
	GReal fCurTypoWidth, fCurTypoHeight;
	bool fCurLayoutIsDirty;
    VFont *fCurFont;

	sLONG fNumLine;

	typedef struct
	{
		VPoint fStart;
		VPoint fEnd;
		GReal fWidth;
		VColor fColor;
		bool fSetColor;
	} StrikeOutLine;
	
	typedef std::vector<StrikeOutLine> VectorOfStrikeOutLine;
	
	VectorOfStrikeOutLine fCurLayoutStrikeoutLines;
	
	eStyleJust fCurJustUniform;
	VPoint fCurLayoutOffset;
    
    bool fIsMonostyle;
};

END_TOOLBOX_NAMESPACE

#endif //__XMACSTYLEDTEXTBOX_H_

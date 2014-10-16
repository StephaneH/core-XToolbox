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
#define UNICODE
#include "vfont.h"
#include "vcolor.h"
#include <windows.h>
#include <stddef.h>
#include <windowsx.h>
#include <richedit.h>
#include <textserv.h>
#include <Tom.h>
#include <tchar.h>
#include "VRect.h"
#include "XWinStyledTextBox.h"
#include "XWinGDIGraphicContext.h"

//ensure we can use std::min & std::max
#undef min
#undef max
#include <stdio.h>

EXTERN_C const IID IID_ITextServices = { // 8d33f740-cf58-11ce-a89d-00aa006cadc5
    0x8d33f740,
    0xcf58,
    0x11ce,
    {0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5}
  };
EXTERN_C const IID IID_ITextHost = { // {D764B0BB-39E9-4307-9CA7-707FB2C1B689}
    0xd764b0bb,
    0x39e9,
    0x4307,
    { 0x9c, 0xa7, 0x70, 0x7f, 0xb2, 0xc1, 0xb6, 0x89 }
  };
EXTERN_C const IID IID_ITextDocument = {
	0x8CC497C0,
	0xA1DF,
	0x11CE,
	{0x80, 0x98, 0x00, 0xAA, 0x00, 0x47, 0xBE, 0x5D}
};
class xBSTR
{
	public:
	xBSTR()
	{
		fString =NULL;
	}
	xBSTR(const XBOX::VString& inStr)
	{
		if(fString)
			fString = SysAllocString(inStr.GetCPointer());
	}
	~xBSTR()
	{
		SysFreeString(fString);
	}
	void ToVString(XBOX::VString& outStr)
	{
		outStr.Clear();
		if(fString)
			outStr.FromUniCString(fString);
	}
	operator BSTR()
	{
		return fString;
	}
	operator BSTR*()
	{
		return &fString;
	}
	BSTR fString;
};

static GReal gOffsetX = 0.0f;

static bool gIsIdeographicLanguageInitialized = false;
static bool gIsIdeographicLanguage = false;


// HIMETRIC units per inch (used for conversion)
#define HIMETRIC_PER_INCH 2540.0

// Convert Himetric along the X axis to X pixels
LONG HimetricXtoDX(LONG xHimetric, LONG xPerInch)
{
	return (LONG) MulDiv(xHimetric, xPerInch, HIMETRIC_PER_INCH);
}

// Convert Himetric along the Y axis to Y pixels
LONG HimetricYtoDY(LONG yHimetric, LONG yPerInch)
{
	return (LONG) MulDiv(yHimetric, yPerInch, HIMETRIC_PER_INCH);
}

// Convert Pixels on the X axis to Himetric
LONG DXtoHimetricX(LONG dx, LONG xPerInch)
{
	return (LONG) MulDiv(dx, HIMETRIC_PER_INCH, xPerInch);
}

// Convert Pixels on the Y axis to Himetric
LONG DYtoHimetricY(LONG dy, LONG yPerInch)
{
	return (LONG) MulDiv(dy, HIMETRIC_PER_INCH, yPerInch);
}

/////////////////////////////////////////////////////////////////////////////
// CallBack functions

DWORD CALLBACK EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StreamCookie* pCookie = (StreamCookie*) dwCookie;

	if(pCookie->fSize - pCookie->fCount < (DWORD) cb)
		*pcb = pCookie->fSize - pCookie->fCount;
	else
		*pcb = cb;
	CopyMemory(pbBuff, ((char *)(pCookie->fText))+pCookie->fCount, *pcb);
	pCookie->fCount += *pcb;

	return 0;	//	callback succeeded - no errors
}

DWORD CALLBACK EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StreamCookie* pCookie = (StreamCookie*) dwCookie;

	pCookie->fSize += cb;
	*pcb = cb;

	char * ptr = (char*)(pCookie->fText);
	if(pCookie->fText)
		CopyMemory(ptr+pCookie->fCount, pbBuff, *pcb);
	pCookie->fCount += cb;
	return 0;	//	callback succeeded - no errors
}

USING_TOOLBOX_NAMESPACE

static bool IsPrinterDC(const HDC inDC)
{
	if (!inDC)
		return false;
	int devicetechno = ::GetDeviceCaps( inDC, TECHNOLOGY );
	return (devicetechno == DT_RASPRINTER || devicetechno == DT_PLOTTER) 	;
}

/////////////////////////////////////////////////////////////////////////////
// XWinStyledTextBox
XWinStyledTextBox::XWinStyledTextBox(HDC inHDC, HDC inOutputHDC, const VString& inText, 
									 const VTreeTextStyle *inStyles, 
									 const VRect& inHwndBounds, 
									 const VColor& inTextColor, 
									 VFont *inFont, 
									 const TextLayoutMode inLayoutMode, 
									 const GReal inRefDocDPI, 
									 const VColor& inParentFrameBackColor,
									 bool inShouldPaintCharBackColor)
: fHDC(inHDC), fHDCMetrics(NULL), VStyledTextBox(inText, inStyles, inHwndBounds, inTextColor, inFont)
{
	fOutputRefDC = inOutputHDC;
	fIsPrinting = IsPrinterDC( inHDC);
	if (!fIsPrinting && fOutputRefDC && fOutputRefDC != inHDC)
		fIsPrinting = IsPrinterDC( fOutputRefDC);

	fShouldPaintCharBackColor = inShouldPaintCharBackColor;
	fParentFrameBackColor = inParentFrameBackColor;
	fIsSizeDirty = true;
	fCurWidth = fCurHeight = 0;
	fCurDPI = 0;	
	fIsFixedLineHeight = false;
	fShouldResetLayout = false;
	fCurFont = NULL;
	fInitDone = false;

	fSelStart = fSelEnd = 0;
	fSelActive = true;

	fInplaceActive = false;

	VRect bounds(inHwndBounds);
	bounds.NormalizeToInt(true); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	RECT rt = bounds;

	SetDrawContext(inHDC);

	//by default set reference DPI to 4D form DPI 
	_SetDocumentDPI(inRefDocDPI);

	fLayoutMode = inLayoutMode;
	Initialize(inText, inStyles, inTextColor, inFont);

	fClientRect = rt;

	_ComputeOffsetX();

	bounds.SetPosBy(fOffsetX, 0.0f);
	bounds.SetSizeBy(-fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)
	rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	fInitDone = true;
}

XWinStyledTextBox::~XWinStyledTextBox()
{
	ReleaseRefCountable(&fCurFont);
}

HRESULT	XWinStyledTextBox::OnTxInPlaceDeactivate()
{
	HRESULT hr = fTextServices->OnTxInPlaceDeactivate();

	if (SUCCEEDED(hr))
	{
		fInplaceActive = FALSE;
	}

	return hr;
}

HRESULT	XWinStyledTextBox::OnTxInPlaceActivate(LPCRECT prcClient)
{
	fInplaceActive = TRUE;

	HRESULT hr = fTextServices->OnTxInPlaceActivate(prcClient);

	if (FAILED(hr))
	{
		fInplaceActive = FALSE;
	}

	return hr;
}

/** change drawing context 
@remarks
	by default, drawing context is the one used to initialize instance

	you can draw to another context but it assumes new drawing context is compatible with the context used to initialize the text box
	(it is caller responsibility to check new context compatibility: normally it is necessary only on Windows because of GDI resolution dependent DPI)
*/
void XWinStyledTextBox::SetDrawContext( ContextRef inContextRef)
{
	sLONG oldDPI = fCurDPI;
	fHDC = inContextRef;
	if (!fHDC)
	{
		if (!fHDCMetrics)
			fHDCMetrics = ::CreateCompatibleDC( NULL); //use screen compatible dc (only valid for metrics: fonts metrics are scaled internally from screen dpi to document dpi - 72 on default for 4D form compat)
		fHDC = fHDCMetrics;
	}
	nPixelsPerInchX = GetDeviceCaps(fHDC, LOGPIXELSX);
	nPixelsPerInchY = GetDeviceCaps(fHDC, LOGPIXELSY);

	fCurDPI = nPixelsPerInchY;
	fIsSizeDirty = fIsSizeDirty || oldDPI != fCurDPI;
}

/** compute x offset
@remarks
	see fOffsetX comment
*/
void XWinStyledTextBox::_ComputeOffsetX()
{
	if (::GetDeviceCaps(fHDC, TECHNOLOGY) != DT_RASDISPLAY || fHAlign == AL_RIGHT || fHAlign == AL_CENTER)
	{
		if (gOffsetX)
			fOffsetX = gOffsetX;
		else
			fOffsetX = -4.0f; //it is default RTE offset (determined experimentally)
		return;
	}
	if (!gOffsetX)
	{
		VRect bounds(fClientRect.left, fClientRect.top, fClientRect.right-fClientRect.left, fClientRect.bottom-fClientRect.top);
		if (bounds.GetWidth() == 0.0f)
			bounds.SetWidth(100.0f);
		if (bounds.GetHeight() == 0.0f)
			bounds.SetHeight(100.0f);
		VPoint pos;
		GReal height;
		fOffsetX = 0.0f;
		GetCaretMetricsFromCharIndex(bounds, 0, pos, height); 
		gOffsetX = -(pos.GetX()-fClientRect.left);
	}
	fOffsetX = gOffsetX;
}


/** set document reference DPI
@remarks
	by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
	but you can override it if needed
*/
void XWinStyledTextBox::_SetDocumentDPI( const GReal inDPI)
{
	fFontSizeToDocumentFontSize = inDPI / nPixelsPerInchY;
}

Boolean XWinStyledTextBox::GetPlainText( VString& outText) const
{
	if (!fIsPlainTextDirty || !fTextServices)
	{
		outText.FromString(fText);
		return TRUE;
	}
	
	fIsPlainTextDirty = false;

	BSTR bStr = NULL;
	fTextServices->TxGetText(&bStr);
	LONG len = ::SysStringLen(bStr);
	if (!bStr)
	{
		fText.SetEmpty();
		outText.SetEmpty();
		return FALSE;
	}

	fText.FromUniCString( bStr);
	fText.ExchangeAll(0x0B,0x0D);

	outText.FromString(fText);
	::SysFreeString( bStr);

	return TRUE;
}

/** get plain text */
const VString& XWinStyledTextBox::_GetPlainText() const
{
	if (!fIsPlainTextDirty || !fTextServices)
		return fText;

	fIsPlainTextDirty = false;

	BSTR bStr = NULL;
	fTextServices->TxGetText(&bStr);
	LONG len = ::SysStringLen(bStr);
	if (!bStr)
	{
		fText.SetEmpty();
		return fText;
	}

	fText.FromUniCString( bStr);
	fText.ExchangeAll(0x0B,0x0D);

	::SysFreeString( bStr);

	return fText;
}

/**
*  Tests whether the given character is a valid space.
*/
static bool isSpace( unsigned char c )
{
  if (c == 32)
	  return true;
  return ( c < 32 ) &&
        ( ( ( ( ( 1L << '\t' ) |
        ( 1L << '\n' ) |
        ( 1L << '\r' ) |
        ( 1L << '\f' ) ) >> c ) & 1L ) != 0 );
}

Boolean XWinStyledTextBox::GetRTFText(VString& outRTFText)
{
	HRESULT hr;
	LRESULT lResult = 0;
	EDITSTREAM editStream;

	if (!fTextServices) 
		return FALSE;

	//get size of RTF text
	fStreamCookie.fText = NULL;
	fStreamCookie.fSize = 0;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamOutCallback;
	hr = fTextServices->TxSendMessage(EM_STREAMOUT, (WPARAM)SF_RTF, (LPARAM)&editStream, &lResult );
	if (FAILED(hr))
		return FALSE;

	//stream out RTF text
	fStreamCookie.fText = (BSTR)malloc(fStreamCookie.fSize+2);
	if (!fStreamCookie.fText)
		return FALSE;

	memset(fStreamCookie.fText, 0, fStreamCookie.fSize+2);
	fStreamCookie.fSize = 0;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamOutCallback;
	lResult = 0;
	hr = fTextServices->TxSendMessage(EM_STREAMOUT, (WPARAM)SF_RTF, (LPARAM)&editStream, &lResult);
	if (hr == S_OK)
	{
		//JQ: workaround for buggy EM_STREAMOUT: even without setting SFF_WRITEXTRAPAR, 
		//	  calling EM_STREAMOUT with SF_RTF adds a extra \fs?? & \par each time we call EM_STREAMOUT which is quite nasty...
		//
		//    the only workaround i have found is to trim extra \par & \fs at the end 

		//prepare reverse parsing
		unsigned char *cStart = (unsigned char *)((char *)fStreamCookie.fText);
		unsigned char *c = cStart+strlen((char *)fStreamCookie.fText);
		unsigned char *cPar = c;
		unsigned char *cParNext = c;

		int numPar = 0;

		c--;
		//skip ending spaces
		while (c >= cStart && isSpace( *c))
			c--;
		if (c >= cStart && *c == '}') //skip end bracket
		{
			while(true)
			{
				//skip ending spaces
				c--;
				while (c >= cStart && isSpace( *c))
					c--;
				if (c < cStart || *c == '{' || *c == '}')
					break;
				if (c-3 >= cStart
					&&
					*(c-3) == '\\'
					&&
					*(c-2) == 'p'
					&&
					*(c-1) == 'a'
					&&
					*(c) == 'r'
					)
				{
					//trim only last \par
					if (numPar)
						break;
					else
					{
						numPar++;
						cParNext = cPar;
						cPar = c-3;
						c = c-3;
					}
				}
				else if (c-4 >= cStart
						&&
						*(c-4) == '\\'
						&&
						*(c-3) == 'f'
						&&
						*(c-2) == 's'
						&&
						*(c-1) >= '0' && *(c-1) <= '9'
						&&
						*(c-0) >= '0' && *(c-0) <= '9'
						)
					 {
						//trim \fs?? tag
						cParNext = c-4;
						cPar = c-4;
						c = c-4;
					 }
					 else if	(c-5 >= cStart
								&&
								*(c-5) == '\\'
								&&
								*(c-4) == 'f'
								&&
								*(c-3) == 's'
								&&
								*(c-2) >= '0' && *(c-2) <= '9'
								&&
								*(c-1) >= '0' && *(c-1) <= '9'
								&&
								*(c-0) >= '0' && *(c-0) <= '9'
								)
								{
									//trim \fs??? tag
									cParNext = c-5;
									cPar = c-5;
									c = c-5;
								}
								else
									break; //either plain text or rtf tag but \par or \fs?? -> stop parsing
				
			}

			if (*cPar)
			{
				*cPar = '}'; //trim ending \par or \fs?? 
				*(cPar+1) = 0;
			}
		}

		outRTFText.FromCString( (char *)fStreamCookie.fText);
		free( fStreamCookie.fText);

		return TRUE;
	}
	free( fStreamCookie.fText);

	return FALSE;
}

Boolean XWinStyledTextBox::SetRTFText(const VString& inRTFText)
{
	HRESULT hr;
	long    len;
	LRESULT lResult = 0;
	EDITSTREAM editStream;

	len = inRTFText.GetLength()*sizeof(UniChar);

	if (!fTextServices) 
		return FALSE;

	_EnableMultiStyle();

	VSize size = inRTFText.GetLength()+1;
	char *buffer = (char*) malloc(size);
	if (!buffer)
		return FALSE;
	size = inRTFText.ToBlock(buffer, size, VTC_DefaultTextExport, true, false);

	fStreamCookie.fText = (BSTR)buffer;
	fStreamCookie.fSize = size;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamInCallback;
	hr = fTextServices->TxSendMessage(EM_STREAMIN, (WPARAM)(SF_RTFNOOBJS), (LPARAM)&editStream, &lResult);
	free(buffer);

	fIsPlainTextDirty = true;
	fIsSizeDirty = true;

	//ensure hidden text is not included with plain text
	//_ClearHidden(); //FIXME: using ITextServices to clear hidden text seems to be buggy randomly (although hidden text ranges are correct) so for now defer it to caller
	
	return (hr == S_OK);
}

void XWinStyledTextBox::_SetTextEx(const VString& inText, bool inSelection)
{
	LRESULT lResult = 0;
	SETTEXTEX  pST;
	sLONG len;
	pST.flags= inSelection ? ST_SELECTION : ST_DEFAULT;
	pST.codepage = 1200;//Unicode
	if (inSelection)
	{
		HRESULT hr = fTextServices->TxSendMessage(EM_SETTEXTEX, (WPARAM) (&pST),(LPARAM)inText.GetCPointer(), &lResult );
		fIsPlainTextDirty = true;
		fIsSizeDirty = true;
		return;
	}
	fText.FromString(inText);
	HRESULT hr = fTextServices->TxSendMessage(EM_SETTEXTEX, (WPARAM) (&pST),(LPARAM)fText.GetCPointer(), &lResult );

	fIsPlainTextDirty = false;
	fIsSizeDirty = true;
}

GReal XWinStyledTextBox::_GetOffsetX() const
{
	bool isPrinting = ::GetDeviceCaps( fHDC, TECHNOLOGY) != DT_RASDISPLAY;
	if (isPrinting)
		return fOffsetX * nPixelsPerInchX/VWinGDIGraphicContext::GetLogPixelsX();
	else
		return fOffsetX;
}

void XWinStyledTextBox::_AdjustBoundsForVerticalAlign(VRect &ioBounds)
{
	if (!(fMaxWidth == ioBounds.GetWidth() && !fIsSizeDirty))
		//calling TxGetNaturalSize whild painting screws up context so allow only if bounds have been computed yet 
		//(should always be the case if using VTextLayout of VGraphicContext styled text methods)
		return;

	switch (fVAlign)
	{
	case AL_BOTTOM:
		{
			GReal width = ioBounds.GetWidth(), height = 0;
			DoGetSize(width, height);
			ioBounds.SetY( ioBounds.GetBottom()-height);
			ioBounds.SetHeight(height);
		}
		break;
	case AL_CENTER:
		{
			GReal width = ioBounds.GetWidth(), height = 0;
			DoGetSize(width, height);
			ioBounds.SetY( floor(ioBounds.GetY()+ioBounds.GetHeight()/2-height/2));
			ioBounds.SetHeight(height);
		}
		break;
	}
}


Boolean XWinStyledTextBox::DoDraw(const XBOX::VRect &inBounds)
{
	VRect bounds(inBounds);
	bounds.NormalizeToInt(true); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)

	_AdjustBoundsForVerticalAlign(bounds);

	GReal offsetX = _GetOffsetX();
	bounds.SetPosBy( offsetX, 0.0f);
	bounds.SetSizeBy( -offsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	VRect boundsClient = bounds;

	RECT rt = boundsClient;
	fClientRect = rt;
	HRESULT hr = S_OK;

	//optimization: do not draw outside clip rect (!)
	RECT updateRect;
	int type;

	POINT viewportOffset;
	::GetViewportOrgEx( fHDC, &viewportOffset);
	int gm = ::GetGraphicsMode( fHDC);
	if (gm == GM_ADVANCED && (viewportOffset.x || viewportOffset.y))
	{
		//get clip box in device space
		{
		StGDIResetTransform resetCTM(fHDC);
		type = ::GetClipBox( fHDC, &updateRect);
		}
		::OffsetRect( &updateRect, -viewportOffset.x, -viewportOffset.y); //transform to logical coord space

		//transform to user space
		VAffineTransform ctm;
		XFORM xform;
		::GetWorldTransform( fHDC, &xform);
		ctm.FromNativeTransform( xform);
		if (!ctm.IsIdentity())
		{
			VRect bounds(updateRect);
			bounds = ctm.Inverse().TransformRect( bounds);
			updateRect = bounds;
		}
	}
	else
	{
		type = ::GetClipBox( fHDC, &updateRect);
		::OffsetRect( &updateRect, -viewportOffset.x, -viewportOffset.y); //transform to logical coord space
	}
	if (type == NULLREGION)
		return TRUE;
	else if (type == -1 || type == ERROR)
		updateRect = bounds;

	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	if (!fIsPrinting && fSelActive && fSelStart < fSelEnd)
	{
		OnTxInPlaceActivate(&fClientRect);	 //ensure selection is painted
		
		_Select(fSelStart, fSelEnd, true);
	}


	hr = fTextServices->TxDraw(
	    DVASPECT_CONTENT,  		// Draw Aspect
		0,						// Lindex
		NULL,					// Info for drawing optimization
		NULL,					// target device information
		fHDC,				// Draw device HDC
		NULL,			 	   	// Target device HDC
		(RECTL *) &rt,	// Bounding client rectangle
		NULL,					// Clipping rectangle for metafiles
		(RECT *) &updateRect,   // Update rectangle
		NULL, 	   				// Call back function
		NULL,					// Call back parameter
		!fIsPrinting ? TXTVIEW_ACTIVE : TXTVIEW_INACTIVE);		// What view of the object could be TXTVIEW_ACTIVE

	if (!fIsPrinting && fSelActive && fSelStart < fSelEnd)
	{
		_Select( 0, 0);
		
		OnTxInPlaceDeactivate();
	}
	return (hr == S_OK);
}

/* get number of lines */
sLONG XWinStyledTextBox::GetNumberOfLines()
{
	if (!fTextServices)
		return 1;

	HRESULT hr = OnTxInPlaceActivate(&fClientRect); //we must activate control (this activation is windowless)
	if (FAILED(hr))
		return 1;

	sLONG numLine = 0;

	LRESULT lResult = 0;
	hr = fTextServices->TxSendMessage(EM_GETLINECOUNT, 0, 0, &lResult); 
	if (SUCCEEDED(hr))
		numLine = (sLONG)lResult;
	else
		numLine = 1;

	hr = OnTxInPlaceDeactivate();

	return numLine;
}

/** return the caret position & height of text at the specified text position
@remarks
	text position is 0-based

	output caret position is in GDI current user space (so is bounded by inHwndBounds)

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

	by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
	but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
*/
void XWinStyledTextBox::GetCaretMetricsFromCharIndex( const VRect& inBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	outCaretPos.SetPos( floor(inBounds.GetLeft()), floor(inBounds.GetTop()));
	outTextHeight = 0.0f;

	if (!fTextDocument)
		return;

	VRect bounds(inBounds);
	bounds.NormalizeToInt(true); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	HRESULT hr = OnTxInPlaceActivate( &fClientRect); //we must activate control in order GetPoint to run successfully
																	//(this activation is windowless)
	if (FAILED(hr))
		return;

	xbox_assert(fTextRange == NULL);
	hr = fTextDocument->Range(inCharIndex, inCharIndex+1, &fTextRange);
	if (FAILED(hr))
		return;

	long type = tomStart | (inCaretLeading ? TA_LEFT : TA_RIGHT) | TA_TOP;
	long x = 0, y = 0;
	hr = fTextRange->GetPoint( type, &x, &y);
	if (SUCCEEDED(hr) && hr != S_FALSE)
	{
		outCaretPos.SetPos( (GReal)x, (GReal)y);

		long type = tomStart | (inCaretLeading ? TA_LEFT : TA_RIGHT) | TA_BOTTOM;
		long yy;
		hr = fTextRange->GetPoint( type, &x, &yy);
		if (SUCCEEDED(hr) && hr != S_FALSE && yy >= y)
			outTextHeight = yy-y;
	}

	hr = OnTxInPlaceDeactivate();

	fTextRange->Release();
	fTextRange = NULL;
}

/** return the text position at the specified coordinates
@remarks
	output text position is 0-based

	input coordinates are in GDI current user space

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool XWinStyledTextBox::GetCharIndexFromCoord( const VRect& inBounds, const VPoint& inPos, VIndex& outCharIndex)
{
	outCharIndex = 0;

	if (!fTextDocument)
		return false;

	_GetPlainText(); //to ensure fText is updated from RTF text 

	VRect bounds(inBounds);
	bounds.NormalizeToInt(true); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	xbox_assert(fTextRange == NULL);
	long x = floor(inPos.GetX()+0.5f);
	long y = floor(inPos.GetY()+0.5f);

	HRESULT hr = OnTxInPlaceActivate( &fClientRect); //we must activate control in order RangeFromPoint to run successfully
																	//(this activation is windowless)
	if (FAILED(hr))
		return false;

	if (x >= fClientRect.right)
		x = fClientRect.right-1;
	if (x < fClientRect.left)
		x = fClientRect.left;
	if (y >= fClientRect.bottom)
		y = fClientRect.bottom-1;
	if (y < fClientRect.top)
		y = fClientRect.top;
	hr = fTextDocument->RangeFromPoint(x, y, &fTextRange);
	if (FAILED(hr))
		return false;

	long start = 0;
	hr = fTextRange->GetStart( &start);
	if (FAILED(hr))
		return false;
	outCharIndex = start;
	if (outCharIndex < 0)
		outCharIndex = 0;

	if (outCharIndex > 0 && (outCharIndex-1 < fText.GetLength()) && (fText[outCharIndex-1] == 0x0D || fText[outCharIndex-1] == '\f'))
	{
		long type = tomStart | TA_LEFT | TA_TOP;
		long xx = 0, yy = 0;
		hr = fTextRange->GetPoint( type, &xx, &yy);
		if (SUCCEEDED(hr) && yy > y)
			outCharIndex--;
	}

	hr = OnTxInPlaceDeactivate();

	fTextRange->Release();
	fTextRange = NULL;
	return true;
}


/** return styled text box run bounds from the specified range	*/
void XWinStyledTextBox::GetRunBoundsFromRange( const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd)
{
	if (!fTextDocument)
		return;

	VRect bounds(inBounds);
	bounds.NormalizeToInt(true); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	xbox_assert(fTextRange == NULL);

	//get plain text
	_GetPlainText(); //to ensure fText is updated from RTF text 
	if (fText.GetLength())
	{
		//parse lines
		if (inEnd < 0)
			inEnd = fText.GetLength();
		else if (inEnd > fText.GetLength())
			inEnd = fText.GetLength();
		if (inEnd <= inStart)
			return;

		HRESULT hr = OnTxInPlaceActivate( &fClientRect); //we must activate control in order GetPoint to run successfully
																		//(this activation is windowless)
		if (FAILED(hr))
			return;

		sLONG lineStart = inStart, lineEnd = inStart;
		const UniChar *c = fText.GetCPointer()+inStart;
		for (;lineStart < inEnd;)
		{
			//parse one line
			bool isCR = (*c == 13) || (*c == 10) || (*c == '\f');
			while (lineEnd < inEnd 
				   &&
				   (!isCR))
			{
				lineEnd++;
				c++;
				isCR = (*c == 13) || (*c == 10) || (*c == '\f');
			}
			if (isCR)
			{
				lineEnd++;
				c++;
				isCR = false;
			}
			bool ok = true;
			if (lineEnd > lineStart)
			{
				//get line bounds
				hr = fTextDocument->Range(lineStart, lineEnd, &fTextRange);
				if (FAILED(hr))
					break;

				//get left top origin
				long type = tomStart | TA_LEFT | TA_TOP;
				long x = 0, y = 0;
				hr = fTextRange->GetPoint( type, &x, &y);
				long xx = x, yy = y;

				//in case of line is wrapped, we need to parse line by dichotomy to extract this line real end
				if (SUCCEEDED(hr) && hr != S_FALSE)
				{
					//get left bottom 
					long type = tomStart | TA_LEFT | TA_BOTTOM;
					hr = fTextRange->GetPoint( type, &xx, &yy);
					if (SUCCEEDED(hr) && hr != S_FALSE)
					{
						//scan line by dichotomy until we find the last character with same bottom
						long yyStart = yy;

						sLONG start = lineStart;
						sLONG end = lineEnd;
						sLONG endMax = lineEnd;
						bool found = false;
						while (true)
						{
							long type = tomEnd | TA_LEFT | TA_BOTTOM;
							hr = fTextRange->GetPoint( type, &xx, &yy);
							if (SUCCEEDED(hr) && hr != S_FALSE)
							{
								if (yy == yyStart)
								{
									//this range is not wrapped
									if (end == endMax)
									{
										//we have reached actual end of line
										found = true;
										break;
									}
									else
									{
										//dichotomically scan remaining sub-range
										start = end-1; //because end pos character is not include in this range
										if (start < lineStart)
											start = lineStart;
										end = (start+1+endMax)/2;
										if (end <= start+1)
										{
											found = true;
											break;
										}
										fTextRange->Release();
										fTextRange = NULL;
										hr = fTextDocument->Range(start, end, &fTextRange);
										if (FAILED(hr))
										{
											ok = false;
											break;
										}
									}
								}
								else 
								{
									//this range is wrapped: dichotomically scan it
									endMax = end-1; //end pos if on another line so max valid end should be less than current end
									end = (start+end)/2;
									if (end <= start)
									{
										found = true;
										break; 
									}
									fTextRange->Release();
									fTextRange = NULL;
									hr = fTextDocument->Range(start, end, &fTextRange);
									if (FAILED(hr))
									{
										ok = false;
										break;
									}
								}
							}
							else
							{
								ok = false;
								break;
							}
						}
						if (found)
						{
							c = fText.GetCPointer()+end;
							isCR = (*c == 13) || (*c == 10) || (*c == '\f');
							lineEnd = end;
							ok = true;
						}
						else
							xx = x;
					}
					else 
						ok = false;
				}
				else
					ok = false;

				if (ok)
				{
					if (xx > x && yy > y)
					{
						VRect bounds;
						bounds.SetCoords(x, y, xx-x, yy-y);
						outRunBounds.push_back( bounds);
					}
				}

				if (fTextRange)
				{
					fTextRange->Release();
					fTextRange = NULL;
				}
			}
			if (!ok)
				break;

			/*
			if (lineEnd < inEnd)
			{
				if (isCR)
				{
					c++;
					lineEnd++;
				}
			}
			*/
			if (lineStart == lineEnd)
				break; //prevent infinite loop
			lineStart = lineEnd;
			lineEnd = lineStart;
		}

		hr = OnTxInPlaceDeactivate();
	}

}

/** insert text at the specified position */
void XWinStyledTextBox::InsertPlainText( sLONG inPos, const VString& inText)
{
	if (!fTextDocument)
		return;

	if (inText.GetLength() == 0)
		return;

	LONG len = _GetPlainText().GetLength();
	if (inPos < 0)
		inPos = 0;
	if (inPos > len)
		inPos = len;


	LRESULT lResult = 0;
	HRESULT hr = fTextServices->TxSendMessage(EM_SETSEL, inPos, inPos, &lResult); //select
	_SetTextEx( inText, true);
	fIsPlainTextDirty = true;
	fIsSizeDirty = true;
	hr = fTextServices->TxSendMessage(EM_SETSEL, -1, -1, &lResult); //unselect

	if (inPos > 0)
	{
		//new text inherits style from character previous inserted position

		//get prev char style
		xbox_assert(fTextRange == NULL);
		fTextDocument->Range(inPos-1, inPos, &fTextRange);
		fTextRange->Select();
		LRESULT lResult = 0;

		LONG wParam = SCF_SELECTION;
		CHARFORMAT2W	charFormat;
		memset(&charFormat, 0, sizeof(CHARFORMAT2W));
		charFormat.cbSize = sizeof(CHARFORMAT2W);
		fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );

		//set style for inserted text
		fTextRange->SetStart( inPos);
		fTextRange->SetEnd( inPos+inText.GetLength());
		fTextRange->Select();
		lResult = 0;
		fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );

		if (fTextRange)
		{
			fTextRange->Release();
			fTextRange = NULL;
		}

		_Select(0,0);
	}

	/*
	xbox_assert(fTextRange == NULL);

	hr = fTextDocument->Range(inPos, inPos, &fTextRange);
	if (SUCCEEDED(hr) && fTextRange)
	{
		if (SUCCEEDED(fTextRange->CanEdit(NULL)))
		{
			BSTR oleText = ::SysAllocString( inText.GetCPointer());
			if (oleText)
				hr = fTextRange->SetText( oleText); 
			::SysFreeString( oleText);

			if (SUCCEEDED(hr))
				fIsPlainTextDirty = true;
		}
		fTextRange->Release();
		fTextRange = NULL;
	}
	*/
}

/** delete text range */
void XWinStyledTextBox::DeletePlainText( sLONG rangeStart, sLONG rangeEnd)
{
	if (!fTextDocument)
		return;

	LONG len = _GetPlainText().GetLength();
	if (rangeEnd == -1)
		rangeEnd = len;

	if (rangeEnd <= rangeStart)
		return;
	if (rangeStart < 0)
		rangeStart = 0;
	if (rangeStart > len)
		rangeStart = len;
	if (rangeEnd > len)
		rangeEnd = len;

	LRESULT lResult = 0;
	HRESULT hr = fTextServices->TxSendMessage(EM_SETSEL, rangeStart, rangeEnd, &lResult); //select
	_SetTextEx( VString(), true);
	fIsPlainTextDirty = true;
	fIsSizeDirty = true;
	hr = fTextServices->TxSendMessage(EM_SETSEL, -1, -1, &lResult); //unselect

	/*
	xbox_assert(fTextRange == NULL);

	hr = fTextDocument->Range(rangeStart, rangeEnd, &fTextRange);
	if (SUCCEEDED(hr) && fTextRange)
	{
		if (fTextRange->CanEdit())
		{		
			hr = fTextRange->Delete( tomCharacter, 0, NULL); //delete text range
			if (SUCCEEDED(hr))
				fIsPlainTextDirty = true;
		}
		fTextRange->Release();
		fTextRange = NULL;
	}
	*/
}

bool XWinStyledTextBox::_GetRTL() const
{
	LRESULT lResult = 0;
	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	fTextServices->TxSendMessage(EM_GETPARAFORMAT , 0, (LPARAM)&paraFormat, &lResult );

	return ((paraFormat.dwMask & PFM_RTLPARA) && (paraFormat.wEffects & PFM_RTLPARA));
}

void XWinStyledTextBox::_SetTextAlign( AlignStyle inHAlign, sLONG inStart, sLONG inEnd)
{
	LRESULT lResult = 0;
	LONG len = _GetPlainText().GetLength();

	bool allText = inStart == 0 && (inEnd == -1 || inEnd >= len);

	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	paraFormat.dwMask |= PFM_ALIGNMENT;

	switch (inHAlign)
	{
		case AL_CENTER:
			{
			paraFormat.wAlignment = PFA_CENTER;
			}
			break;
			
		case AL_RIGHT:
			{
			paraFormat.wAlignment = PFA_RIGHT;
			}
			break;
			
		case AL_LEFT:
			{
			paraFormat.wAlignment = PFA_LEFT;
			}
			break;

		case AL_JUST:
			{
			paraFormat.wAlignment = PFA_JUSTIFY;
			}
			break;

		default:
			{
			//default: depends on rtl
			paraFormat.wAlignment = _GetRTL() ? PFA_RIGHT : PFA_LEFT;
			}
			break;
	}

	if (m_dwPropertyBits & TXTBIT_RICHTEXT)
	{
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&paraFormat, &lResult );
		if (allText)
		{
			fParaFormat.dwMask |= PFM_ALIGNMENT;
			fParaFormat.wAlignment = paraFormat.wAlignment;
			fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE); 
		}
	}
	else
	{
		fParaFormat.dwMask |= PFM_ALIGNMENT;
		fParaFormat.wAlignment = paraFormat.wAlignment;
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
	}

	fIsSizeDirty = true;
}


/** set text horizontal alignment (default is AL_DEFAULT) */
void XWinStyledTextBox::SetTextAlign( AlignStyle inHAlign, sLONG inStart, sLONG _inEnd)
{
	sLONG inEnd = _inEnd;
	VIndex length = GetPlainTextLength();
	
	if (inEnd == -1)
		inEnd = length;
	if (inStart == 0 && inEnd == length)
	{
		if (fHAlign == inHAlign)
			return;
	}
	
	_Select( inStart, inEnd);
	if (inStart == 0 && inEnd >= length-1)
		fHAlign = inHAlign;
	_SetTextAlign( inHAlign, inStart, _inEnd);
	if (inStart == 0 && inEnd >= length-1)
		_ComputeOffsetX();
	_Select( 0, 0);
}

/** set text vertical alignment (default is AL_DEFAULT) */
void XWinStyledTextBox::SetParaAlign( AlignStyle inVAlign)
{
	//only on full text
	fVAlign = inVAlign;
}


/** set paragraph line height 

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height 
*/
void XWinStyledTextBox::SetLineHeight( const GReal inLineHeight, sLONG inStart, sLONG inEnd)
{
	bool allText = inStart == 0 && inEnd == -1;

	_ClearFixedLineHeight();

	_Select( inStart, inEnd);

	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	paraFormat.dwMask |= PFM_LINESPACING;
		
	if (inLineHeight == -1)
		//single spacing
		paraFormat.bLineSpacingRule = 0;
	else if (inLineHeight < 0)
	{
		//multiple of line height
		paraFormat.bLineSpacingRule = 5;
		paraFormat.dyLineSpacing = (-inLineHeight)*20;
	}
	else
	{
		//fixed line height
		paraFormat.bLineSpacingRule = 4;
		paraFormat.dyLineSpacing = ICSSUtil::PointToTWIPS( inLineHeight);
	}

	if (allText)
	{
		fParaFormat.dwMask |= PFM_LINESPACING;
		fParaFormat.bLineSpacingRule = paraFormat.bLineSpacingRule;
		fParaFormat.dyLineSpacing = paraFormat.dyLineSpacing;
	}

	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&paraFormat, &lResult );
	if (allText)
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE); 
	
	_Select( 0, 0);

	fIsSizeDirty = true;
}

void XWinStyledTextBox::SetRTL(bool inRTL, sLONG inStart, sLONG inEnd)
{
	bool allText = inStart == 0 && inEnd == -1;
	
	_Select( inStart, inEnd);

	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	paraFormat.dwMask |= PFM_RTLPARA;
	if (inRTL)
		paraFormat.wEffects |= PFM_RTLPARA;
	else
		paraFormat.wEffects &= ~PFM_RTLPARA;

	if (allText)
	{
		fParaFormat.dwMask |= PFM_RTLPARA;
		fParaFormat.wEffects = (fParaFormat.wEffects & (~PFM_RTLPARA)) | (paraFormat.wEffects & PFM_RTLPARA);
	}

	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&paraFormat, &lResult );
	if (allText)
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE); //we do not update default paraformat because we update selection
	
	if (inStart == 0 && (inEnd == -1 || inEnd == GetPlainTextLength()) && fHAlign == AL_DEFAULT)
	{
		//default is based on rtl or ltr so reset it
		fHAlign = AL_LEFT;
		SetTextAlign(AL_DEFAULT, inStart, inEnd); 
	}

	_Select( 0, 0);

	fIsSizeDirty = true;
}

/** set first line padding offset in point 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
void XWinStyledTextBox::SetTextIndent(const GReal inPadding, sLONG inStart, sLONG inEnd)
{
	bool allText = inStart == 0 && inEnd == -1;

	_Select( inStart, inEnd);

	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	paraFormat.dwMask |= PFM_STARTINDENT | PFM_OFFSET;
	if (inPadding >= 0)
	{
		paraFormat.dxStartIndent = ICSSUtil::PointToTWIPS(inPadding);
		paraFormat.dxOffset = 0;
	}
	else
	{
		paraFormat.dxStartIndent = 0;
		paraFormat.dxOffset = -ICSSUtil::PointToTWIPS(inPadding);
	}

	if (allText)
	{
		fParaFormat.dwMask |= PFM_STARTINDENT | PFM_OFFSET;
		fParaFormat.dxStartIndent = paraFormat.dxStartIndent;
		fParaFormat.dxOffset = paraFormat.dxOffset;
	}

	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&paraFormat, &lResult );
	if (allText)
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE); 
	
	_Select( 0, 0);

	fIsSizeDirty = true;
}

/** set tab stop offset in point */
void XWinStyledTextBox::SetTabStop(const GReal inOffset, const eTextTabStopType inType, sLONG inStart, sLONG inEnd)
{
	bool allText = inStart == 0 && inEnd == -1;

	_Select( inStart, inEnd);

	PARAFORMAT2	paraFormat;
	memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	paraFormat.cbSize = sizeof(PARAFORMAT2);
	paraFormat.dwMask |= PFM_TABSTOPS;
	paraFormat.cTabCount = 1;
	paraFormat.rgxTabs[0] = ICSSUtil::PointToTWIPS( inOffset) & 0xffffff;
	LONG alignment = 0; //TTST_LEFT
	switch (inType)
	{
	case TTST_RIGHT:
		alignment = 2;
		break;
	case TTST_CENTER:
		alignment = 1;
		break;
	case TTST_DECIMAL:
		alignment = 3;
		break;
	case TTST_BAR:
		alignment = 4;
		break;
	}
	alignment = alignment<<24;
	paraFormat.rgxTabs[0] |= alignment;

	if (allText)
	{
		fParaFormat.dwMask |= PFM_TABSTOPS;
		fParaFormat.cTabCount = 1;
		fParaFormat.rgxTabs[0] = paraFormat.rgxTabs[0];
	}
	
	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&paraFormat, &lResult );
	if (allText)
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE); 
	
	_Select( 0, 0);

	fIsSizeDirty = true;
}


void XWinStyledTextBox::_Select(sLONG inStart, sLONG inEnd, bool inNoSwitchToMultistyle)
{
	LONG len = _GetPlainText().GetLength();
	if (inEnd == -1)
		inEnd = len;
	if (inEnd < inStart)
		inEnd = inStart;
	
	if (!inNoSwitchToMultistyle && inStart < inEnd)
		if (inStart > 0 || inEnd < len)
			//switch to RichText mode if style is not applied to all text
			_EnableMultiStyle();

	xbox_assert(fTextRange == NULL);

	fTextDocument->Range(inStart, inEnd, &fTextRange);
	if (testAssert(fTextRange))
	{
		fTextRange->Select();
		fTextRange->Release();
		fTextRange = NULL;
	}
}


void XWinStyledTextBox::DoApplyStyle(const VTextStyle* inStyle, VTextStyle * inStyleInherit)
{
	LONG len = _GetPlainText().GetLength();
	sLONG rangeStart, rangeEnd;
	LONG wParam = SCF_SELECTION;

	inStyle->GetRange(rangeStart, rangeEnd);
	_Select( rangeStart, rangeEnd);

	LRESULT lResult = 0;

	if (m_dwPropertyBits & TXTBIT_RICHTEXT)
	{
		//multistyle: merge new style with current style on selection range -> only styles overriden with charFormat.dwMask will be changed

		CHARFORMAT2W	charFormat;
		memset(&charFormat, 0, sizeof(CHARFORMAT2W));
		charFormat.cbSize = sizeof(CHARFORMAT2W);
		
		if (CharFormatFromVTextStyle(&charFormat, inStyle, fHDC, inStyleInherit && inStyleInherit->GetHasBackColor() && inStyleInherit->GetBackGroundColor() != RGBACOLOR_TRANSPARENT))
		{		
			/*
			if (fIsFixedLineHeight)
			{
				if (inStyle->GetFontSize() >= 0 && inStyle->GetFontSize() != fDefaultFontSize)
				{
					//we apply a font size different from the default font size -> disable fixed line height

					_ClearFixedLineHeight();
					_Select( rangeStart, rangeEnd);
				}
			}
			*/

			fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );
			fIsSizeDirty = true;
		}
	}
	else
	{
		//monostyle: we update uniform fCharFormat & inform control with TXTBIT_CHARFORMATCHANGE because EM_SETCHARFORMAT works only with RichText edit control

		if (CharFormatFromVTextStyle(&fCharFormat, inStyle, fHDC, inStyleInherit && inStyleInherit->GetHasBackColor() && inStyleInherit->GetBackGroundColor() != RGBACOLOR_TRANSPARENT))
		{
			if (fCurFont && inStyle->GetFontSize() != -1)
			{
				VFont *font = fCurFont->DeriveFontSize( inStyle->GetFontSize(), 72);
				CopyRefCountable(&fCurFont, font);
				ReleaseRefCountable(&font);
			}

			fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&fCharFormat, &lResult );
			fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, 
				TXTBIT_CHARFORMATCHANGE);

			fIsSizeDirty = true;
		}
	}

	//update eventually paragraph format: here only justification
	if (inStyle->GetJustification() != JST_Notset)
	{
		AlignStyle alignment = ITextLayout::JustToAlignStyle(inStyle->GetJustification());
		if (rangeStart == 0 && rangeEnd >= len-1) 
		{
			if (fHAlign != alignment)
			{
				fHAlign = alignment;
				_SetTextAlign( alignment);
				_ComputeOffsetX();
			}
		}
	}
	_DoApplyStyleRef( inStyle);

	_Select( 0, 0);
}


Boolean XWinStyledTextBox::DoInitialize()
{
	fIsPlainTextDirty = false;
	fIsSizeDirty = true;

	SetRectEmpty(&fClientRect);
	SetRectEmpty(&fViewInset);

	//FIXME: dynamic switching from standard edit to rich edit is buggy because text size is not always equal if text contains japanese characters for instance
	//		 so disable it for now until a better fix...
	//on default, we use standard edit control: it will automatically switch to RichText edit control if RTF text is set or if a style is applied on a sub-part of the text 
	//m_dwPropertyBits = TXTBIT_READONLY | TXTBIT_HIDESELECTION | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP)/* | TXTBIT_USECURRENTBKG | TXTBIT_WORDWRAP*/;
	m_dwPropertyBits = TXTBIT_READONLY | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP)/* | TXTBIT_USECURRENTBKG | TXTBIT_WORDWRAP*/;

	if((fLayoutMode & TRM_LEGACY_MONOSTYLE) ==0)
		m_dwPropertyBits |= TXTBIT_RICHTEXT;

	InitDefaultCharFormat(fHDC);
	InitDefaultParaFormat();
	fTextServices = NULL;
	fTextDocument = NULL;
	fTextRange = NULL;
	fRefCount = 1;

	m_dwMaxLength = INFINITE;
	HRESULT hr = S_OK;
	hr = CreateTextServicesObject();
	if (SUCCEEDED(hr))
	{
		LRESULT lResult = 0;
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lResult); //apply paraformat on all text - which is empty now (wparam is always 0 for EM_SETPARAFORMAT)
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);

		lResult = 0;
		fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_DEFAULT, (LPARAM)&fCharFormat, &lResult ); //set default charformat
		fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, TXTBIT_CHARFORMATCHANGE);

		//JQ 14/06/2012: fixed auto font - we should disable autofont/autosize/autokeyboard
		//				 because we do not want control to change font or fontsize or keyboard layout...
		LRESULT langOptions = 0;
		fTextServices->TxSendMessage(EM_GETLANGOPTIONS, 0, 0, &langOptions);
		langOptions &= ~(IMF_AUTOFONT|IMF_DUALFONT|IMF_AUTOFONTSIZEADJUST|IMF_AUTOKEYBOARD);
		lResult = 0;
		fTextServices->TxSendMessage(EM_SETLANGOPTIONS, 0, langOptions, &lResult);
	}
	return (hr == S_OK);
}

/** set selection range 
@remarks
	used only by Windows impl to let native control to paint selection
*/
void XWinStyledTextBox::Select(sLONG inStart, sLONG inEnd, bool inIsActive) 
{ 
	fSelStart = inStart; 
	fSelEnd = inEnd; 
	if (fSelEnd == -1)
		fSelEnd = _GetPlainText().GetLength();
	fSelActive = inIsActive;
}


void XWinStyledTextBox::_EnableMultiStyle()
{
	/*
	if (!fTextServices)
		return;

	//if((fLayoutMode & TRM_LEGACY_MONOSTYLE) ==TRM_LEGACY_MONOSTYLE)
	//{
	//	return;
	//}
	
	if (m_dwPropertyBits & TXTBIT_RICHTEXT)
		return;

	xbox_assert((fLayoutMode & TRM_LEGACY_MONOSTYLE) != 0); //it is legacy monostyle but we need to switch to richtext in order to paint selection for instance 
															//(if not legacy monostyle, TXTBIT_RICHTEXT is always set)

	fIsSizeDirty = true;


	//m_dwPropertyBits |= TXTBIT_RICHTEXT;
	//fTextServices->OnTxPropertyBitsChange(TXTBIT_RICHTEXT, TXTBIT_RICHTEXT);

	//return;
	//in order to switch to RichText control, we need to re-create control from scratch because we cannot transform a standard edit control to a RichText edit control
	//(we could use first RichText edit in plain text mode but we do not want to use RichText control in monostyle only)

	//backup first plain text (uniform style is available in fCharFormat)
	VString text = _GetPlainText();

	//release & create RichText control
	if (fTextDocument != NULL)
		fTextDocument->Release();
	fTextDocument = NULL;
	if (fTextServices != NULL)
		fTextServices->Release();
	fTextServices = NULL;

	m_dwPropertyBits = TXTBIT_RICHTEXT | TXTBIT_READONLY | TXTBIT_HIDESELECTION | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP);
	m_dwMaxLength = INFINITE;
	HRESULT hr = S_OK;
	hr = CreateTextServicesObject();

	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lResult); //apply paraformat on all text (wparam is always 0 for EM_SETPARAFORMAT)
	fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);

	lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_DEFAULT, (LPARAM)&fCharFormat, &lResult ); //set default charformat
	fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, TXTBIT_CHARFORMATCHANGE);

	//JQ 14/06/2012: fixed auto font - we should disable autofont/autosize/autokeyboard
	//				 because we do not want control to change font or fontsize or keyboard layout...
	LRESULT langOptions = 0;
	fTextServices->TxSendMessage(EM_GETLANGOPTIONS, 0, 0, &langOptions);
	langOptions &= ~(IMF_AUTOFONT|IMF_DUALFONT|IMF_AUTOFONTSIZEADJUST|IMF_AUTOKEYBOARD);
	lResult = 0;
	fTextServices->TxSendMessage(EM_SETLANGOPTIONS, 0, langOptions, &lResult);

	//set text
	_SetText( text);

	//set uniform style 
	xbox_assert(fTextRange == NULL);
	fTextDocument->Range(0, text.GetLength(), &fTextRange);
	fTextRange->Select();

	lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_SELECTION, (LPARAM)&fCharFormat, &lResult ); //apply on all text

	fTextRange->Release();
	fTextRange = NULL;

	_InitFixedLineHeight( fCurFont);
	*/
}

void XWinStyledTextBox::DoRelease()
{
	if (fTextDocument != NULL)
		fTextDocument->Release();
	if (fTextServices != NULL)
		fTextServices->Release();
	fHDC = NULL;
	if (fHDCMetrics)
		::DeleteDC( fHDCMetrics);
}

void XWinStyledTextBox::DoGetSize(GReal &ioWidth, GReal &outHeight,bool inForPrinting)
{
	if (fMaxWidth == ioWidth && !fIsSizeDirty)
	{
		ioWidth = fCurWidth;
		outHeight = fCurHeight;
		return;
	}

	fMaxWidth = ioWidth;

	if (fTextServices)
	{
		
		if(inForPrinting)
		{
			GetSizeForPrinting(ioWidth,outHeight);
		}
		else
		{
			GReal offsetX = _GetOffsetX();

			SIZEL szExtent;
			sLONG width = ioWidth+fabs(offsetX);//(ioWidth * HIMETRIC_PER_INCH) / (GReal)nPixelsPerInchX;
			sLONG height = 1;

			szExtent.cx = 1;
			szExtent.cy = 1;
			fTextServices->TxGetNaturalSize(DVASPECT_CONTENT, 
				fHDC, 
				NULL,
				NULL,
				TXTNS_FITTOCONTENT,
				&szExtent,
				&width,
				&height);

			if (fCurFont && (!(m_dwPropertyBits & TXTBIT_RICHTEXT)))
			{
				//we need to add last line height if last character is CR otherwise for standard text edit, height does not include last CR
				const VString& text = _GetPlainText();
				if (!text.IsEmpty() && text.GetUniChar(text.GetLength()) == 13)
				{
					VGraphicContext *gc = VGraphicContext::CreateForComputingMetrics( eWinGDIGraphicContext); 
					VFontMetrics *metrics = new VFontMetrics( fCurFont, gc);
					height += metrics->GetAscent()+metrics->GetDescent()+metrics->GetExternalLeading(); 
					ReleaseRefCountable(&gc);
				}
			}

			ioWidth = width-fabs(offsetX);

			if (ioWidth < 0.0f)
				ioWidth = 0.0f;
			outHeight = height;

			VRect bounds(0, 0, ioWidth, outHeight);
			bounds.SetSizeBy(fabs(offsetX), 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

			RECT rt = bounds;
			fClientRect = rt;
			fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
			fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);
		}
		
		fCurWidth = ioWidth;
		fCurHeight = outHeight;

		fIsSizeDirty = false;
	}
	else
		fIsSizeDirty = true;
}

void XWinStyledTextBox::GetSizeForPrinting(GReal &ioWidth, GReal &outHeight)
{
	if (fTextServices)
	{
		LRESULT r;
		GReal offsetX = _GetOffsetX();
		int ratio, denom;
		sLONG outputDCDPI = GetDeviceCaps(fOutputRefDC,LOGPIXELSY);
		sLONG refDCDPI = GetDeviceCaps(fHDC,LOGPIXELSY);
		SIZEL szExtent={1,1};
		sLONG width = 0;
		sLONG height = 1;

		width = MulDiv( ioWidth+fabs(offsetX), outputDCDPI ,96);
		szExtent.cx = width;

		fTextServices->TxGetNaturalSize(DVASPECT_CONTENT, 
			fOutputRefDC, 
			NULL,
			NULL,
			TXTNS_FITTOCONTENT,
			&szExtent,
			&width,
			&height);

		ioWidth = ((width-fabs(offsetX)) *96) / (outputDCDPI*1.0);
		
		if (ioWidth < 0.0f)
			ioWidth = 0.0f;
		outHeight = (height * 96 / (outputDCDPI*1.0));
	}
}

sLONG XWinStyledTextBox::_GetLineHeight( VFont *inFont)
{
	fDefaultFontSize = inFont->GetPixelSize(); //on Windows, 4D form dpi=72 so 1pt=1px and so a real 12px size font = 12pt size font in 4D form (equal to font size as stored in VTextStyle)
	bool isPrinting = ::GetDeviceCaps( fHDC, TECHNOLOGY) != DT_RASDISPLAY;
	VGraphicContext *gc = isPrinting ? VGraphicContext::Create( fHDC, VRect(), 1, false, true, eWinGDIGraphicContext) : VGraphicContext::CreateForComputingMetrics( eWinGDIGraphicContext); 
	VFontMetrics *metrics = new VFontMetrics( inFont, gc);
	sLONG lineHeight = ICSSUtil::PointToTWIPS( (metrics->GetAscent()+metrics->GetDescent()+metrics->GetExternalLeading())*fFontSizeToDocumentFontSize); 
	delete metrics;
	ReleaseRefCountable(&gc);
	return lineHeight;
}

void XWinStyledTextBox::_InitFixedLineHeight(VFont *inFont)
{
	//disabled because somewhat buggy in multistyle or while swithing between monostyle to multistyle: 
	//in monostyle we let control paint selection & in multistyle we let control change line height
	//fixed ACI0081052: dirty hack but i did not found any other workaround 
	/*
	if (!gIsIdeographicLanguageInitialized)
	{
		DialectCode dialectCode = XBOX::VIntlMgr::GetSystemUIDialectCode(); //it seems we need to hack only if system dialect code is ideographic

		//FIXME: maybe i forget some ideographic languages ?
		gIsIdeographicLanguage =	dialectCode == DC_JAPANESE 
									||
									dialectCode == DC_KOREAN_WANSUNG
									||
									dialectCode == DC_KOREAN_JOHAB
									||
									dialectCode == DC_KOREAN_JOHAB
									||
									dialectCode == DC_CHINESE_TRADITIONAL
									||
									dialectCode == DC_CHINESE_SIMPLIFIED
									||
									dialectCode == DC_CHINESE_HONGKONG
									||
									dialectCode == DC_CHINESE_SINGAPORE;
		gIsIdeographicLanguageInitialized = true;
	}
	if (gIsIdeographicLanguage && inFont && m_dwPropertyBits & TXTBIT_RICHTEXT)
	{
		//fix line height on ideographic OS (because of nasty RTE bug ACI0081052)

		fParaFormat.dwMask |= PFM_LINESPACING;
		fParaFormat.bLineSpacingRule = 4;
		
		fParaFormat.dyLineSpacing = _GetLineHeight( inFont); 

		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
		fIsFixedLineHeight = true;

		_Select(0, -1);
		LRESULT lResult = 0;
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lResult); //apply default paraformat on all text (wparam is always 0 for EM_SETPARAFORMAT)
		
		if (fInitDone)
			fShouldResetLayout = true; //tell caller to rebuild the layout (there are artifacts otherwise if we change fixed line height after initialization)
	}
	*/
}

void XWinStyledTextBox::_ClearFixedLineHeight()
{
	/*
	if (fIsFixedLineHeight)
	{
		//reset to normal line spacing

		fParaFormat.dwMask |= PFM_LINESPACING;
		fParaFormat.bLineSpacingRule = 0;
		fParaFormat.dyLineSpacing = 0; 
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
		fIsFixedLineHeight = false;

		_Select(0, -1);
		LRESULT lResult = 0;
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lResult); //apply default paraformat on all text (wparam is always 0 for EM_SETPARAFORMAT)

		if (fInitDone)
			fShouldResetLayout = true; //tell caller to rebuild the layout (there are artifacts otherwise if we change fixed line height after initialization)
	}
	*/
}

void XWinStyledTextBox::_SetFont(VFont *font, const VColor &textColor)
{
	CopyRefCountable(&fCurFont, font);

	HFONT hfont,oldfont;
	hfont=font ? font->GetFontRef() : NULL;
	CharFormatFromHFONT(&fCharFormat, hfont, textColor, fHDC);
	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_DEFAULT, (LPARAM)&fCharFormat, &lResult );
	fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, 
		TXTBIT_CHARFORMATCHANGE);
	lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_ALL, (LPARAM)&fCharFormat, &lResult );

	_InitFixedLineHeight( font);
	
	fIsSizeDirty = true;
}

bool XWinStyledTextBox::_VTextStyleFromCharFormat(VTextStyle* ioStyle, CHARFORMAT2W* inCharFormat)
{
	if (!ioStyle)
		return false;

	ioStyle->SetJustification(ITextLayout::JustFromAlignStyle(fHAlign));

	bool isUniform = true;
	if (inCharFormat->dwMask & CFM_BOLD)
	{
		ioStyle->SetBold(inCharFormat->dwEffects & CFE_BOLD ? 1 : 0);
	}
	else
	{
		ioStyle->SetBold(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_ITALIC)
	{
		ioStyle->SetItalic(inCharFormat->dwEffects & CFE_ITALIC ? 1 : 0);
	}
	else
	{
		ioStyle->SetItalic(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_UNDERLINE)
	{
		ioStyle->SetUnderline(inCharFormat->dwEffects & CFE_UNDERLINE ? 1 : 0);
	}
	else
	{
		ioStyle->SetUnderline(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_STRIKEOUT)
	{
		ioStyle->SetStrikeout(inCharFormat->dwEffects & CFE_STRIKEOUT ? 1 : 0);
	}
	else
	{
		ioStyle->SetStrikeout(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_FACE)
	{
		VString fontname(inCharFormat->szFaceName);
		ioStyle->SetFontName(fontname);
	}
	else
	{
		ioStyle->SetFontName("");
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_SIZE)
	{
		xbox_assert(fFontSizeToDocumentFontSize > 0.0f);
		Real fontSize = floor((inCharFormat->yHeight*72.0f)/(LY_PER_INCH * fFontSizeToDocumentFontSize) + 0.5f); //for consistency with  XWinStyledTextBox::CharFormatFromVTextStyle
		ioStyle->SetFontSize( fontSize);
	}
	else
	{
		ioStyle->SetFontSize(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_BACKCOLOR)
	{
		if (inCharFormat->dwEffects & CFE_AUTOBACKCOLOR)
		{
			ioStyle->SetHasBackColor(false);
		}
		else
		{
			ioStyle->SetHasBackColor(true);
			XBOX::VColor backcolor;
			backcolor.WIN_FromCOLORREF(inCharFormat->crBackColor);
			ioStyle->SetBackGroundColor(backcolor.GetRGBAColor());
		}
	}
	else
	{
		ioStyle->SetHasBackColor(false);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_COLOR)
	{
		ioStyle->SetHasForeColor(true);
		XBOX::VColor forecolor;
		forecolor.WIN_FromCOLORREF(inCharFormat->crTextColor);
		ioStyle->SetColor(forecolor.GetRGBAColor());
	}
	else
	{
		ioStyle->SetHasForeColor(false);
		isUniform = false;
	}
	
	return 	isUniform;
}

bool XWinStyledTextBox::_GetStyle(VTextStyle* ioStyle, sLONG inStart, sLONG inEnd)
{
	if (!testAssert(ioStyle))
		return false;

	if (!fTextServices)
		return false;

	if (inStart >= inEnd)
	{
		ioStyle->Reset();
		return false;
	}

	ioStyle->SetRange(inStart, inEnd);
	LONG wParam;

	ITextRange *textRange;
	HRESULT hr = fTextDocument->Range(inStart, inEnd, &textRange);
	if (FAILED(hr))
		return false;

	textRange->Select();
	wParam = SCF_SELECTION;

	CHARFORMAT2W	charFormat;
	memset(&charFormat, 0, sizeof(CHARFORMAT2W));
	charFormat.cbSize = sizeof(CHARFORMAT2W);
	LRESULT lr = 0;
	hr = fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lr);
	if (FAILED(hr))
	{
		textRange->Release();
		return false;
	}
	textRange->Release();

	return _VTextStyleFromCharFormat(ioStyle, &charFormat);
}




/////////////////////////////////////////////////////////////////////////////
// ITextHost functions
HDC XWinStyledTextBox::TxGetDC()
{
	xbox_assert(fHDC);
	return fHDC;
}

INT XWinStyledTextBox::TxReleaseDC(HDC hdc)
{
#if VERSIONDEBUG
	xbox_assert(fHDC == hdc);
#endif
	return 1;
}

BOOL XWinStyledTextBox::TxShowScrollBar(INT fnBar, BOOL fShow)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetScrollRange(INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw)
{
	return FALSE;
}

void XWinStyledTextBox::TxInvalidateRect(LPCRECT prc, BOOL fMode)
{
}

void XWinStyledTextBox::TxViewChange(BOOL fUpdate)
{
}

BOOL XWinStyledTextBox::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxShowCaret(BOOL fShow)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetCaretPos(INT x, INT y)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetTimer(UINT idTimer, UINT uTimeout)
{
	return FALSE;
}

void XWinStyledTextBox::TxKillTimer(UINT idTimer)
{
}

void XWinStyledTextBox::TxScrollWindowEx(INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll)
{
}

void XWinStyledTextBox::TxSetCapture(BOOL fCapture)
{
}

void XWinStyledTextBox::TxSetFocus()
{
}

void XWinStyledTextBox::TxSetCursor(HCURSOR hcur, BOOL fText)
{
}

BOOL XWinStyledTextBox::TxScreenToClient(LPPOINT lppt)
{
	return TRUE; //for windowless, we use only coordinates in current DC user space
}

BOOL XWinStyledTextBox::TxClientToScreen(LPPOINT lppt)
{
	return TRUE; //for windowless, we use only coordinates in current DC user space
}

HRESULT	XWinStyledTextBox::TxActivate(LONG * plOldState)
{
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxDeactivate(LONG lNewState)
{
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetClientRect(LPRECT outClientRect)
{
	*outClientRect = fClientRect;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetViewInset(LPRECT outViewInset)
{
	*outViewInset = fViewInset;
	return S_OK;
}

HRESULT XWinStyledTextBox::TxGetCharFormat(const CHARFORMATW **outCharFormat)
{
	*outCharFormat = (CHARFORMATW*)(&fCharFormat);
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetParaFormat(const PARAFORMAT **outParaFormat)
{
	*outParaFormat = &fParaFormat;
	return S_OK;
}

COLORREF XWinStyledTextBox::TxGetSysColor(int nIndex)
{
	return GetSysColor(nIndex);
}

HRESULT	XWinStyledTextBox::TxGetBackStyle(TXTBACKSTYLE *pstyle)
{
	*pstyle = TXTBACK_TRANSPARENT;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetMaxLength(DWORD *plength)
{
	*plength = m_dwMaxLength;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetScrollBars(DWORD *pdwScrollBar)
{
	*pdwScrollBar = 0;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetPasswordChar(TCHAR *pch)
{
	return S_FALSE;
}

HRESULT	XWinStyledTextBox::TxGetAcceleratorPos(LONG *pcp)
{
	*pcp = -1;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetExtent(LPSIZEL lpExtent)
{
	// Calculate the length & convert to himetric
	*lpExtent = fExtent;

	return S_OK;
}

HRESULT XWinStyledTextBox::OnTxCharFormatChange(const CHARFORMATW * pcf)
{
	memcpy(&fCharFormat, pcf, pcf->cbSize);
	return S_OK;
}

HRESULT	XWinStyledTextBox::OnTxParaFormatChange(const PARAFORMAT * ppf)
{
	memcpy(&fParaFormat, ppf, ppf->cbSize);
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits)
{
	DWORD dwProperties = 0;

	dwProperties = m_dwPropertyBits | TXTBIT_PARAFORMATCHANGE;

	//if (m_dwPropertyBits & ES_MULTILINE)
	//{
	//	dwProperties |= TXTBIT_MULTILINE;
	//}

	//if (m_dwPropertyBits & ES_READONLY)
	//{
	//	dwProperties |= TXTBIT_READONLY;
	//}


	//if (m_dwPropertyBits & ES_PASSWORD)
	//{
	//	dwProperties |= TXTBIT_USEPASSWORD;
	//}

	//if (!(m_dwPropertyBits & ES_NOHIDESEL))
	//{
	//	dwProperties |= TXTBIT_HIDESELECTION;
	//}
	//dwProperties |= TXTBIT_WORDWRAP;

	*pdwBits = dwProperties & dwMask;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxNotify(DWORD iNotify, void *pv)
{
	return S_OK;
}

HIMC XWinStyledTextBox::TxImmGetContext()
{
	return NULL;
}

void XWinStyledTextBox::TxImmReleaseContext(HIMC himc)
{
}

HRESULT	XWinStyledTextBox::TxGetSelectionBarWidth(LONG *lSelBarWidth)
{
	*lSelBarWidth = 100;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// custom functions
bool XWinStyledTextBox::CharFormatFromVTextStyle(CHARFORMAT2W* ioCharFormat, const VTextStyle* inStyle, HDC inHDC, bool inParentHasBackColor)
{
	//remark: here we must update dwMask only for styles we override (!)

	if (!inStyle)
		return (false);

	bool needUpdate = false;
	sLONG isbold = inStyle->GetBold();
	if(isbold != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_BOLD;
		if (isbold != 0)
		{
			ioCharFormat->dwEffects |= CFE_BOLD;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_BOLD;
		}
	}

	sLONG isitalic = inStyle->GetItalic();
	if(isitalic != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_ITALIC;
		if (isitalic != 0)
		{
			ioCharFormat->dwEffects |= CFE_ITALIC;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_ITALIC;
		}
	}

	sLONG isunderline = inStyle->GetUnderline();
	if(isunderline != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_UNDERLINE;
		if (isunderline != 0)
		{
			ioCharFormat->dwEffects |= CFE_UNDERLINE;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_UNDERLINE;
		}
	}

	sLONG isstrikeout = inStyle->GetStrikeout();
	if(isstrikeout != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_STRIKEOUT;
		if (isstrikeout != 0)
		{
			ioCharFormat->dwEffects |= CFE_STRIKEOUT;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_STRIKEOUT;
		}
	}

	const VString& fontname = inStyle->GetFontName();
	if(fontname.GetLength() > 0)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_FACE;
		//JQ 09/07/2010: fixed font name copy
		//memcpy(ioCharFormat->szFaceName, fontname.GetCPointer(), sizeof(ioCharFormat->szFaceName));
		wcscpy( ioCharFormat->szFaceName, fontname.GetCPointer());
		ioCharFormat->bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	}

	GReal fontsize = inStyle->GetFontSize();
	if(fontsize != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_SIZE;
		//JQ 09/07/2010: ??? une taille de fonte est TOUJOURS exprimée en POINTS et pas en PIXELS...
		//				 donc fontsize en inch = fontsize en points / 72 (car 1 pt = 1/72 inch)
		//				 et fontsize en himetrics = fonsize en inch * LY_PER_INCH
		//LONG yPixPerInch = GetDeviceCaps(inHDC, LOGPIXELSY);
		//ioCharFormat->yHeight = fontsize * LY_PER_INCH / yPixPerInch;
		ioCharFormat->yHeight = floor((fontsize * fFontSizeToDocumentFontSize * LY_PER_INCH) / 72 + 0.5f);
	}

	bool hasBackColor = inStyle->GetHasBackColor();
	if(fShouldPaintCharBackColor && hasBackColor && (inParentHasBackColor || inStyle->GetHasBackColor() != RGBACOLOR_TRANSPARENT))
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_BACKCOLOR;
		if (inStyle->GetBackGroundColor() == RGBACOLOR_TRANSPARENT)
		{
			ioCharFormat->dwEffects &= ~CFE_AUTOBACKCOLOR;
			ioCharFormat->crBackColor = fParentFrameBackColor.WIN_ToCOLORREF();
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_AUTOBACKCOLOR;
			XBOX::VColor backcolor;
			backcolor.FromRGBAColor(inStyle->GetBackGroundColor());
			ioCharFormat->crBackColor = backcolor.WIN_ToCOLORREF();
		}
	}

	bool hasForeColor = inStyle->GetHasForeColor();
	if(hasForeColor)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_COLOR;
		ioCharFormat->dwEffects &= ~CFE_AUTOCOLOR;
		XBOX::VColor forecolor;
		forecolor.FromRGBAColor(inStyle->GetColor());
		ioCharFormat->crTextColor = forecolor.WIN_ToCOLORREF();
	}

	return 	needUpdate;
}


HRESULT XWinStyledTextBox::CharFormatFromHFONT(CHARFORMAT2W* ioCharFormat, HFONT hFont, const VColor &textColor, HDC inHDC)
// Takes an HFONT and fills in a CHARFORMAT2W structure with the corresponding info
{

	// Get LOGFONT for default font
	LOGFONTW lf;
	LONG yPixPerInch;
	if (!hFont)
	{
		//JQ 09/07/2010: try to get current font from hdc first
		//				 otherwise we cannot inherit styles from current dc font !
		hFont = (HFONT) ::GetCurrentObject(inHDC, OBJ_FONT);
		if (!hFont)
			hFont = (HFONT) GetStockObject(SYSTEM_FONT);
	}

	// Get LOGFONT for passed hfont
	if (!GetObjectW(hFont, sizeof(LOGFONTW), &lf))
		return E_FAIL;

	// Set CHARFORMAT structure
	memset(ioCharFormat, 0, sizeof(CHARFORMAT2W));
	ioCharFormat->cbSize = sizeof(CHARFORMAT2W);

	//ioCharFormat->yHeight = -lf.lfHeight * LY_PER_INCH / nPixelsPerInchY;
	ioCharFormat->yHeight = floor((-lf.lfHeight * fFontSizeToDocumentFontSize * LY_PER_INCH) / 72 + 0.5f);
	ioCharFormat->yOffset = 0;
	ioCharFormat->crTextColor = textColor.WIN_ToCOLORREF();


	ioCharFormat->dwEffects = CFM_EFFECTS | CFE_AUTOBACKCOLOR;
	if (!(m_dwPropertyBits & TXTBIT_RICHTEXT)) //CFE_AUTOBACKCOLOR is buggy with standard edit control (black color for background...) 
											   //so force COLOR_WINDOW syscolor here to simulate RichText edit control
		ioCharFormat->crBackColor = (COLORREF)::GetSysColor(COLOR_WINDOW);
	ioCharFormat->dwEffects &= ~(/*CFE_PROTECTED |*/ CFE_LINK | CFE_AUTOCOLOR);

	if(lf.lfWeight < FW_BOLD) {
	  ioCharFormat->dwEffects &= ~CFE_BOLD;
	}

	if(!lf.lfItalic) {
	  ioCharFormat->dwEffects &= ~CFE_ITALIC;
	}

	if(!lf.lfUnderline) 
	{
	  ioCharFormat->dwEffects &= ~CFE_UNDERLINE;
	}

	if(!lf.lfStrikeOut) {
	  ioCharFormat->dwEffects &= ~CFE_STRIKEOUT;
	}

	ioCharFormat->dwMask = CFM_ALL | CFM_BACKCOLOR | CFM_STYLE | CFM_COLOR;

	wcscpy(ioCharFormat->szFaceName, lf.lfFaceName);
	ioCharFormat->bCharSet = lf.lfCharSet;
	ioCharFormat->bPitchAndFamily = lf.lfPitchAndFamily;
	return S_OK;
}

HRESULT XWinStyledTextBox::InitDefaultCharFormat(HDC hdc)
{
	VColor col(VColor::sBlackColor);
	return CharFormatFromHFONT(&fCharFormat, NULL, col, hdc);
}

HRESULT XWinStyledTextBox::InitDefaultParaFormat()
{
	memset(&fParaFormat, 0, sizeof(PARAFORMAT2));
	fParaFormat.cbSize = sizeof(PARAFORMAT2);
	fParaFormat.dwMask = PFM_ALL2;
	fParaFormat.wAlignment = PFA_LEFT;
	fHAlign = AL_LEFT;
	fVAlign = AL_TOP;
	//fParaFormat.dxStartIndent = -2*;
	fParaFormat.cTabCount = 1;
	fParaFormat.rgxTabs[0] = lDefaultTab;
	return S_OK;
}

static bool sIsDllLoaded = false;
static PCreateTextServices theCreateTextServices = NULL;

static bool sIsRiched20Loaded = false;
static PCreateTextServices theCreateTextServices_Riched20 = NULL;

HRESULT XWinStyledTextBox::CreateTextServicesObject()
{
	HMODULE hMod = NULL;	
	HRESULT hr;
	IUnknown *spUnk;
	PCreateTextServices pCreateTS =  NULL;
	// Create Text Services component


	if(VSystem::IsEightOne() && fIsPrinting)
	{
		if(!sIsRiched20Loaded)
		{
			hMod = LoadLibraryW(L"RichEd20.dll");
			if(hMod)
			{
				theCreateTextServices_Riched20 = ( PCreateTextServices) GetProcAddress(hMod, "CreateTextServices");
				sIsRiched20Loaded = true;
			}
		}
		pCreateTS = theCreateTextServices_Riched20;
	}
	if (!pCreateTS)
	{
		if(!sIsDllLoaded)
		{
			hMod = LoadLibraryW(L"msftedit.dll");
			if(!hMod)
				hMod = LoadLibraryW(L"RichEd20.dll");
			if(hMod)
			{
				theCreateTextServices = ( PCreateTextServices) GetProcAddress(hMod, "CreateTextServices");		
				sIsDllLoaded = true;
			}
		}
		pCreateTS=theCreateTextServices;
	}
	if (!testAssert(pCreateTS))
		return S_FALSE;

	hr = (pCreateTS(NULL, static_cast<ITextHost*>(this), &spUnk));
	if (hr == S_OK) {
		hr = spUnk->QueryInterface(IID_ITextServices, (void**)&fTextServices);
		hr = spUnk->QueryInterface(IID_ITextDocument, (void**)&fTextDocument);
		spUnk->Release();
	}
	return hr;
}

HRESULT XWinStyledTextBox::QueryInterface( 
	REFIID riid,
	void __RPC_FAR *__RPC_FAR *ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	*ppvObject = NULL;

	//if (IsEqualIID(riid, IID_IUnknown) 
	//	|| IsEqualIID(riid, IID_ITextHost)) 
	//{
	//	AddRef();
	//	*ppvObject = (ITextHost *) this;
	//	hr = S_OK;
	//}

	return hr;
}

ULONG XWinStyledTextBox::AddRef(void)
{
	return (ULONG)IRefCountable::Retain();
}

ULONG XWinStyledTextBox::Release(void)
{
	return (ULONG)IRefCountable::Release();
}


/** return true if full range is hidden */
bool XWinStyledTextBox::_IsRangeHidden( sLONG inStart, sLONG inEnd, bool &isMixed)
{
	isMixed = false;
	if (!fTextServices)
		return false;

	if (inStart >= inEnd)
		return false;

	ITextRange *textRange;
	HRESULT hr = fTextDocument->Range(inStart, inEnd, &textRange);
	if (FAILED(hr))
		return false;

	textRange->Select();
	LONG wParam = SCF_SELECTION;

	CHARFORMAT2W	charFormat;
	memset(&charFormat, 0, sizeof(CHARFORMAT2W));
	charFormat.cbSize = sizeof(CHARFORMAT2W);
	LRESULT lr = 0;
	hr = fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lr);
	if (FAILED(hr))
	{
		textRange->Release();
		return false;
	}
	textRange->Release();

	bool result = false;
	if (charFormat.dwMask & CFM_HIDDEN)
	{
		result = ((charFormat.dwEffects & CFE_HIDDEN) != 0);
		return result;
	}
	isMixed = true;
	return false;
}

/** return ranges of hidden text */
void XWinStyledTextBox::GetRangesHidden( VectorOfTextRange& outRanges)
{
	sLONG start = 0;
	sLONG end = _GetPlainText().GetLength();

	bool isMixed;
	bool isHidden = _IsRangeHidden( start, end, isMixed);
	if(!isMixed)
	{
		if (isHidden)
			outRanges.push_back( TextRange(start, end));
		return;
	}
	else 
	{
		//context stack (we implement recursivity locally to prevent stack overflow)
		std::vector<TextRange> stackRange;
		std::vector<bool> stackScanNext;

		bool scanNext = false;
		bool doContinue = false;
		do
		{
			doContinue = false;
			sLONG middle = (start+end)/2;
			isHidden = false;
			if (middle > start && !scanNext)
			{
				//scan first half
				isHidden = _IsRangeHidden( start, middle, isMixed);
				if (!isMixed)
				{
					if (isHidden)
						outRanges.push_back(TextRange(start,middle));
				}
				else
				{
					//push context
					stackRange.push_back(TextRange(start,end));
					stackScanNext.push_back(middle < end);

					//iterate recursively on first half
					end = middle;
					doContinue = true;
				}
			}
			scanNext = false;
			if (!doContinue)
			{
				//scan second half
				if (middle < end)
				{
					isHidden = _IsRangeHidden( middle, end, isMixed);
					if (!isMixed)
					{
						if (isHidden)
							outRanges.push_back(TextRange(middle,end));
					}
					else
					{
						//push context
						stackRange.push_back(TextRange(start,end));
						stackScanNext.push_back(false);

						//iterate recursively on second half
						start = middle;
						doContinue = true;
					}
				}

				if (!doContinue)
				{
					while (!stackRange.empty() && !scanNext)
					{
						//restore context
						start = stackRange.back().first;
						end = stackRange.back().second;
						stackRange.pop_back();
						scanNext = stackScanNext.back();
						stackScanNext.pop_back();
					}
				}
			}
		}
		while (!stackRange.empty() || scanNext);
	}
}

/** clear hidden text 
@remarks
	plain text includes hidden text so call this method to remove hidden text
*/
void XWinStyledTextBox::_ClearHidden()
{
	if (!fTextServices)
		return;

	VectorOfTextRange ranges;
	GetRangesHidden( ranges);

	VectorOfTextRange::const_reverse_iterator it = ranges.rbegin();
	for(;it != ranges.rend(); it++)
	{
		ITextRange *textRange = NULL;
		sLONG start = it->first;
		sLONG end = it->second;
		HRESULT hr = fTextDocument->Range((long)(start), (long)(end), &textRange);
		if (FAILED(hr))
			continue;

		textRange->SetText( NULL);
		textRange->Release();
	}
	if (ranges.size() > 0)
	{
		fIsSizeDirty = true;
		fIsPlainTextDirty = true;
	}
}


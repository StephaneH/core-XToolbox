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
#ifndef __VGDI_Debug__
#define __VGDI_Debug__

BEGIN_TOOLBOX_NAMESPACE

#if _DEBUG && ENABLE_GDI_DEBUG==1

#ifdef __cplusplus
extern "C" {
#endif
// util

void gdidbg_SetAssertOnDelete(HGDIOBJ inObj);
void gdidbg_Disable(int objtype); // 0 for all
void gdidbg_Enable(int objtype); // 0 for all


// brush API
HBRUSH _DBG_CreateBrushIndirect(const LOGBRUSH *inLogBrush, LPCTSTR, int);
HBRUSH _DBG_CreateDIBPatternBrush(HGLOBAL hPackedDIB,UINT nUsage, LPCTSTR, int);
HBRUSH _DBG_CreateDIBPatternBrushPt(const void* lpPackedDIB, UINT iUsage, LPCTSTR, int);
HBRUSH _DBG_CreateHatchBrush(int nIndex,COLORREF crColor , LPCTSTR, int);
HBRUSH _DBG_CreatePatternBrush(HBITMAP hbmp, LPCTSTR, int);
HBRUSH _DBG_CreateSolidBrush(COLORREF crCol, LPCTSTR, int);

// pen
HPEN _DBG_CreatePen( int, int, COLORREF, LPCTSTR, int );
HPEN _DBG_CreatePenIndirect(  const LOGPEN *,LPCTSTR,int);
HPEN _DBG_ExtCreatePen( DWORD,DWORD,const LOGBRUSH *,DWORD,const DWORD *,LPCTSTR,int);

//palette
HPALETTE  _DBG_CreatePalette(const LOGPALETTE *, LPCTSTR, int);
HPALETTE  _DBG_CreateHalftonePalette(HDC hdc, LPCTSTR, int);

//font
HFONT    _DBG_CreateFontIndirectA(const LOGFONTA *p1, LPCTSTR, int);
HFONT    _DBG_CreateFontIndirectW(const LOGFONTW *p1, LPCTSTR, int);
#ifdef UNICODE
#define _DBG_CreateFontIndirect  _DBG_CreateFontIndirectW
#else
#define _DBG_CreateFontIndirect  _DBG_CreateFontIndirectA
#endif

HFONT _DBG_CreateFontA(int p1, int p2, int p3, int p4, int p5, DWORD p6,DWORD p7, DWORD p8, DWORD p9, DWORD p10, DWORD p11,DWORD p12, DWORD p13, LPCSTR p14, LPCTSTR, int);
HFONT _DBG_CreateFontW(int p1, int p2, int p3, int p4, int p5, DWORD p6,DWORD p7, DWORD p8, DWORD p9, DWORD p10, DWORD p11,DWORD p12, DWORD p13, LPCWSTR p14, LPCTSTR, int);
#ifdef UNICODE
#define _DBG_CreateFont  _DBG_CreateFontW
#else
#define _DBG_CreateFont  _DBG_CreateFontA
#endif 

// region
HRGN _DBG_CreateEllipticRgn(int p1, int p2, int p3, int p4, LPCTSTR, int);
HRGN _DBG_CreateEllipticRgnIndirect(const RECT *p1, LPCTSTR, int);
HRGN _DBG_CreateRectRgn(int p1, int p2, int p3, int p4, LPCTSTR, int);
HRGN _DBG_CreateRectRgnIndirect(const RECT *p1, LPCTSTR, int);
HRGN _DBG_CreateRoundRectRgn(int p1, int p2, int p3, int p4, int p5, int p6, LPCTSTR, int);
HRGN _DBG_CreatePolygonRgn(const POINT *p1, int p2, int p3, LPCTSTR, int);
HRGN _DBG_CreatePolyPolygonRgn(const POINT *p1, const INT *p2, int p3, int p4, LPCTSTR, int);
HRGN _DBG_ExtCreateRegion(const XFORM *p1, DWORD p2, const RGNDATA *p3, LPCTSTR, int);
HRGN _DBG_PathToRegion(HDC p1, LPCTSTR, int);

// dc
HDC _DBG_CreateCompatibleDC(HDC p1, LPCTSTR, int);
HDC _DBG_CreateDCA(LPCSTR p1, LPCSTR p2, LPCSTR p3, const DEVMODEA *p4, LPCTSTR, int);
HDC _DBG_CreateDCW(LPCWSTR p1, LPCWSTR p2, LPCWSTR p3, const DEVMODEW *p4, LPCTSTR, int);
HDC _DBG_CreateICA(LPCSTR, LPCSTR , LPCSTR , const DEVMODEA *, LPCTSTR, int);
HDC _DBG_CreateICW(LPCWSTR, LPCWSTR , LPCWSTR , const DEVMODEW *, LPCTSTR, int);
#ifdef UNICODE
#define _DBG_CreateIC  _DBG_CreateICW
#else
#define _DBG_CreateIC  _DBG_CreateICA
#endif

BOOL _DBG_DeleteDC( HDC Object, LPCTSTR File, int Line );
BOOL _DBG_ReleaseDC( HWND hWnd, HDC hDc, LPCTSTR File, int Line );

HDC _DBG_GetDC( HWND, LPCTSTR, int );
HDC _DBG_GetDCEx( HWND, HRGN, DWORD, LPCTSTR, int );

#ifdef UNICODE
#define _DBG_CreateDC  _DBG_CreateDCW
#else
#define _DBG_CreateDC  _DBG_CreateDCA
#endif

// bitmap
HBITMAP _DBG_CreateBitmap(int, int, UINT, UINT, const void *, LPCTSTR, int);
HBITMAP _DBG_CreateBitmapIndirect(const BITMAP *, LPCTSTR, int);
HBITMAP _DBG_CreateDIBitmap(HDC, const BITMAPINFOHEADER *, DWORD, const void *, const BITMAPINFO *, UINT, LPCTSTR, int);
HBITMAP _DBG_CreateDIBSection(HDC p1, const BITMAPINFO * p2	, UINT p3, void ** p4, HANDLE p5, DWORD p6, LPCTSTR file, int line);
HBITMAP _DBG_CreateCompatibleBitmap(HDC, int, int, LPCTSTR, int);
HBITMAP _DBG_CreateDiscardableBitmap(HDC, int, int, LPCTSTR, int);
HBITMAP _DBG_LoadBitmapA( HINSTANCE, LPCSTR, LPCTSTR, int );
HBITMAP _DBG_LoadBitmapW( HINSTANCE, LPCWSTR, LPCTSTR, int );
#ifdef UNICODE
#define _DBG_LoadBitmap  _DBG_LoadBitmapW
#else
#define _DBG_LoadBitmap  _DBG_LoadBitmapA
#endif // !

BOOL _DBG_DeleteObject(HGDIOBJ obj,LPCTSTR, int);
HGDIOBJ _DBG_SelectObject(HDC indc,HGDIOBJ obj,LPCTSTR, int);

#ifdef __cplusplus
}
#endif

#ifndef __NO_GDIDEBUG_INCLUDE__
	// brush API
	#define CreateBrushIndirect( p1 ) _DBG_CreateBrushIndirect( p1, __FILE__, __LINE__ )
	#define CreateDIBPatternBrush( p1, p2 ) _DBG_CreateDIBPatternBrush( p1, p2, __FILE__, __LINE__  )
	#define CreateDIBPatternBrushPt( p1, p2 ) _DBG_CreateDIBPatternBrushPt( p1, p2, __FILE__, __LINE__ )
	#define CreateHatchBrush( p1, p2 ) _DBG_CreateHatchBrush( p1, p2, __FILE__, __LINE__  )
	#define CreateSolidBrush( p1 ) _DBG_CreateSolidBrush( p1, __FILE__, __LINE__  )
	#define CreatePatternBrush( p1 ) _DBG_CreatePatternBrush( p1, __FILE__, __LINE__  )
	
	// pen
	#define CreatePen( p1, p2, p3) _DBG_CreatePen( p1, p2, p3, __FILE__, __LINE__ )
	#define CreatePenIndirect(  p1) _DBG_CreatePenIndirect(  p1,__FILE__, __LINE__)
	#define ExtCreatePen( p1,p2,p3,p4,p5) _DBG_ExtCreatePen( p1,p2,p3,p4,p5,__FILE__, __LINE__)
	
	//palette
	#define CreatePalette(p1) _DBG_CreatePalette(p1,__FILE__, __LINE__)
	#define CreateHalftonePalette(p1) _DBG_CreateHalftonePalette(p1,__FILE__, __LINE__)
	
	// font
	#define CreateFontIndirectA(p1)  _DBG_CreateFontIndirectA(p1,__FILE__, __LINE__)
	#define CreateFontIndirectW(p1) _DBG_CreateFontIndirectW(p1,__FILE__, __LINE__)
	#define CreateFontA(p1, p2, p3, p4, p5, p6,p7, p8, p9, p10, p11,p12, p13, p14) _DBG_CreateFontA(p1, p2, p3, p4, p5, p6,p7, p8, p9, p10, p11,p12, p13, p14 ,__FILE__, __LINE__)
	#define	CreateFontW(p1, p2, p3, p4, p5, p6,p7, p8, p9, p10, p11,p12, p13, p14) _DBG_CreateFontW(p1, p2, p3, p4, p5, p6,p7, p8, p9, p10, p11,p12, p13, p14,__FILE__, __LINE__)
	
	// region
	#define CreateEllipticRgn( p1,  p2,  p3,  p4) _DBG_CreateEllipticRgn( p1,  p2,  p3,  p4, __FILE__, __LINE__)
	#define CreateEllipticRgnIndirect(p1) _DBG_CreateEllipticRgnIndirect(p1, __FILE__, __LINE__)
	#define CreateRectRgn( p1,  p2,  p3,  p4) _DBG_CreateRectRgn( p1,  p2,  p3,  p4, __FILE__, __LINE__)
	#define CreateRectRgnIndirect( p1) _DBG_CreateRectRgnIndirect(p1, __FILE__, __LINE__)
	#define CreateRoundRectRgn( p1,  p2,  p3,  p4,  p5,  p6) _DBG_CreateRoundRectRgn( p1,  p2,  p3,  p4,  p5,  p6, __FILE__, __LINE__)
	#define CreatePolygonRgn(p1,  p2,  p3) _DBG_CreatePolygonRgn(p1,  p2,  p3, __FILE__, __LINE__)
	#define CreatePolyPolygonRgn(p1, p2,  p3,  p4) _DBG_CreatePolyPolygonRgn(p1, p2,  p3,  p4, __FILE__, __LINE__)
	#define ExtCreateRegion(p1,  p2, p3) _DBG_ExtCreateRegion(p1,  p2, p3, __FILE__, __LINE__)
	#define PathToRegion( p1) _DBG_PathToRegion(p1, __FILE__, __LINE__)
	
	// DC
	#define CreateCompatibleDC(p1) _DBG_CreateCompatibleDC(p1, __FILE__, __LINE__)
	#define CreateDCA(p1,p2,p3,p4) _DBG_CreateDCA(p1,p2,p3,p4, __FILE__, __LINE__)
	#define CreateDCW( p1,p2,p3,p4) _DBG_CreateDCW( p1,p2,p3,p4, __FILE__, __LINE__)
	#define CreateICA(p1, p2 , p3 , p4) _DBG_CreateICA(p1, p2 , p3 , p4, __FILE__, __LINE__)
	#define CreateICW(p1, p2 , p3 , p4) _DBG_CreateICW(p1, p2 , p3 , p4, __FILE__, __LINE__)

	#define DeleteDC( p1) _DBG_DeleteDC( p1, __FILE__, __LINE__ )
	#define ReleaseDC( p1, p2) _DBG_ReleaseDC( p1, p2, __FILE__, __LINE__)
	
	#define GetDC( p1 ) _DBG_GetDC( p1,__FILE__, __LINE__)
	#define GetDCEx( p1, p2, p3, LPSTR, int ) _DBG_GetDCEx( p1, p2, p3, __FILE__, __LINE__)
	
	// bitmap
	#define CreateBitmap(p1, p2, p3, p4, p5) _DBG_CreateBitmap(p1, p2, p3, p4, p5, __FILE__, __LINE__)
	#define CreateBitmapIndirect(p1) _DBG_CreateBitmapIndirect(p1, __FILE__, __LINE__)
	#define CreateDIBitmap(p1, p2, p3, p4, p5, p6) _DBG_CreateDIBitmap(p1, p2, p3, p4, p5, p6, __FILE__, __LINE__)
	#define CreateDIBSection(p1, p2, p3, p4, p5, p6) _DBG_CreateDIBSection(p1, p2, p3, p4, p5, p6 ,__FILE__, __LINE__)
	#define CreateCompatibleBitmap(p1, p2, p3) _DBG_CreateCompatibleBitmap(p1, p2, p3, __FILE__, __LINE__)
	#define CreateDiscardableBitmap(p1, p2, p3) _DBG_CreateDiscardableBitmap(p1, p2, p3, __FILE__, __LINE__)
	#define LoadBitmapA( p1, p2) _DBG_LoadBitmapA( p1, p2, __FILE__, __LINE__ )
	#define LoadBitmapW( p1, p2) _DBG_LoadBitmapW( p1, p2, __FILE__, __LINE__ )
	
	#define DeleteObject(p1) _DBG_DeleteObject(p1,__FILE__, __LINE__)
	#define SelectObject(p1,p2) _DBG_SelectObject(p1,p2,__FILE__, __LINE__)
#endif	// __NO_GDIDEBUG_INCLUDE__

#else	// _DEBUG

	#define gdidbg_SetAssertOnDelete(x)
	#define gdidbg_Disable(x);
	#define gdidbg_Enable(x); // 0 for all
	
#endif	// _DEBUG

END_TOOLBOX_NAMESPACE

#endif
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
#ifndef __VGraphicsTypes__
#define __VGraphicsTypes__

#include "KernelIPC/Sources/VKernelIPCTypes.h"
#include <cmath> 

/* enable/disable support for custom fonts substitutes */
#define	 ENABLE_CUSTOM_FONTS_SUBSTITUTES 1

#if VERSIONWIN

namespace Gdiplus
{
	class Color;
	class Font;
	class Pen;
	class Brush;
	class SolidBrush;
	class LinearGradientBrush;
	class PathGradientBrush;
	class Matrix;
	class GraphicsPath;
	class Bitmap;
	class Graphics;
	class Metafile;
	class Region;
	class PointF;
	class RectF;
	class Image;
	class ImageCodecInfo;
	enum FillMode;
	typedef UINT	GraphicsState;
	typedef INT		PixelFormat;
}

//set 1 to use Direct2D impl on Vista/Win7
//(set to 0 to force Gdiplus impl on Vista/Win7)
#define ENABLE_D2D 1

#if ENABLE_D2D
//it is preferable to set this constant to 1 in order to ensure 
//we can share resources like ID2D1Geometry between D2D devices
#define D2D_FACTORY_MULTI_THREADED 1

//set to 1 to ensure D2D is compatible with GDI, which means:
//- 1 DIP = 1 GDI pixel so for x = 10 in GDI, x = 10 also in D2D (if page scaling is 1.0f)
//- no scaling by default while rendering D2D render target on screen - otherwise we would have a scaling to screen DPI/96.0f
//- DWrite font size in DIP is equal to GDI & GDIPlus font size in Pixel
//- while using GDI commands, drawing/reading will use render target gdi-compatible dc if between BeginUsingContext & EndUsingContext
//  in order to be able to draw with GDI over Direct2D content 
//caution: MUST always be set to 1 for 4D app (otherwise big pb...)
#define D2D_GDI_COMPATIBLE 1

#endif

#ifndef _D2D1_H_
//Direct2D forward references:
//NDJQ: d2d1.h is included only while building Graphics 
//		in order to avoid to link all modules with d2d1.lib & compilation errors with Altura 
//		so please do not remove these forward references
interface ID2D1RenderTarget;
interface ID2D1BitmapRenderTarget;
interface ID2D1Bitmap;
interface ID2D1Geometry;
interface ID2D1GeometrySink;
interface ID2D1StrokeStyle;
interface ID2D1GradientStopCollection;
interface IDWriteFont;
interface IDWriteTextFormat;
interface IDWriteTextLayout;
interface IDWriteInlineObject;
interface IDWriteLocalizedStrings;
struct DWRITE_LINE_METRICS;
#endif
#ifndef __wincodec_h__
interface IWICBitmap;
interface IWICBitmapSource;
#endif
BEGIN_TOOLBOX_NAMESPACE
class VWinD2DGraphicContext;
END_TOOLBOX_NAMESPACE
#endif

#if VERSIONMAC
#include <ApplicationServices/ApplicationServices.h>
#define ENABLE_D2D 0

// sdk 10.7 doesn't contain QuickDraw APIs
#if ARCH_64 || (__MAC_OS_X_VERSION_MAX_ALLOWED >= 1070)
#define WITH_QUICKDRAW 0
#else
#define WITH_QUICKDRAW 1
#endif
#endif


BEGIN_TOOLBOX_NAMESPACE

#if VERSIONMAC
typedef CGFloat GReal;
#define GREAL_IS_DOUBLE	CGFLOAT_IS_DOUBLE
#else
typedef float	GReal;
#define GREAL_IS_DOUBLE	0
#endif

// Tolerance value that allows to consider 2 pixels equal.
//
// CAUTION! As coords are specified as float, you should care about
// the following (especially if your frames are to be zoomed).
//
// Never compare two values directly such as x == y or GetHeight() > 0
// but instead use the kREAL_PIXEL_PRECISION constant.
//
// Don't write 'GetWidth() == 2.0' but 'Abs(GetWidth() - 2.0) < kREAL_PIXEL_PRECISION'
// Don't write 'GetWidth() > 2.0' but '(GetWidth() - 2.0) > kREAL_PIXEL_PRECISION'
// Don't write 'GetWidth() <= 2.0' but '(GetWidth() - 2.0) < kREAL_PIXEL_PRECISION'
// Don't write 'GetWidth() < 2.0' but '(GetWidth() - 2.0) < -kREAL_PIXEL_PRECISION'
// Don't write 'GetWidth() >= 2.0' but '(GetWidth() - 2.0) > -kREAL_PIXEL_PRECISION'
// Note the MINUS SIGN before kREAL_PIXEL_PRECISION in the last two cases
//
const GReal	kREAL_PIXEL_PRECISION	= (GReal) 0.000001;
const GReal	kPRECISION_PIXEL	= kREAL_PIXEL_PRECISION;	// Alias


// Limit values for QD and GDI coord spaces
const sLONG	kMAX_GDI_RGN_SIZE	= 268435455;	// 28-bit unsigned value (0x0FFFFFFF)
const GReal	kMAX_QD_RGN_SIZE	= kMAX_uWORD;	// 16-bit unsigned value (0xFFFF)
const sLONG	kMAX_GDI_RGN_COORD	= 134217727;	// 28-bit signed value (0x07FFFFFF)
const GReal	kMAX_QD_RGN_COORD	= kMAX_sWORD;	// 16-bit signed value (0x7FFF)
const sLONG	kMIN_GDI_RGN_COORD	= -134217727;	// 28-bit signed value (0x08000000)
const GReal	kMIN_QD_RGN_COORD	= kMIN_sWORD;	// 16-bit signed value (0x8000)


// Cross-platform alias types
#if VERSIONMAC
	typedef RGBColor		ColorRef;
	typedef HIShapeRef		RgnRef;
	typedef GrafPtr			PortRef;
	typedef CGContextRef	ContextRef;
	typedef	CGRect			RectRef;
	typedef CTFontRef		FontRef;
	typedef CGShadingRef	GradientRef;
	typedef CGPatternRef	PatternRef;
#ifndef COMPILING_XTOOLBOX_LIB
	typedef ::IconRef		IconRef;
#endif
	// Note1: IconRef, MenuRef, ControlRef and WindowRef are used as is
	// Note2: ControlRef and HwndRef are fully equivalent
#elif VERSIONWIN
	typedef COLORREF	ColorRef;
	typedef HRGN		RgnRef;
	typedef HDC			PortRef;
	typedef HDC			ContextRef;
	typedef	RECT		RectRef;
	typedef HICON		IconRef;
	typedef HGDIOBJ		GradientRef;
	typedef HGDIOBJ		PatternRef;

	// Note1: HwndRef, ControlRef and WindowRef are fully equivalent
	// Note2: ContextRef and PortRef are fully equivalent
#if USE_GDIPLUS
	typedef Gdiplus::Font*	FontRef;
#else
	typedef HFONT			FontRef;
#endif
#endif

// Standard color constants
enum StdColor {
	COL_WHITE	= 0,	// Plain colors
	COL_GRAY5,
	COL_GRAY10,
	COL_GRAY20,
	COL_GRAY30,
	COL_GRAY40,
	COL_GRAY50,
	COL_GRAY60,
	COL_GRAY70,
	COL_GRAY80,
	COL_GRAY90,
	COL_BLACK,
	COL_RED,
	COL_GREEN,
	COL_BLUE,
	COL_YELLOW,
	COL_MAGENTA,
	COL_CYAN,
	COL_GRAY,
	COL_GRAY_LITE,
	COL_GRAY_HILITE,
	COL_GRAY_SHADOW,
	COL_GRAY_DKSHADOW,
	COL_LAST_PLAIN_COLOR = COL_GRAY_DKSHADOW,
	COL_PLAIN_BACKGROUND,		// Look handler colors
	COL_PLAIN_TEXT,
	COL_CONTROL_TITLE_ACTIVE,	// Buttons, tabs, status-bar, etc
	COL_CONTROL_TITLE_INACTIVE,
	COL_CONTROL_TITLE_DISABLED,
	COL_LIST_TEXT_ACTIVE,		// List view, grids, etc
	COL_LIST_TEXT_INACTIVE,
	COL_LIST_TEXT_DISABLED,
	COL_LIST_TEXT_HILITED,
	COL_LIST_TEXT_HILITED_INACTIVE,
	COL_LIST_TEXT_HILITED_DISABLED,
	COL_LIST_BACKGROUND,
	COL_LIST_BACKGROUND_ALTERNATE,
	COL_LIST_BACKGROUND_SORTED,
	COL_LIST_BACKGROUND_HILITED,
	COL_LIST_BACKGROUND_HILITED_INACTIVE,
	COL_LIST_BACKGROUND_HILITED_DISABLED,
	COL_LIST_H_SEPARATOR,
	COL_LIST_H_SEPARATOR_HILITED,
	COL_LIST_H_SEPARATOR_HILITED_INACTIVE,
	COL_LIST_H_SEPARATOR_HILITED_DISABLED,
	COL_LIST_V_SEPARATOR,
	COL_LIST_V_SEPARATOR_HILITED,
	COL_LIST_V_SEPARATOR_HILITED_INACTIVE,
	COL_LIST_V_SEPARATOR_HILITED_DISABLED,
	COL_EDIT_TEXT_ACTIVE,		// All plain text zones
	COL_EDIT_TEXT_INACTIVE,
	COL_EDIT_TEXT_DISABLED,
	COL_EDIT_TEXT_HILITED,
	COL_EDIT_TEXT_HILITED_INACTIVE,
	COL_EDIT_TEXT_HILITED_DISABLED,
	COL_EDIT_TEXT_BACK,
	COL_EDIT_TEXT_HILITED_BACK,
	COL_EDIT_TEXT_HILITED_INACTIVE_BACK,
	COL_EDIT_TEXT_HILITED_DISABLED_BACK,
	COL_TIPS_TEXT,
	COL_TIPS_BACKGROUND,
	COL_TEXT_MENU,				// Obsolete. Use LookHandler primives.
	COL_TEXT_MENU_HI,
	COL_MENU_BACK,
	COL_MENU_HI_BACK,
	COL_MENU_HI_LIGHT_BORDER,
	COL_MENU_HI_DARK_BORDER,
	COL_PLAIN = COL_PLAIN_BACKGROUND,	// Old names. Use previous ones please.
	COL_TEXT = COL_PLAIN_TEXT,
	COL_TEXT_BUTTON = COL_CONTROL_TITLE_ACTIVE,
	COL_TEXT_BACK = COL_EDIT_TEXT_BACK,
	COL_TEXT_HI = COL_EDIT_TEXT_HILITED,
	COL_TEXT_DIS = COL_EDIT_TEXT_DISABLED,
	COL_TEXT_HI_BACK = COL_EDIT_TEXT_HILITED_BACK,
	COL_TIPS = COL_TIPS_TEXT,
	COL_TIPS_BACK = COL_TIPS_BACKGROUND
};


// Standard pattern constants
typedef enum PatternStyle {
	PAT_FILL_NONE	= -1,
	PAT_FILL_PLAIN	= 0,	// See specific PDF file for sample
	PAT_FILL_EMPTY,
	PAT_FILL_CHECKWORK_8,
	PAT_FILL_CHECKWORK_4,
	PAT_FILL_GRID_8,
	PAT_FILL_GRID_4,
	PAT_FILL_GRID_FAT_8,
	PAT_FILL_GRID_FAT_4,
	PAT_FILL_H_HATCH_8,
	PAT_FILL_V_HATCH_8,
	PAT_FILL_HV_HATCH_8,
	PAT_FILL_L_HATCH_8,
	PAT_FILL_R_HATCH_8,
	PAT_FILL_H_HATCH_FAT_8,
	PAT_FILL_V_HATCH_FAT_8,
	PAT_FILL_HV_HATCH_FAT_8,
	PAT_FILL_L_HATCH_FAT_8,
	PAT_FILL_R_HATCH_FAT_8,
	PAT_FILL_LAST	= PAT_FILL_R_HATCH_FAT_8,
	PAT_LINE_PLAIN	= 1000,
	PAT_LINE_DOT_LARGE,
	PAT_LINE_DOT_MEDIUM,
	PAT_LINE_DOT_SMALL,
	PAT_LINE_DOT_MINI,
	PAT_LINE_AXIS_MEDIUM,
	PAT_LINE_AXIS_SMALL,
	PAT_LINE_DBL_SMALL,
	PAT_LINE_DBL_MEDIUM,
	PAT_LINE_LAST	= PAT_LINE_DBL_MEDIUM,
	
#if USE_OBSOLETE_API
	PAT_HOLLOW	= PAT_FILL_EMPTY,
	PAT_PLAIN	= PAT_FILL_PLAIN,
	PAT_H_HATCH	= PAT_FILL_H_HATCH_FAT_8,
	PAT_V_HATCH	= PAT_FILL_V_HATCH_FAT_8,
	PAT_HV_HATCH	= PAT_FILL_HV_HATCH_FAT_8,
	PAT_R_HATCH	= PAT_FILL_R_HATCH_FAT_8,
	PAT_L_HATCH	= PAT_FILL_L_HATCH_FAT_8,
	PAT_WHITE	= PAT_FILL_EMPTY,
	PAT_BLACK	= PAT_FILL_PLAIN,
	PAT_LITE_GRAY,
	PAT_MED_GRAY,
	PAT_DARK_GRAY,
	PAT_LR_HATCH
#endif
} PatternStyle;


typedef enum GradientStyle {
	GRAD_STYLE_LINEAR	= 0,
	GRAD_STYLE_GAUSSIAN,
	GRAD_STYLE_LUT,			//multicolor gradient with look-up color table 
							//(slow but accurate version : linear interpolate color from neighbouring colors in LUT)
	GRAD_STYLE_LUT_FAST		//multicolor gradient with look-up color table 
							//(fast but less accurate version : pick nearest color in LUT )
} GradientStyle;


// Standard cursor constants
typedef enum CursorStyle { 
	CURS_STARTUP	= -2,	// Special value for module startup
	CURS_SYSTEM		= -1,	// Special value for non-client area under Windows
	CURS_ARROW		= 0,
	CURS_TEXT,
	CURS_THIN_CROSS,
	CURS_THICK_CROSS,
	CURS_WATCH,
	CURS_CLIC,
	CURS_MOVE,
	CURS_MOVE_KNOB,
	CURS_SIZE_H,
	CURS_SIZE_V,
	CURS_SIZE_TL_BR,
	CURS_SIZE_BL_TR,
	CURS_DROP,
	CURS_SIZE_COL_V,
	CURS_SIZE_COL_H,
	CURS_SIZE_COLUMN = CURS_SIZE_COL_V,
	CURS_SIZE_ROW = CURS_SIZE_COL_H,
	CURS_SPLIT_V,
	CURS_SPLIT_H,
	CURS_ERASER,
	CURS_OPENED_HAND,
	CURS_CLOSED_HAND,
	CURS_CONTEXTUAL_MENU,
	CURS_COPY,
	CURS_ALIAS,
	CURS_HELP,
	CURS_FORBIDDEN,
	CURS_ORIENTED_TO_RIGHT_ARROW,
	CURS_AUTO,
	CURS_ZOOM_IN,
	CURS_ZOOM_OUT,
	CURS_INACTIVE_WINDOW,
	CURS_LAYOUT_GOOD_REORDER,
	CURS_LAYOUT_BAD_REORDER,
	CURS_OLD_DRAG,
	CURS_RESIZE_LEFT,
	CURS_RESIZE_RIGHT,
	CURS_RESIZE_UP,
	CURS_RESIZE_DOWN
} CursorStyle;


// Standard font constants
typedef enum StdFont {
	STDF_NONE	= 0,
	STDF_APPLICATION,
	STDF_SYSTEM,		// Default button font (radio/checkbox/popup)
	STDF_SYSTEM_SMALL,	// Small button font (radio/checkbox/popup)
	STDF_SYSTEM_MINI,	// Mini button font (radio/checkbox/popup)
	STDF_TEXT,			// Default text editor font
	STDF_EDIT,			// Default edit frame font
	STDF_LIST,			// Default listing font
	STDF_MENU,			// Default menu font
	STDF_MENU_MINI,		// Small menus font
	STDF_PUSH_BUTTON,	// Default push button font
	STDF_ICON,			// Default toolbar font
	STDF_CAPTION,		// Default caption/groupbox font (used for views as default)
	STDF_TIP,			// Default tip font
	STDF_WINDOW_SMALL_CAPTION,	// Internal use only
	STDF_WINDOW_CAPTION,
	STDF_LAST
} StdFont;


// Font face constants (may be used as masks)
typedef uLONG	VFontFace;
enum {
	KFS_NORMAL		= 0,
	KFS_ITALIC		= 1,
	KFS_BOLD		= 2,
	KFS_UNDERLINE	= 4,
	KFS_EXTENDED	= 8,
	KFS_CONDENSED	= 16,
	KFS_SHADOW		= 32,
	KFS_OUTLINE		= 64,
	KFS_STRIKEOUT	= 128
};


// Generic alignment constants
typedef enum AlignStyle {
	AL_LEFT	= 0,
	AL_TOP = AL_LEFT,
	AL_CENTER,
	AL_RIGHT,
	AL_BOTTOM = AL_RIGHT,
	AL_JUST,
	AL_BASELINE,		//used only for embedded object vertical alignment
	AL_SUPER,			//superscript: used only for embedded object vertical alignment
	AL_SUB,				//subscript: used only for embedded object vertical alignment
	AL_DEFAULT,
	AL_NONE				//means that alignment is not defined and should be ignored
} AlignStyle;


// Standard pen style constants
typedef enum DashStyle
{
	PEN_HOLLOW	= 0,
	PEN_NORMAL,
	PEN_DASH,
	PEN_DOT,
	PEN_DASH_DOT
}	DashStyle;


// Generic draw states
typedef enum PaintStatus {
	PS_NONE	= -1,	// Special values to use as param
	PS_USE_VIEW_STATE	= -2,
	PS_ACTIVE = 0,	// Default enabled state
	PS_INACTIVE,	// Default enabled state in inactive window
	PS_HILITED,		// For push or toggle buttons while tracking
	PS_DISABLED,	// Disabled states
	PS_DISABLED_INACTIVE,
	PS_ROLLOVER,	// For buttons, links, etc
	PS_SELECTED,	// For icons, text, etc
	
	// For radios, checks, toggle buttons, etc
	PS_OFF	= PS_ACTIVE,
	PS_INACTIVE_OFF	= PS_INACTIVE,
	PS_DISABLED_OFF	= PS_DISABLED,
	PS_PUSHED_OFF	= PS_HILITED,
	PS_ROLLOVER_OFF	= PS_ROLLOVER,
	PS_DISABLED_INACTIVE_OFF = PS_DISABLED_INACTIVE,
	
	PS_ON	= PS_SELECTED,
	PS_INACTIVE_ON,
	PS_DISABLED_ON,
	PS_PUSHED_ON,
	PS_ROLLOVER_ON,
	PS_DISABLED_INACTIVE_ON,
	
	PS_MIXED,
	PS_INACTIVE_MIXED,
	PS_DISABLED_MIXED,
	PS_PUSHED_MIXED,
	PS_ROLLOVER_MIXED,
	PS_DISABLED_INACTIVE_MIXED,
	
	// Aliases
	PS_NORMAL	= PS_ACTIVE,
	PS_PUSHED	= PS_HILITED,
	
#if USE_OBSOLETE_API
	PS_GROUP_MIXED = PS_PUSHED_MIXED,
	PS_RADIO = PS_SELECTED
#endif
} PaintStatus;


// Transfert mode constants
typedef enum TransferMode {
	TM_COPY		= 0,
	TM_OPAQUE	= TM_COPY, 
	TM_OR,
	TM_MIXED	= TM_OR, 
	TM_XOR,
	TM_INVERT	= TM_XOR, 
	TM_ERASE,
	TM_NOT_COPY,
	TM_NOT_OR,
	TM_NOT_XOR,
	TM_NOT_ERASE, 
	TM_TRANSPARENT,
	TM_BLEND,
	TM_AND
} TransferMode;


// Drawing mode constants
typedef enum DrawingMode {
	DM_NORMAL	= 0,
	DM_HILITE,
	DM_SHADOW,
	DM_CLIP,				// Not drawn just clipped (text mode)
	DM_OUTLINED,			// Framed (text mode)
	DM_FILLED,				// Framed and Filled (text mode)
	DM_PLAIN = DM_NORMAL	// Filled (text mode)
} DrawingMode;


/** fill rule type definition */
typedef enum FillRuleType
{
	FILLRULE_NONZERO = 0, //default
	FILLRULE_EVENODD
} FillRuleType;

// options to measure text
typedef uLONG	TextMeasuringMode;
enum {
	TMM_NORMAL							= 0,
	TMM_GENERICTYPO						= 1,		// gdiplus only  : use  StringFormat::GenericTypographic() as defaut format.
};

// options to layout some text.
typedef uLONG	TextLayoutMode;
enum {
	TLM_NORMAL							= 0,
	TLM_DONT_WRAP						= 1,		// DrawTextBox only: Indicates that text should not be wrapped.
	TLM_TRUNCATE_MIDDLE_IF_NECESSARY	= 2,		// DrawTextBox only: truncate text in the middle (with '...') to fit specified width (needs TLM_DONT_WRAP)
	TLM_TEXT_POS_UPDATE_NOT_NEEDED		= 4,		// DrawText only: performance hint: DrawText is not required to update the graphic context text position. 
	TLM_VERTICAL						= 8,		// DrawText + DrawTextBox: draw text vertically from top to bottom and right to left (traditional chinese/japanese/korean layout)
	TLM_LEFT_TO_RIGHT					= 16,		// DrawText + DrawTextBox: force left to right base writing direction (overriding default bidirectional algorithm behavior)
	TLM_RIGHT_TO_LEFT					= 32,		// DrawText + DrawTextBox: force right to left base writing direction (overriding default bidirectional algorithm behavior)
	TLM_ALIGN_LEFT						= 64,		// DrawTextBox only: paragraph left horizontal alignment (used to precompute paragraph style in Mac OS CoreText implementation)
	TLM_ALIGN_RIGHT						= 128,		// DrawTextBox only: paragraph right horizontal alignment (used to precompute paragraph style in Mac OS CoreText implementation)
	TLM_ALIGN_CENTER					= 256,		// DrawTextBox only: paragraph center horizontal alignment (used to precompute paragraph style in Mac OS CoreText implementation)
	TLM_ALIGN_JUSTIFY					= 512,		// DrawTextBox only: paragraph justify horizontal alignment (used to precompute paragraph style in Mac OS CoreText implementation)
	TLM_TRUNCATE_END_IF_NECESSARY		= 1024,		// DrawTextBox only: truncate text in the end (with '...') to fit specified width (needs TLM_DONT_WRAP)
	TLM_D2D_IGNORE_OVERHANGS			= 2048,		// disable overhang adjustment: on default, text bounds calc & drawing methods take account Direct2D text box overhangs (to avoid text to be cropped)
													// (disable it for accurate text drawing at text kerning boundary - for instance if you draw contiguous text runs)
	TRM_LEGACY_MONOSTYLE				= 4096,		// window only int text services without TXTBIT_RICHTEXT, workarround for japanese bug
	TLM_LEGACY_MULTILINE				= 8192		// window only DrawTextW without DT_SINGLELINE
};
#define TLM_PARAGRAPH_STYLE_ALIGN_MASK	(TLM_ALIGN_LEFT|TLM_ALIGN_RIGHT|TLM_ALIGN_CENTER|TLM_ALIGN_JUSTIFY)
#define TLM_PARAGRAPH_STYLE_MASK		(TLM_DONT_WRAP|TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY|TLM_LEFT_TO_RIGHT|TLM_RIGHT_TO_LEFT|TLM_PARAGRAPH_STYLE_ALIGN_MASK)		

/** tab stop type (alias for eTabStopType) */
typedef enum eTextTabStopType
{
	TTST_LEFT,
	TTST_RIGHT,
	TTST_CENTER,
	TTST_DECIMAL,
	TTST_BAR
} eTextTabStopType;

/** graphic context type definition */
typedef enum GraphicContextType
{
	//on Windows Vista+, D2D graphic context (if D2D is available & enabled) (hardware if available)
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	eDefaultGraphicContext = 0,

	//on Windows Vista+, D2D software graphic context (if D2D is available & enabled) 
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	eSoftwareGraphicContext,

	//on Windows Vista+, D2D graphic context (if D2D is available & enabled) (hardware if available)
	//on Windows XP, Gdiplus graphic context
	//on Mac OS, Quartz2D graphic context
	//(will switch on software only graphic context if hardware is not available)
	eHardwareGraphicContext,

#if VERSIONWIN
	eWinGDIGraphicContext,
	eWinGDIPlusGraphicContext,
	eWinD2DSoftwareGraphicContext,
	eWinD2DHardwareGraphicContext,
	ePrinterGraphicContext = eWinGDIGraphicContext,
#endif
#if VERSIONMAC
	eMacQuartz2DGraphicContext = eDefaultGraphicContext,
	ePrinterGraphicContext = eDefaultGraphicContext,
#endif

	eCountGraphicContext

} GraphicContextType;


// Options for VFontMetrics
typedef uLONG	TextRenderingMode;
enum {
	TRM_NORMAL					= 0,			// anti-aliasing according to user & system settings
	TRM_WITH_ANTIALIASING		= 1,			// force anti-antialiasing ON
	TRM_WITHOUT_ANTIALIASING	= 2,			// force anti-antialiasing OFF
	TRM_CONSTANT_SPACING		= 4,			// Ensures that GetTextWidth will produce the same width as multiple calls to on individual characters

	TRM_WITH_ANTIALIASING_CLEARTYPE	= 1,		// force cleartype antialiasing
	TRM_WITH_ANTIALIASING_NORMAL = 8,			// force classic antialiasing
	
	//legacy mode: default is auto ie legacy rendering mode (GDI rendering) is used only if font is not truetype in GDIPlus & Direct2D impl
	TRM_LEGACY_ON				= 16,			// force legacy rendering mode (use only GDI rendering) - Windows only 
	TRM_LEGACY_OFF				= 32			// disable legacy rendering mode (use only impl text rendering: if font is not truetype, it is substituted with truetype font in GDIPlus & Direct2D impl) - Windows only
};


// Line styles
typedef enum CapStyle {
	CS_BUTT	= 0,
	CS_ROUND,
	CS_SQUARE,
	CS_DEFAULT	= CS_BUTT
} CapStyle;


typedef enum JoinStyle {
	JS_MITER	= 0,
	JS_ROUND,
	JS_BEVEL,
	JS_DEFAULT	= JS_MITER
} JoinStyle;


typedef enum PictureMosaic {
	PM_NONE		= 0,
	PM_TOPLEFT,
	PM_CENTER,
	PM_TILE,
	PM_SCALE_EVEN,
	PM_SCALE_TO_FIT,
	PM_SCALE_CENTER,
	PM_4D_SCALE_EVEN,
	PM_4D_SCALE_CENTER,
	PM_LAST = PM_4D_SCALE_CENTER,
	PM_MATRIX
} PictureMosaic;


typedef enum ImageKind {
	IK_NONE	= 0,
	IK_FIRST_VECTORIAL,
	IK_PICT = IK_FIRST_VECTORIAL,
	IK_EMF,
	IK_WMF,
	IK_DXF,
	IK_EPSF,
	IK_PDF,
	IK_QUARTZ,
	IK_LAST_VECTORIAL = IK_EPSF,
	IK_FIRST_BITMAP = 100,
	IK_BMP = IK_FIRST_BITMAP,
	IK_GIF,
	IK_PCX,
	IK_JPEG,
	IK_TIFF,
	IK_QTIME,
	IK_PIXMAP,
	IK_LAST_BITMAP = IK_PIXMAP
} ImageKind;


typedef enum DrawingUnit {
	DU_PIXELS	= 0,
	DU_TWIPS,
	DU_MICRONS
} DrawingUnit;


typedef enum VBitmapTransparencyMode {
	kBITMAP_OPAQUE	= 0, 
	kBITMAP_USEALPHA, 
	kBITMAP_USEPIXEL, 
	kBITMAP_USECOLOR
} VBitmapTransparencyMode;


typedef enum VPixelFormat {
	kPIXEL_UNKNOWN, 
	kPIXEL_GRAYSCALE, 	// 8 bits
	kPIXEL_RGB, 		// 32 bits
	kPIXEL_RGBA, 	// 32 bits
	kPIXEL_PRGBA	// 32 bits
} VPixelFormat;


typedef struct VPixelColor
{
#if VERSIONMAC
	uBYTE	fAlpha;
	uBYTE	fRed;
	uBYTE	fGreen;
	uBYTE	fBlue;
#elif VERSIONWIN
	uBYTE	fBlue;
	uBYTE	fGreen;
	uBYTE	fRed;
	uBYTE	fAlpha;
#endif
} VPixelColor;


#if USE_OBSOLETE_API

typedef enum FontStyle {
	FONT_MODERN	= 0,
	FONT_ROMAN,
	FONT_SWISS,
	FONT_SCRIPT,
	FONT_DECORATIVE
} FontStyle;
 
 
typedef enum IconState {
	IS_NORMAL	= 0,
	IS_GRAY,
	IS_SHADOW,
	IS_WHITE
} IconState;

#pragma pack(push, 2)
// QD struct

typedef struct QDRect
{
	sWORD		top;
	sWORD		left;
	sWORD		bottom;
	sWORD		right;
}QDRect;

typedef struct QDPoint
{
	sWORD		v;
	sWORD		h;
}QDPoint;

typedef struct QDRGBColor 
{
	uWORD      red;
	uWORD      green;
	uWORD      blue;
}QDRGBColor;

#pragma pack(pop)

#endif	// USE_OBSOLETE_API


/** delegate class used to get 4D database resource path */
class XTOOLBOX_API IGraphics_ApplicationIntf
{
public:
	virtual XBOX::VError GetDBResourcePath( XBOX::VFilePath& outPath) = 0;
};

END_TOOLBOX_NAMESPACE

#endif
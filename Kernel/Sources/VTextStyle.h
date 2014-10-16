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
#ifndef __VTextStyle__
#define __VTextStyle__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VString_ExtendedSTL.h"

//forward reference for DB language context reference (defined in 4D)
class VDBLanguageContext;

BEGIN_TOOLBOX_NAMESPACE

class VString;

typedef enum eStyleJust
{
	JST_Notset = -1,
	JST_Default = 0,
	JST_Left,
	JST_Right,
	JST_Center,
	JST_Mixed,
	JST_Justify,
	JST_Baseline,		//used only for embedded image vertical alignment
	JST_Superscript,	//used only for embedded image vertical alignment
	JST_Subscript,		//used only for embedded image vertical alignment
	JST_Top = JST_Left,
	JST_Bottom = JST_Right
} eStyleJust;

/** CSS rect type definition */
typedef struct sDocPropRect
{
	sLONG left;
	sLONG top;
	sLONG right;
	sLONG bottom;

	bool operator ==(const sDocPropRect& inRect) const { return left == inRect.left && top == inRect.top && right == inRect.right && bottom == inRect.bottom; }
} sDocPropRect;

/** layout page mode */
typedef enum eDocPropLayoutPageMode
{
	kDOC_LAYOUT_MODE_NORMAL = 0,
	kDOC_LAYOUT_MODE_PAGE = 1,
	kDOC_LAYOUT_MODE_WEB = 2
} eDocPropLayoutPageMode;

/** text align */
typedef eStyleJust eDocPropTextAlign;

/** text direction (alias for CSSProperty::eDirection) */
typedef enum eDocPropDirection
{
	kDOC_DIRECTION_LTR = 0,
	kDOC_DIRECTION_RTL
} eDocPropDirection;

/** tab stop type (alias for CSSProperty::eTabStopType) */
typedef enum eDocPropTabStopType
{
	kDOC_TAB_STOP_TYPE_LEFT = 0,
	kDOC_TAB_STOP_TYPE_RIGHT,
	kDOC_TAB_STOP_TYPE_CENTER,
	kDOC_TAB_STOP_TYPE_DECIMAL,
	kDOC_TAB_STOP_TYPE_BAR
} eDocPropTabStopType;


/** border style (alias for CSSProperty::eBorderStyle) */
typedef enum eDocPropBorderStyle
{
	kDOC_BORDER_STYLE_NONE = 0,
	kDOC_BORDER_STYLE_HIDDEN,
	kDOC_BORDER_STYLE_DOTTED,
	kDOC_BORDER_STYLE_DASHED,
	kDOC_BORDER_STYLE_SOLID,
	kDOC_BORDER_STYLE_DOUBLE,
	kDOC_BORDER_STYLE_GROOVE,
	kDOC_BORDER_STYLE_RIDGE,
	kDOC_BORDER_STYLE_INSET,
	kDOC_BORDER_STYLE_OUTSET
} eDocPropBorderStyle;

/** background repeat (alias for CSSProperty::eBackgroundRepeat) */
typedef enum eDocPropBackgroundRepeat
{
	kDOC_BACKGROUND_REPEAT = 0,
	kDOC_BACKGROUND_REPEAT_X,
	kDOC_BACKGROUND_REPEAT_Y,
	kDOC_BACKGROUND_NO_REPEAT

} eDocPropBackgroundRepeat;


/** background box (alias for CSSProperty::eBackgroundBox) */
typedef enum eDocPropBackgroundBox
{
	kDOC_BACKGROUND_BORDER_BOX = 0,
	kDOC_BACKGROUND_PADDING_BOX,
	kDOC_BACKGROUND_CONTENT_BOX

} eDocPropBackgroundBox;


/** background size (alias for CSSProperty::eBackgroundSize)
@remark
background size doc property type is VectorOfSLong (size = 2 for width & height) ;
here is the mapping between CSS values (on the left) & doc property sLONG value (on the right):

length 				length in TWIPS (if length > 0 - 0 is assumed to be auto)
percentage			[1%;1000%] is mapped to [-1;-1000] (so 200% is mapped to -200) - overflowed percentage is bound to the [1%;1000%]
auto				kDOC_BACKGROUND_SIZE_AUTO (no height)
cover				kDOC_BACKGROUND_SIZE_COVER (no height)
contain				kDOC_BACKGROUND_SIZE_CONTAIN (no height)
*/
typedef enum eDocPropBackgroundSize
{
	kDOC_BACKGROUND_SIZE_AUTO = 0,
	kDOC_BACKGROUND_SIZE_COVER = -10000,
	kDOC_BACKGROUND_SIZE_CONTAIN = -10001,
	kDOC_BACKGROUND_SIZE_UNDEFINED = -20000	//used to filter undefined width or height

} eDocPropBackgroundSize;


typedef enum eFontStyle
{
	kFONT_STYLE_NORMAL = 0,
	kFONT_STYLE_ITALIC = 1,
	kFONT_STYLE_OBLIQUE = 2		//implemented as italic
} eFontStyle;

typedef enum eTextDecoration
{
	kTEXT_DECORATION_NONE = 0,
	kTEXT_DECORATION_UNDERLINE = 1,
	kTEXT_DECORATION_OVERLINE = 2,	//remark: not supported
	kTEXT_DECORATION_LINETHROUGH = 4,	//alias 'strikeout' style
	kTEXT_DECORATION_BLINK = 8		//remark: not supported
} eTextDecoration;

typedef enum eFontWeight	//note: only normal & bold are actually supported (if value >= kFONT_WEIGHT_BOLD, it is assumed to be kFONT_WEIGHT_BOLD otherwise kFONT_WEIGHT_NORMAL)
{
	kFONT_WEIGHT_100 = 100,
	kFONT_WEIGHT_200 = 200,
	kFONT_WEIGHT_300 = 300,
	kFONT_WEIGHT_400 = 400,
	kFONT_WEIGHT_500 = 500,
	kFONT_WEIGHT_600 = 600,
	kFONT_WEIGHT_700 = 700,
	kFONT_WEIGHT_800 = 800,
	kFONT_WEIGHT_900 = 900,

	kFONT_WEIGHT_NORMAL = 400,
	kFONT_WEIGHT_BOLD = 700,

	kFONT_WEIGHT_LIGHTER = -1,
	kFONT_WEIGHT_BOLDER = -2,

	kFONT_WEIGHT_MIN = 100,
	kFONT_WEIGHT_MAX = 900
} eFontWeight;

/** list style type (alias CSSProperty::eListStyleType) */
typedef enum eDocPropListStyleType
{
	kDOC_LIST_STYLE_TYPE_FIRST = 0,

	kDOC_LIST_STYLE_TYPE_NONE = 0,

	kDOC_LIST_STYLE_TYPE_UL_FIRST,

	kDOC_LIST_STYLE_TYPE_DISC = kDOC_LIST_STYLE_TYPE_UL_FIRST,
	kDOC_LIST_STYLE_TYPE_CIRCLE,
	kDOC_LIST_STYLE_TYPE_SQUARE,

	kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE,
	kDOC_LIST_STYLE_TYPE_DIAMOND,
	kDOC_LIST_STYLE_TYPE_CLUB,
	kDOC_LIST_STYLE_TYPE_CUSTOM,

	kDOC_LIST_STYLE_TYPE_UL_LAST = kDOC_LIST_STYLE_TYPE_CUSTOM,
	kDOC_LIST_STYLE_TYPE_UL_4D_FIRST = kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE,
	kDOC_LIST_STYLE_TYPE_UL_4D_LAST = kDOC_LIST_STYLE_TYPE_CUSTOM,

	kDOC_LIST_STYLE_TYPE_OL_FIRST,

	kDOC_LIST_STYLE_TYPE_DECIMAL = kDOC_LIST_STYLE_TYPE_OL_FIRST,
	kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO,
	kDOC_LIST_STYLE_TYPE_LOWER_LATIN,
	kDOC_LIST_STYLE_TYPE_LOWER_ROMAN,
	kDOC_LIST_STYLE_TYPE_UPPER_LATIN,
	kDOC_LIST_STYLE_TYPE_UPPER_ROMAN,
	kDOC_LIST_STYLE_TYPE_LOWER_GREEK,
	kDOC_LIST_STYLE_TYPE_ARMENIAN,
	kDOC_LIST_STYLE_TYPE_GEORGIAN,
	kDOC_LIST_STYLE_TYPE_HEBREW,
	kDOC_LIST_STYLE_TYPE_HIRAGANA,
	kDOC_LIST_STYLE_TYPE_KATAKANA,
	kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC,

	kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,
	kDOC_LIST_STYLE_TYPE_OL_4D_FIRST = kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,
	kDOC_LIST_STYLE_TYPE_OL_4D_LAST = kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,

	kDOC_LIST_STYLE_TYPE_OL_LAST = kDOC_LIST_STYLE_TYPE_OL_4D_LAST,

	kDOC_LIST_STYLE_TYPE_LAST = kDOC_LIST_STYLE_TYPE_OL_LAST

} eDocPropListStyleType;

#define kDOC_LIST_START_AUTO	-1234567


/** document node type */
typedef enum eDocNodeType
{
	kDOC_NODE_TYPE_DOCUMENT = 0,
	kDOC_NODE_TYPE_DIV, //TODO: not implemented yet
	kDOC_NODE_TYPE_PARAGRAPH,
	kDOC_NODE_TYPE_IMAGE,
	kDOC_NODE_TYPE_TABLE,
	kDOC_NODE_TYPE_TABLEROW,
	kDOC_NODE_TYPE_TABLECELL,

	kDOC_NODE_TYPE_CLASS_STYLE,	//it is used to store class style properties only (storage class for parsed CSS declarations with selector 'elemname.classname')
	kDOC_NODE_TYPE_UNKNOWN,

	kDOC_NODE_TYPE_COUNT

} eDocNodeType;

/** span text reference type */
typedef enum eDocSpanTextRef
{
	kSpanTextRef_4DExp,		//4D expression
	kSpanTextRef_URL,		//URL link
	kSpanTextRef_User,		//user property 
	kSpanTextRef_Image,		//image
	kSpanTextRef_UnknownTag	//unknown or user tag
} eDocSpanTextRef;

/** span reference display text type */
typedef enum eSpanRefTextType
{
	kSRTT_Uniform = 0,		//space as text
	kSRTT_Value = 1,		//value as text
	kSRTT_Src = 2,		//source as text
	kSRTT_4DExp_Normalized = 4			//tokenized expression is normalized - converted for current language & version without special tokens (valid only for 4D exp and if combined with kSRTT_Src)

} eSpanRefTextType;

/** span reference display text type mask */
typedef uLONG	SpanRefTextTypeMask;

#define UNDEFINED_STYLE -1
typedef uLONG	RGBAColor;

#define RGBACOLOR_TRANSPARENT 0

/** span reference display text type
important: do not modify constants from 1 to 512 otherwise it would screw up 4D commands
*/
typedef enum eSpanRefCombinedTextType
{
	kSRCTT_4DExp_Value = 1,		//computed expression
	kSRCTT_4DExp_Src = 2,		//tokenized expression

	kSRCTT_URL_Value = 4,		//url label
	kSRCTT_URL_Src = 8,		//url link

	kSRCTT_User_Value = 16,		//user label
	kSRCTT_User_Src = 32,		//user source

	kSRCTT_UnknownTag_Value = 64,		//tag plain text
	kSRCTT_UnknownTag_Src = 128,		//xml tag as plain text

	kSRCTT_Image_Value = 256,		//a breakable character (used only for line-breaking - image is displayed, not the character)
	kSRCTT_Image_Src = 512,		//image url

	kSRCTT_4DExp_Normalized = 1024,		//tokenized expression is normalized - converted for current language & version without special tokens (valid only if combined with kSRCTT_4DExp_Src)

	kSRCTT_Value = kSRCTT_4DExp_Value | kSRCTT_URL_Value | kSRCTT_User_Value | kSRCTT_UnknownTag_Value | kSRCTT_Image_Value,
	kSRCTT_Src = kSRCTT_4DExp_Src | kSRCTT_URL_Src | kSRCTT_User_Src | kSRCTT_UnknownTag_Src | kSRCTT_Image_Src

} eSpanRefCombinedTextType;

/** span reference display text type mask
@remarks
if mask value is 0 for a span reference kind, text is equal to a single no-breaking space (uniform text)
*/
typedef uLONG	SpanRefCombinedTextTypeMask;


//CSS unit types (alias of XBOX::CSSProperty::eUnit - make sure it is consistent with CSSProperty::eUnit enum)
typedef enum eCSSUnit
{
	kCSSUNIT_PERCENT = 0,			/* percentage (number range is [0.0, 1.0]) */
	kCSSUNIT_PX,					/* pixels */
	kCSSUNIT_PC,					/* picas */
	kCSSUNIT_PT,					/* points */
	kCSSUNIT_MM,					/* millimeters */
	kCSSUNIT_CM,					/* centimeters */
	kCSSUNIT_IN,					/* inchs */
	kCSSUNIT_EM,					/* size of the current font */
	kCSSUNIT_EX,					/* x-height of the current font */
	kCSSUNIT_FONTSIZE_LARGER,		/* larger than the parent node font size (used by FONTSIZE only) */
	kCSSUNIT_FONTSIZE_SMALLER		/* smaller than the parent node font size (used by FONTSIZE only) */
} eCSSUnit;

class XTOOLBOX_API VDocCache4DExp;
class XTOOLBOX_API VTextStyle;

class XTOOLBOX_API IDocSpanTextRef
{
public:
	virtual							~IDocSpanTextRef() {}

	virtual	IDocSpanTextRef*		Clone() = 0;
	
	virtual bool					EqualTo(const IDocSpanTextRef *inSpanRef) const = 0;
	
	virtual	SpanRefTextTypeMask		CombinedTextTypeMaskToTextTypeMask(SpanRefCombinedTextTypeMask inCTT) = 0;

	virtual	eDocSpanTextRef			GetType() const = 0;

	virtual	void					SetRef(const VString& inValue) = 0;

	virtual	const VString&			GetRef() const = 0;

	virtual	const VString&			GetRefNormalized() const = 0;

	/** for 4D expression, compute and store expression for current language and version without special tokens;
	for other reference, do nothing */
	virtual	void					UpdateRefNormalized(VDBLanguageContext *inLC) = 0;

	virtual	void					SetDefaultValue(const VString& inValue) = 0;

	virtual	const VString&			GetDefaultValue() const = 0;

	virtual	const VString&			GetComputedValue() = 0;


	/** evaluate now 4D expression */
	virtual	void					EvalExpression(VDBLanguageContext *inLC, VDocCache4DExp *inCache4DExp = NULL) = 0;

	/** return span reference style override
	@remarks
	this style is always applied after VTreeTextStyle styles
	*/
	virtual	const VTextStyle*		GetStyle() const = 0;

	virtual	SpanRefTextTypeMask		ShowRef() const = 0;
	virtual	void					ShowRef(SpanRefTextTypeMask inShowRef) = 0;

	/** true: display normalized reference if ShowRef() == kSRTT_Src (for 4D exp, it is expression for current language and version without special tokens)
	false: display reference as set initially
	@remarks
	you should call at least once UpdateRefNormalized before calling then GetDisplayValue
	*/
	virtual	bool					ShowRefNormalized() const = 0;
	virtual	void					ShowRefNormalized(bool inShowRefNormalized) = 0;

	virtual	bool					ShowHighlight() const = 0;
	virtual	void					ShowHighlight(bool inShowHighlight) = 0;

	virtual	bool					IsMouseOver() const = 0;

	/** return true if mouse over status has changed, false otherwise */
	virtual	bool					OnMouseEnter() = 0;

	/** return true if mouse over status has changed, false otherwise */
	virtual	bool					OnMouseLeave() = 0;

	virtual	const VString&			GetDisplayValue(bool inResetDisplayValueDirty = true) = 0;

	virtual bool					IsDisplayValueDirty() const = 0;

	virtual	bool					IsClickable() const = 0;

	virtual	bool					OnClick() const = 0;
};

/** span text reference custom layout delegate
@remarks
it might be used for instance to customize layout & painting of embedded objects like images (see IDocBaseLayout in TextCore - derived from this class)
*/
class XTOOLBOX_API IDocSpanTextRefLayout
{
public:
	virtual	void					RetainRef() = 0;
	virtual	void					ReleaseRef() = 0;
protected:
	virtual							~IDocSpanTextRefLayout() {}
};

class XTOOLBOX_API IDocSpanTextRefFactory
{
public:
	virtual ~IDocSpanTextRefFactory() {}
	static	IDocSpanTextRefFactory* Get() { return fFactory; }
	static  void					Init(IDocSpanTextRefFactory* inFactory) { fFactory = inFactory; }
	static	void					DeInit() { if (fFactory) delete fFactory; fFactory = NULL; }

	virtual	IDocSpanTextRef*		Create(const eDocSpanTextRef inType, const VString& inRef, const VString& inDefaultValue = CVSTR(" ")) = 0;

private:
	static	IDocSpanTextRefFactory*	fFactory;
};


//CSS helper methods
class XTOOLBOX_API ICSSUtil
{
public:
	/** convert point value to TWIPS 
	@remarks
		round to nearest TWIPS integral value
	*/
	static sLONG PointToTWIPS(Real inValue) { return inValue >= 0 ? floor(inValue*20+0.5f) : -floor((-inValue)*20+0.5f); }
	static Real TWIPSToPoint( sLONG inValue) { return (((Real)inValue)/20.0); }

	/** convert CSS dimension to TWIPS */
	static sLONG CSSDimensionToTWIPS(const Real inDimNumber, const eCSSUnit inDimUnit, const uLONG inDPI = 72, const sLONG inParentValueTWIPS = 0);

	/** convert CSS dimension to point */
	static Real CSSDimensionToPoint(const Real inDimNumber, const eCSSUnit inDimUnit, const uLONG inDPI = 72, const sLONG inParentValueTWIPS = 0)
	{
		//convert first to TWIPS to ensure point will have TWIPS precision
		sLONG twips = CSSDimensionToTWIPS( inDimNumber, inDimUnit, inDPI, inParentValueTWIPS);
		return TWIPSToPoint(twips);
	}

	/** round a number at TWIPS precision using nearest TWIPS integral value (so result is always multiple of 1/20 = 0.05) 
	@remarks 
		it is used to round values in point to TWIPS precision - so that RoundTWIPS(value in point)*20 is always a integer 
	*/
	static Real	RoundTWIPS( const Real inValue) { return TWIPSToPoint(PointToTWIPS( inValue)); } 

	/** round a number at TWIPS precision using ceil of TWIPS value */
	static Real	RoundTWIPSCeil( const Real inValue) { return TWIPSToPoint(ceil(inValue*20)); } 
	/** round a number at TWIPS precision using floor of TWIPS value */
	static Real	RoundTWIPSFloor( const Real inValue) { return TWIPSToPoint(floor(inValue*20)); } 

};



class XTOOLBOX_API VTextStyle : public VObject
{
typedef VObject inherited;
public:
	/** style mask enum type definition (alias VFrameTextEdit::ENTE_StyleAttributs) */
	typedef enum eStyle
	{
		eStyle_Font			= 1,
		eStyle_FontSize		= 2, 
		eStyle_Bold			= 4,
		eStyle_Italic		= 8,
		eStyle_Underline	= 16,
		eStyle_Strikeout	= 32,
		eStyle_Color		= 64,
		eStyle_Just			= 128,
		eStyle_BackColor	= 256,
		eStyle_Ref			= 512,

		eStyle_MaskFont		= eStyle_Font|
							  eStyle_FontSize,

		eStyle_MaskFace		= eStyle_Bold|
							  eStyle_Italic|
							  eStyle_Underline|
							  eStyle_Strikeout, 

		eStyle_MaskColors	= eStyle_Color| 
							  eStyle_BackColor,

		eStyle_Mask			= eStyle_Font|
							  eStyle_FontSize|
							  eStyle_Bold|
							  eStyle_Italic|
							  eStyle_Underline|
							  eStyle_Strikeout| 
							  eStyle_Color| 
							  eStyle_Just|
							  eStyle_BackColor|
							  eStyle_Ref
	} eStyle;

	VTextStyle(const VTextStyle* inTextStyle = NULL);
	virtual		~VTextStyle();
	VTextStyle& operator = ( const VTextStyle& inTextStyle);
	bool operator == (const VTextStyle& inTextStyle) const { return IsEqualTo( inTextStyle); }
	bool operator != (const VTextStyle& inTextStyle) const { return !IsEqualTo( inTextStyle); }

	void		SetFontStyleToNormal()
	{
				SetBold(0);
				SetItalic(0);
				SetUnderline(0);
				SetStrikeout(0);
	}

	void		SetRange(sLONG inBegin, sLONG inEnd);
	void		GetRange(sLONG& outBegin, sLONG& outEnd) const;
	
	//Bold , italic et underline et strikeout peuvent avoir les valeur TRUE(1), FALSE(0) ou UNDEFINED_STYLE(-1)
	void		SetBold(sLONG inValue);
	sLONG		GetBold() const;
	void		SetItalic(sLONG inValue);
	sLONG		GetItalic() const;
	void		SetUnderline(sLONG inValue);
	sLONG		GetUnderline() const;
	void		SetStrikeout(sLONG inValue);
	sLONG		GetStrikeout() const;
	void		SetColor(const RGBAColor& inValue);
	RGBAColor	GetColor() const;
	void		SetBackGroundColor(const RGBAColor& inValue);
	RGBAColor	GetBackGroundColor() const;
	void		SetHasBackColor(bool inValue);
	bool		GetHasBackColor() const;
	void		SetHasForeColor(bool inValue);
	bool		GetHasForeColor() const;
	/** set font size 
	@remarks
		it is 4D form compliant font size so on Windows it is actually pixel font size (equal to point size in 4D form where dpi=72)
		
		this font size is equal to font size in 4D form
	*/
	void		SetFontSize(const Real& inValue);
	/** get font size 
	@remarks
		it is 4D form compliant font size so on Windows it is actually pixel font size (equal to point size in 4D form where dpi=72)
		
		this font size is equal to font size in 4D form
	*/
	Real		GetFontSize() const;
	void		SetFontName(const VString& inValue);
	const VString&	GetFontName() const;
	void		SetJustification(eStyleJust inValue);
	eStyleJust	GetJustification() const;

	void		Translate(sLONG inValue);

	/** merge current style with the specified style 
	@remarks
		input style properties override current style properties without taking account input style range
		(current style range is preserved)
	*/
	void		MergeWithStyle( const VTextStyle *inStyle, const VTextStyle *inStyleParent = NULL, bool inForMetricsOnly = false, bool inAllowMergeSpanRefs = false);

	/** append styles from the passed style 
	@remarks
		input style properties override current style properties without taking account input style range
		(current style range is preserved)

		if (inOnlyIfNotSet == true), overwrite only styles which are not defined locally
	*/
	void		AppendStyle( const VTextStyle *inStyle, bool inOnlyIfNotSet = false, bool inAllowMergeSpanRefs = false);

	/** return true if none property is defined (such style is useless & should be discarded) */
	bool		IsUndefined(bool inIgnoreSpanRef = false) const;

	/** return true if this style & the input style are equal */
	bool		IsEqualTo( const VTextStyle &inStyle, bool inIncludeRange = true) const;

	void		Reset();

	/** reset styles if equal to inStyle styles */
	void		ResetCommonStyles( const VTextStyle *inStyle, bool inIgnoreSpanRef = true);

	/** return style which have properties common to this style & the input style 
		and reset properties which are common in the two styles
		(NULL if none property is commun)
	*/
	VTextStyle *ExtractCommonStyle(VTextStyle *ioStyle2);

	bool		IsSpanRef() const { return fSpanTextRef != NULL; }

	/** set and own span text reference */
	void		SetSpanRef( IDocSpanTextRef *inRef); 

	/** get span text reference  */
	IDocSpanTextRef *GetSpanRef() const { return fSpanTextRef; }

protected:
	sLONG	fBegin;
	sLONG	fEnd;
	sLONG	fIsBold;
	sLONG	fIsUnderline;
	sLONG	fIsStrikeout;
	sLONG	fIsItalic;
	RGBAColor	fColor;
	RGBAColor	fBkColor;
	bool	fHasBackColor:1;
	bool	fHasForeColor:1;
	Real	fFontSize;
    VString*	fFontName;
	eStyleJust fJust;

	/** span text reference (like 4D expression, url link, user prop, etc...) */
	IDocSpanTextRef *fSpanTextRef;
};


class XTOOLBOX_API VTreeTextStyle : public VObject, public IRefCountable
{
typedef VObject inherited;
public:
typedef void (*fnBeforeReplace)(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText);
typedef void (*fnAfterReplace)(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced);


	VTreeTextStyle(VTextStyle* inData = NULL);
	VTreeTextStyle(const VTreeTextStyle& inStyle);
	VTreeTextStyle(const VTreeTextStyle* inStyle, bool inDeepCopy = true);
	virtual		~VTreeTextStyle();

	bool operator == (const VTreeTextStyle& inStyles) const { return IsEqualTo( inStyles); }
	bool operator != (const VTreeTextStyle& inStyles) const { return !IsEqualTo( inStyles); }

	VTreeTextStyle* GetParent()const;
	void AddChild(VTreeTextStyle* inValue, bool inAutoGrowRange = false);
	void InsertChild(VTreeTextStyle* inValue, sLONG pos, bool inAutoGrowRange = false);
	void RemoveChildAt(sLONG inNth);

	void RemoveChildren();

	void SetData(VTextStyle* inValue);
	VTextStyle* GetData () const;

	sLONG GetChildCount() const	;
	VTreeTextStyle* GetNthChild(sLONG inNth) const;

	bool HasSpanRefs() const 
	{ 
		xbox_assert(fNumChildSpanRef >= 0); 
		return fNumChildSpanRef > 0; 
	}

	void Translate(sLONG inValue);

	/** return true if theses styles are equal to the passed styles 
	@remarks
		comparison ignores parent
	*/
	bool IsEqualTo( const VTreeTextStyle& inStyles) const;

	/** replace styled text at the specified range with text 
	@remarks
		replaced text will inherit only uniform style on replaced range

		return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
	*/
	static VIndex ReplaceStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, const IDocSpanTextRef * inPreserveSpanTextRef = NULL, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	static VIndex InsertStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inPos = 0, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false)
	{
		return ReplaceStyledText( ioText, ioStyles, inText, inStyles, inPos, inPos, inAutoAdjustRangeWithSpanRef, inSkipCheckRange);
	}

	static VIndex RemoveStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false)
	{
		return ReplaceStyledText( ioText, ioStyles, CVSTR(""), NULL, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange);
	}

	/** return true if position is over a span reference */
	bool IsOverSpanRef(const sLONG inPos) { return GetStyleRefAtPos( inPos) != NULL; }

	/** return true if range intersects a span reference */
	bool IsOverSpanRef(const sLONG inStart, const sLONG inEnd) { return GetStyleRefAtRange( inStart, inEnd) != NULL; }

	/** replace text with span text reference on the specified range
	@remarks
		it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on span ref display type and on inNoUpdateRef (only one space plain text if inNoUpdateRef == true - if true, inNoUpdateRef changes span ref display type to uniform)

		you should no more use or destroy passed inSpanRef even if method returns false

		IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
				   so you should call before inSpanRef->EvalExpression to eval 4D expression - if you want to show 4D exp value of course;
				   note that you might call EvalExpression after ReplaceAndOwnSpanRef (using GetStyleRefAtRange(inStart, inStart+1)->GetSpanRef()->EvalExpression()) 
				   but in that case you should call also UpdateSpanRefs to update 4D exp plain text because EvalExpression does not update plain text
				   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
				    - if display type is set to normalized source)
	*/
	static bool ReplaceAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, IDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = true, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);


	/** insert span text reference at the specified text position 
	@remarks
		see ReplaceAndOwnSpanRef	
	*/
	static bool InsertAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, IDocSpanTextRef* inSpanRef, sLONG inPos = 0, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = true, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

	/** convert uniform character index (the index used by 4D commands) to actual plain text position 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void CharPosFromUniform( sLONG& ioPos) const;

	/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void CharPosToUniform( sLONG& ioPos) const;

	/** convert uniform character range (as used by 4D commands) to actual plain text position 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void RangeFromUniform( sLONG& inStart, sLONG& inEnd) const;

	/** convert actual plain text range to uniform range (as used by 4D commands) 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void RangeToUniform( sLONG& inStart, sLONG& inEnd) const;

	/** adjust selection 
	@remarks
		you should call this method to adjust selection if styles contain span references 
	*/
	void AdjustRange(sLONG& ioStart, sLONG& ioEnd) const;

	/** adjust char pos
	@remarks
		you should call this method to adjust char pos if styles contain span references 
	*/
	void AdjustPos(sLONG& ioPos, bool inToEnd = true) const;

	/** return span reference at the specified text position if any */
	const VTextStyle *GetStyleRefAtPos(const sLONG inPos) const;

	/** return the first span reference which intersects the passed range */
	const VTextStyle *GetStyleRefAtRange(const sLONG inStart = 0, const sLONG inEnd = -1) const;

	/** show or hide span ref value or source
	@remarks
		propagate flag to all VDocSpanTextRef & mark all as dirty
	*/ 
	void ShowSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs = kSRCTT_Value);

	/** highlight or not span references 
	@remarks
		propagate flag to all VDocSpanTextRef 

		on default (if VDocSpanTextRef delegate is not overriden) it changes only highlight status for value
		because if reference is displayed, it is always highlighted

		caller should redraw text after calling this method to ensure new style is propertly applied
	*/ 
	void ShowHighlight( bool inShowHighlight = true);

	/** call this method to update span reference style while mouse enters span ref 
	@remarks
		return true if style has changed
	*/
	bool NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef);

	/** call this method to update span reference style while mouse leaves span ref 
	@remarks
		return true if style has changed
	*/
	bool NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef);

	/** update span references plain text  
	@remarks
		parse & update span references plain text  

		return true if plain text or styles have been updated

		this method should be called on control event DoIdle in order to update dirty span text references plain text
		(range might be constrained to text visible range only)
	*/
	static bool UpdateSpanRefs( VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, 
								void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

	/** freeze span references  
	@remarks
		reset styles span references

		this method preserves VTextStyle span reference container: it only resets VTextStyle span reference, transforming it to a normal VTextStyle
	*/
	void FreezeSpanRefs( sLONG inStart = 0, sLONG inEnd = -1);

	/** freeze expressions  
	@remarks
		parse & replace any 4D expression reference with evaluated expression plain text & discard 4D expression reference on the passed range 

		return true if any 4D expression has been replaced with plain text
	*/
	static bool FreezeExpressions( VDBLanguageContext *inLC, VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	/** reset cached 4D expressions on the specified range */
	void ResetCacheExpressions( VDocCache4DExp *inCache4DExp, sLONG inStart = 0, sLONG inEnd = -1);

	/** eval 4D expressions  
	@remarks
		return true if any 4D expression has been evaluated
	*/
	bool EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL);

	bool HasExpressions() const;

	/** expand styles ranges from the specified range position by the specified offset 
	@remarks
		should be called just after text has been inserted in a multistyled text
		(because inserted text should inherit style at current inserted text position)
		for instance:
			text.Insert(textToInsert, 10); //caution: here 1-based index
			styles->ExpandRangesAtPosBy( 9, textToInsert.GetLength()); //caution: here 0-based index so 10-1 and not 10
	*/
	void ExpandAtPosBy(	sLONG inStart, sLONG inInflate = 1, const IDocSpanTextRef *inInflateSpanRef = NULL, VTextStyle **outStyleToPostApply = NULL, 
						bool inOnlyTranslate = false,
						bool inIsStartOfParagraph = false);

	/** truncate the specified style range
	@param inStart
		start of truncated range
	@param inLength
		length of truncated range (-1 to truncate from inStart to the end of this VTreeTextStyle range)
	@remarks
		should be called just after text has been deleted in a multistyled text
		for instance:
			text.Remove(10, 5); //caution: here 1-based index
			styles->Truncate(9, 5); //caution: here 0-based index so 10-1 and not 10
	*/
	void Truncate(sLONG inStart, sLONG inLength = -1, bool inRemoveStyleIfEmpty = true, bool inPreserveSpanRefIfEmpty = false);

	/** return true if plain text range can be deleted or replaced 
	@remarks
		return false if range overlaps span text references plain text & if range does not fully includes span text reference(s) plain text
		(we cannot delete or replace some sub-part of span text reference plain text but only full span text reference plain text at once)
	*/
	bool CanDeleteOrReplace( sLONG inStart, sLONG inEnd);

	/** return true if style can be applied 
	@remarks
		return false if it is conflicting with span references (span references cannot override)	
	*/
	bool CanApplyStyle( const VTextStyle *inStyle, sLONG inStartBase = 0) const;

	/** build a vector of sequential styles from the current cascading styles 
	@remarks
		output styles ranges are disjoint & n+1 style range follows immediately n style range

		this method does not modify current VTreeTextStyle
	*/
	void BuildSequentialStylesFromCascadingStyles( std::vector<VTextStyle *>& outStyles, VTextStyle *inStyleParent = NULL, bool inForMetrics = false, bool inFirstUniformStyleOnly = false) const;

	/** convert cascading styles to sequential styles 
	@remarks
		cascading styles are merged in order to build a tree which has only one level deeper;
		on output, child VTreeTextStyle ranges will be disjoint & n+1 style range follows immediately n style range
	*/
	void BuildSequentialStylesFromCascadingStyles( VTextStyle *inStyleParent = NULL, bool inFirstUniformStyleOnly = false);

	/** build cascading styles from the input sequential styles 
	@remarks
		input styles ranges should not overlap  

		this methods overrides current styles with the input styles
		this class owns input VTextStyle instances so caller should not delete it
	*/
	void BuildCascadingStylesFromSequentialStyles( std::vector<VTextStyle *>& inStyles);

	/** create new tree text style from the current but constrained to the specified range & eventually relative to the start of the specified range 
	@param inStart
		start of new range 
	@param inEnd
		end of new range 
	@param inRelativeToStart
		if true, new range is translated by -inStart
	@param inNULLIfRangeEmpty
		if true, return NULL if new range is empty 
	*/ 
	VTreeTextStyle *CreateTreeTextStyleOnRange( const sLONG inStart, const sLONG inEnd, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true, bool inNoMerge = false) const;

	/** apply style 
	@param inStyle
		style to apply
	@remarks
		caller is owner of inStyle
	*/
	void ApplyStyle( const VTextStyle *inStyle, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	/** apply styles 
	@param inStyle
		styles to apply
	@remarks
		caller is owner of inStyle
	*/
	void ApplyStyles( const VTreeTextStyle *inStyles, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	/** create & return uniform style on the specified range 
	@remarks
		undefined value is set for either mixed or undefined style 

		mixed status is returned optionally in outMaskMixed using VTextStyle::Style mask values
		
		return value is always not NULL
	*/
	VTextStyle *CreateUniformStyleOnRange( sLONG inStart, sLONG inEnd, bool inRelativeToStart = true, uLONG *outMaskMixed = NULL) const;

	/** get font name on the specified range 
	@return
		true: font name is used on all range
		false: font name is mixed: return font name of the font used by range start character
	*/
	bool		GetFontName(sLONG inStart, sLONG inEnd, VString& ioFontName) const;

	/** get font size on the specified range 
	@return
		true: font size is used on all range
		false: font size is mixed: return font size of the font used by range start character

		caution: for compatibility, returned font size is in 4D form compliant font size (72dpi)
				 so it is actually font size in pixel on Windows: see VTextStyle
	*/
	bool		GetFontSize(sLONG inStart, sLONG inEnd, Real& ioFontSize) const;

	/** get text color on the specified range 
	@return
		true: text color is used on all range
		false: text color is mixed: return text color of the font used by range start character
	*/
	bool		GetTextForeColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const;

	/** get back text color on the specified range 
	@return
		true: back text color is used on all range
		false: back text color is mixed: return back text color of the font used by range start character
	*/
	bool		GetTextBackColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const;

	/** get font weight on the specified range 
	@return
		true: font weight is used on all range
		false: font weight is mixed: return font weight of the font used by range start character
	*/
	bool		GetBold( sLONG inStart, sLONG inEnd, sLONG& ioBold) const;

	/** get font italic on the specified range 
	@return
		true: font italic is used on all range
		false: font italic is mixed: return font italic of the font used by range start character
	*/
	bool		GetItalic(sLONG inStart, sLONG inEnd, sLONG& ioItalic) const;

	/** get font underline on the specified range 
	@return
		true: font underline is used on all range
		false: font underline is mixed: return font underline of the font used by range start character
	*/
	bool		GetUnderline(sLONG inStart, sLONG inEnd, sLONG& ioUnderline) const;

	/** get font strikeout on the specified range 
	@return
		true: font strikeout is used on all range
		false: font strikeout is mixed: return font strikeout of the font used by range start character
	*/
	bool		GetStrikeout(sLONG inStart, sLONG inEnd, sLONG& ioStrikeOut) const;

	/** get justification on the specified range 
	@return
		true: justification is used on all range
		false: justification is mixed: return justification used by range start character
	*/
	bool		GetJustification(sLONG inStart, sLONG inEnd, eStyleJust& ioJust) const;

	/** return true if it is undefined */
	bool		IsUndefined(bool inTrueIfRangeEmpty = false) const;

	/** return true if font family is defined */
	bool		HasFontFamilyOverride() const;

	void		NotifySetSpanRef() { if (fParent) fParent->fNumChildSpanRef++; }
	void		NotifyClearSpanRef() { if (fParent) fParent->fNumChildSpanRef--; }
protected:
	/** merge current style recursively with the specified style 
	@remarks
		input style properties which are defined override current style properties without taking account input style range
		(current style range is preserved)
	*/
	void _MergeWithStyle( const VTextStyle *inStyle, VTextStyle *inStyleInherit = NULL);

	/** apply style 
	@param inStyle
		style to apply
	@remarks
		caller is owner of inStyle
	*/
	void _ApplyStyleRec( const VTextStyle *inStyle, sLONG inRecLevel = 0, VTextStyle *inStyleInherit = NULL, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	void SetParent(VTreeTextStyle* inValue);

	typedef std::vector<VTreeTextStyle*>	VectorOfStyle;
	VectorOfStyle		fChildren;
	VTreeTextStyle*		fParent;
	VTextStyle*			fdata;

	VIndex				fNumChildSpanRef;
};

#define kDOC_CACHE_4DEXP_SIZE 1000

class XTOOLBOX_API VDocCache4DExp : public VObject, public IRefCountable
{
public:
	VDocCache4DExp(VDBLanguageContext *inLC, const sLONG inMaxSize = kDOC_CACHE_4DEXP_SIZE) :VObject(), IRefCountable()
	{
		xbox_assert(inLC);
		fDBLanguageContext = inLC;
		fMaxSize = inMaxSize;
		fSpanRef = IDocSpanTextRefFactory::Get() ? IDocSpanTextRefFactory::Get()->Create(kSpanTextRef_4DExp, CVSTR("")) : NULL;
		fEvalAlways = fEvalDeferred = false;
	}
	virtual							~VDocCache4DExp() { if (fSpanRef) delete fSpanRef; }

	/** return 4D expression value, as evaluated from the passed 4D tokenized expression
	@remarks
	returns cached value if 4D exp has been evaluated yet, otherwise eval 4D exp & append pair<exp,value> to the cache

	if fEvalAways == true, it always eval 4D exp & update cache
	if fEvalDeferred == true, if 4D exp is not in cache yet, it adds exp to the internal deferred list but does not eval now 4D exp (returns empty string as value):
	caller might then call EvalDeferred() to eval deferred expressions and clear deferred list
	*/
	void					Get(const VString& inExp, VString& outValue, VDBLanguageContext *inLC = NULL);

	/** manually add a pair <exp,value> to the cache
	@remarks
	add or replace the existing value

	caution: it does not check if value if valid according to the expression so it might be anything
	*/
	void					Set(const VString& inExp, const VString& inValue);

	/** return cache actual size */
	sLONG					GetCacheCount() const { return fMapOfExp.size(); }

	/** return the cache pair <exp,value> at the specified cache index (1-based) */
	bool					GetCacheExpAndValueAt(VIndex inIndex, VString& outExp, VString& outValue);

	/** append cached expressions from the passed cache */
	void					AppendFromCache(const VDocCache4DExp *inCache);

	/** remove the passed 4D tokenized exp from cache */
	void					Remove(const VString& inExp);

	/** clear cache */
	void					Clear() { fMapOfExp.clear(); }

	void					SetDBLanguageContext(VDBLanguageContext *inLC)
	{
		if (fDBLanguageContext == inLC)
			return;
		fDBLanguageContext = inLC;
		fMapOfExp.clear();
	}
	VDBLanguageContext*		GetDBLanguageContext() const { return fDBLanguageContext; }

	/** always eval 4D expression
	@remarks
	if true, Get() will always eval & update cached 4D expression, even if it is in the cache yet
	if false, Get() will only eval & cache 4D expression if it is not in the cache yet, otherwise it returns value from cache
	*/
	void					SetEvalAlways(bool inEvalAlways = true) { fEvalAlways = inEvalAlways; }
	bool					GetEvalAlways() const { return fEvalAlways; }

	/** defer 4D expression eval
	@remarks
	see fEvalDeferred
	*/
	void					SetEvalDeferred(bool inEvalDeferred = true) { fEvalDeferred = inEvalDeferred; if (!fEvalDeferred) fMapOfDeferredExp.clear(); }
	bool					GetEvalDeferred() const { return fEvalDeferred; }

	/** clear deferred expression */
	void					ClearDeferred() { fMapOfDeferredExp.clear(); }

	/** eval deferred expressions & add to cache
	@remarks
	return true if any expression has been evaluated

	on exit, clear also map of deferred expressions
	*/
	bool					EvalDeferred();

private:
	/** time stamp + exp value */
	typedef std::pair<uLONG, VString> ExpValue;

	/** map of exp value per exp */
	typedef unordered_map_VString< ExpValue > MapOfExp;

	/** map of deferred expressions (here boolean value is dummy & unused) */
	typedef unordered_map_VString< bool > MapOfDeferredExp;


	/** DB language context */
	VDBLanguageContext*		fDBLanguageContext;

	/** map of 4D expressions per 4D tokenized expression  */
	MapOfExp				fMapOfExp;

	/** map of deferred 4D expressions
	@see
	fEvalDeferred
	*/
	MapOfDeferredExp		fMapOfDeferredExp;

	/** span ref (used locally to eval any 4D exp) */
	IDocSpanTextRef*		fSpanRef;

	/** if true, Get() will always eval 4D exp & update cached value
	otherwise it returns cached value if any
	*/
	bool					fEvalAlways;

	/** if true, Get() will return exp value from cache if exp has been evaluated yet
	otherwise it adds expression to evaluate to a internal deferred list:
	expressions in deferred list can later be evaluated by calling EvalDeferred	()
	@remarks
	if true, fEvalAlways is ignored
	*/
	bool					fEvalDeferred;

	sLONG					fMaxSize;
};

class VDocNode;
class VDocText;
class VDocClassStyleManager;
class VDocParagraph;
class VDocClassStyle;


/* document interface
@remarks
	interface for document-only methods

	VDocText, VDocTextLayout & VWriteView should share this interface in order to unify document editing for both document, document layout or document frame 
	(VDocTextLayout & VWriteView encapsulate layout & frame-specific management through this common interface)
*/
class XTOOLBOX_API IDocText
{
public:

	//
	// document load/save/replace
	//

	/** create and retain document from the current document
	@param inStart
		range start
	@param inEnd
		range end
	*/
	virtual	VDocText*			IDOC_CreateDocument(sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** create and retain paragraph from the current document plain text, styles & paragraph properties (only first paragraph properties are copied)
	@param inStart
		range start
	@param inEnd
		range end
	*/
	virtual	VDocParagraph*		IDOC_CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** init document from the passed document
	@param inDocument
		document source
	@remarks
		the passed document is used only to initialize document: document is not retained
	*/
	virtual	void				IDOC_InitFromDocument(const VDocText *inDocument) = 0;

	/** init document from the passed paragraph
	@param inPara
		paragraph source
	@remarks
		the passed paragraph is used only to initialize document: paragraph is not retained
	*/
	virtual	void				IDOC_InitFromParagraph(const VDocParagraph *inPara) = 0;

	/** replace document range with the specified document fragment */
	virtual	void				IDOC_ReplaceDocument(const VDocText *inDoc, sLONG inStart = 0, sLONG inEnd = -1) = 0;


	//
	// paragraph reset styles/merge/unmerge
	//



	/** reset all text properties & character styles to default values */
	virtual	void				IDOC_ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;


	/** merge paragraph(s) on the specified range to a single paragraph
	@remarks
		if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
		resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR
	*/
	virtual	bool				IDOC_MergeParagraphs(sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** unmerge paragraph(s) on the specified range
	@remarks
		any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR,
		resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)
	*/
	virtual	bool				IDOC_UnMergeParagraphs(sLONG inStart = 0, sLONG inEnd = -1) = 0;



	//
	// plain text & character styles read/write
	//



	/** return text length */
	virtual	uLONG				IDOC_GetTextLength() const = 0;

	/** replace current text & styles
	@remarks
		here passed styles are always copied

		this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
	*/
	virtual	void				IDOC_SetText(const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	virtual	void				IDOC_GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** return unicode character at the specified position (0-based) */
	virtual	UniChar				IDOC_GetUniChar(sLONG inIndex) const = 0;

	/** apply character style (use passed style range) */
	virtual	bool				IDOC_ApplyStyle(const VTextStyle* inStyle, bool inIgnoreIfEmpty = true) = 0;

	/** return text character styles on the passed range */
	virtual	VTreeTextStyle*		IDOC_RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true) const = 0;



	//
	// text references (url, 4D expression, inline image, etc...)
	//



	virtual	bool				IDOC_HasSpanRefs() const = 0;

	/** replace text with span text reference on the specified range
	@remarks
		it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

		you should no more use or destroy passed inSpanRef even if method returns false

		IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
		so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
		(passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens
		- if display type is set to normalized source)
	*/
	virtual	bool				IDOC_ReplaceAndOwnSpanRef(IDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inNoUpdateRef = false, VDBLanguageContext *inLC = NULL) = 0;

	/** return span reference at the specified text position if any */
	virtual	const VTextStyle*	IDOC_GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const = 0;

	/** return the first span reference which intersects the passed range */
	virtual	const VTextStyle*	IDOC_GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const = 0;

	/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */
	virtual	bool				IDOC_UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, VDBLanguageContext *inLC = NULL) = 0;

	/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
	virtual	bool				IDOC_FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** show span references source or value */
	virtual	void				IDOC_ShowRefs(SpanRefCombinedTextTypeMask inShowRefs, VDBLanguageContext *inLC = NULL) = 0;
	virtual	SpanRefCombinedTextTypeMask IDOC_ShowRefs() const = 0;

	/** eval 4D expressions
	@remarks
		note that it only updates span 4D expression references but does not update plain text:
		call UpdateSpanRefs after to update layout plain text

		return true if any 4D expression has been evaluated
	*/
	virtual	bool				IDOC_EvalExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL) = 0;


	/** convert uniform character index (the index used by 4D commands) to actual plain text position
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				IDOC_CharPosFromUniform(sLONG& ioPos) const = 0;

	/** convert actual plain text char index to uniform char index (the index used by 4D commands)
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				IDOC_CharPosToUniform(sLONG& ioPos) const = 0;

	/** convert uniform character range (as used by 4D commands) to actual plain text position
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				IDOC_RangeFromUniform(sLONG& inStart, sLONG& inEnd) const = 0;

	/** convert actual plain text range to uniform range (as used by 4D commands)
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				IDOC_RangeToUniform(sLONG& inStart, sLONG& inEnd) const = 0;

	/** adjust selection
	@remarks
		you should call this method to adjust with surrogate pairs or span references
	*/
	virtual	void				IDOC_AdjustRange(sLONG& ioStart, sLONG& ioEnd) const = 0;

	/** adjust char pos
	@remarks
		you should call this method to adjust with surrogate pairs or span references
	*/
	virtual	void				IDOC_AdjustPos(sLONG& ioPos, bool inToEnd = true) const = 0;





	//
	// class styles (read/write per-class styles or read/write element(s) styles on a text range)
	//


	/** apply class style to the current document node and/or children depending on inTargetDocType
	@param inClassStyle
		the class style - if inClassStyle class name is not empty, apply only to current or children nodes with same class name
	@param inResetStyles
		if true, reset styles to default prior to apply new styles
	@param inReplaceClass
		if inClassStyle class is not empty:
		if true, apply class style to current document and/or children which match inTargetDocType and change node class to inClassStyle class
		if false, apply class style to current node and/or children which match both inTargetDocType and inClassStyle class
		if inClassStyle class is empty, ignore inReplaceClass and apply inClassStyle to current node and/or children which match inTargetDocType - and do not modify node class
	@param inStart
		text range start
	@param inEnd
		text range end
	@param inTargetDocType
		apply to current document node and/or children if and only if node type is equal to inTargetDocType
		(it works also with embedded images or tables)
	*/
	virtual	bool				IDOC_ApplyClassStyle(	const VDocClassStyle *inClassStyle, bool inResetStyles = false, bool inReplaceClass = false,
														sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH) = 0;


	/* get class style count per element node type */
	virtual	uLONG				IDOC_GetClassStyleCount(eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) const = 0;

	/** get class style name from index (0-based)
	@remarks
		0 <= inIndex < GetClassStyleCount(inDocNodeType)
	*/
	virtual	void				IDOC_GetClassStyleName(VIndex inIndex, VString& outClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) const = 0;

	/** apply class to the current document node and/or children depending on inTargetDocType
	@param inClass
		the class name
	@param inResetStyles
		if true, reset styles to default prior to apply class styles
	@param inStart
		text range start
	@param inEnd
		text range end
	@param inTargetDocType
		apply to current node and/or children if and only if node document type is equal to inTargetDocType
		(it works also with embedded images or tables)

	@remarks
		if might fail if there is none class style associated with the class name (you should use RetainClassStyle and/or AddOrReplaceClassStyle to edit class style prior to call this method)
	*/
	virtual	bool				IDOC_ApplyClass(const VString &inClass, bool inResetStyles = false, sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH) = 0;

	/** retain class style */
	virtual	VDocClassStyle*		IDOC_RetainClassStyle(const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) const = 0;

	/** add or replace class style
	@remarks
		class style might be modified if inDefaultStyles is not equal to NULL (as only styles not equal to default are stored)
	*/
	virtual	bool				IDOC_AddOrReplaceClassStyle(const VString& inClass, VDocClassStyle *inClassStyle, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) = 0;

	/** remove class style */
	virtual	bool				IDOC_RemoveClassStyle(const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) = 0;

	/** clear class styles */
	virtual	bool				IDOC_ClearClassStyles(bool inKeepDefaultStyles = false) = 0;

	/** create and initialize a class style from properties of the layout node at the specified character index which match the specified document node type
	@remarks
	class style class name will be equal to layout node class
	*/
	virtual	VDocClassStyle*		IDOC_CreateClassStyleFromCharPos(sLONG inPos = 0, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) = 0;

	/** get class of the layout node at the specfied character index */
	virtual	void				IDOC_GetClassFromCharPos(VString& outClass, sLONG inPos = 0, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) = 0;

};

class VSpanHandler;
class VScrap;

typedef enum eHTMLParserOptions
{
	kHTML_PARSER_TAG_IMAGE_SUPPORTED	= 1,	//if enabled, parser will parse image tags as image span references otherwise as unknown tags 
	kHTML_PARSER_TAG_TABLE_SUPPORTED	= 2,	//if enabled, parser will parse table tags as table span references otherwise as unknown tags //TODO: tables are not implemented yet

	kHTML_PARSER_DISABLE_XHTML4D		= 1024,	//if enabled, parser will ignore 4D namespace if present & parse XHTML4D as XHTML11, ignoring any 4D style

	kHTML_PARSER_PRESERVE_WHITESPACES	= 2048,	//if enabled, parser will preserve whitespaces on default (for legacy XHTML, default is to consolidate whitespaces)
												//(useful only for legacy XHTML because for XHTML4D, paragraph whitespaces are preserved with 'white-space:pre-wrap') 

} eHTMLParserOptions;

#if ENABLE_VDOCTEXTLAYOUT
#define kHTML_PARSER_OPTIONS_DEFAULT	kHTML_PARSER_TAG_IMAGE_SUPPORTED
#else
#define kHTML_PARSER_OPTIONS_DEFAULT	0
#endif


class ISpanTextParser
{
public:
#if VERSIONWIN
virtual float GetSPANFontSizeDPI() const = 0;
#endif

/** return true if character is valid XML 1.1 
	(some control & special chars are not valid like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
virtual bool IsXMLChar(UniChar inChar) const = 0;

/** convert plain text to XML 
@param ioText
	text to convert
@param inFixInvalidChars
	if true (default), fix invalid XML chars - see remarks below
@param inEscapeXMLMarkup
	if true (default), escape XML markup (<,>,',",&) 
@param inConvertCRtoBRTag
	if true (default is false), it will convert any end of line to <BR/> XML tag
@param inConvertForCSS
	if true, convert "\" to "\\" because in CSS '\' is used for escaping characters (!)  
@param inEscapeCR
	if true, escapes CR to "\0D" in CSS or "&#xD;" in XML (exclusive with inConvertCRtoBRTag) - should be used only while serializing XML or CSS strings
@param inNoEncodeQuotes
	if true, characters ''' and '"' are not encoded 
@remarks
	see IsXMLChar 

	note that end of lines are normalized to one CR only (0x0D) - if end of lines are not converted to <BR/>

	if inFixInvalidChars == true, invalid chars are converted to whitespace (but for 0x0B which is replaced with 0x0D) 
	otherwise Xerces would fail parsing invalid XML 1.1 characters - parser would throw exception
*/
virtual void TextToXML(VString& ioText, bool inFixInvalidChars = true, bool inEscapeXMLMarkup = true, bool inConvertCRtoBRTag = false, 
					   bool inConvertForCSS = false, bool inEscapeCR = false, bool inNoEncodeQuotes = false) = 0;

/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): input text is SPAN4D or XHTML4D tagged text (version depending on "d4-version" CSS property) 
	false: input text is XHTML 1.1 fragment (generally from external source than 4D) 
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRefs
	span references text mask (default is uniform text i.e. a whitespace character)

CAUTION: this method returns only parsed character styles: you should use now ParseDocument method which returns a VDocText * if you need to preserve paragraph or document properties
*/
virtual void ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly = true, const uLONG inDPI = 72, 
							SpanRefCombinedTextTypeMask inShowSpanRefs = 0, VDBLanguageContext *inLC = NULL, uLONG inOptions = kHTML_PARSER_OPTIONS_DEFAULT) = 0;

/** generate span text from text & character styles 

CAUTION: this method will serialize text align for compat with v13 but you should use now SerializeDocument (especially if text has been parsed before with ParseDocument)
*/
virtual void GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL) =0;

/** get plain text from span text 
@remarks
	span references are evaluated on default
*/
virtual void GetPlainTextFromSpanText(	const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly = true, 
										SpanRefCombinedTextTypeMask inShowSpanRefs = kSRCTT_Value, VDBLanguageContext *inLC = NULL) =0;

/** replace styled text at the specified range with text 
@remarks
	if inInheritUniformStyle == true, replaced text will inherit uniform style on replaced range (true on default)
*/
virtual bool ReplaceStyledText(	VString& ioSpanText, const VString& inSpanTextOrPlainText, sLONG inStart = 0, sLONG inEnd = -1, bool inTextPlainText = false, bool inInheritUniformStyle = true, bool inSilentlyTrapParsingErrors = true) = 0;

/** replace styled text with span text reference on the specified range
	
	you should no more use or destroy passed inSpanRef even if method returns false
*/
virtual bool ReplaceAndOwnSpanRef( VString& ioSpanText, IDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true) = 0;

/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range 

	return true if any 4D expression has been replaced with plain text
*/
virtual bool FreezeExpressions( VDBLanguageContext *inLC, VString& ioSpanText, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true) = 0;

/** return the first span reference which intersects the passed range 
@return
	eDocSpanTextRef enum value or -1 if none ref found
*/
virtual sLONG GetReferenceAtRange(	VDBLanguageContext *inLC,
								    const VString& inSpanText, 
									const sLONG inStart = 0, const sLONG inEnd = -1, 
									sLONG *outFirstRefStart = NULL, sLONG *outFirstRefEnd = NULL, 
									VString *outFirstRefSource  = NULL, VString *outFirstRefValue = NULL
									) = 0;

//document parsing

/** parse text document 
@param inDocText
	span or xhtml text
@param inVersion
	span or xhtml version (no matter the value, if source is html & 4D namespace is present, it is parsed as XHTML4D - and version is parsed from d4-version)
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRefs
	span references text mask (default is uniform text i.e. a whitespace character)
@remarks
	on default, document format is assumed to be V13 compatible (but version might be modified if "d4-version" CSS prop is present)
*/
virtual VDocText *ParseDocument(	const VString& inDocText, const uLONG inVersion = kDOC_VERSION_SPAN4D, const uLONG inDPI = 72, 
									SpanRefCombinedTextTypeMask inShowSpanRefs = 0, bool inSilentlyTrapParsingErrors = true, VDBLanguageContext *inLC = NULL, uLONG inOptions = kHTML_PARSER_OPTIONS_DEFAULT) = 0;

/** initialize VTextStyle from CSS declaration string */ 
virtual void ParseCSSToVTextStyle( VDBLanguageContext *inLC, VDocNode *ioDocNode, const VString& inCSSDeclarations, VTextStyle &outStyle, const VTextStyle *inStyleInherit = NULL) = 0;

/** initialize VDocNode props from CSS declaration string */ 
virtual void ParseCSSToVDocNode( VDocNode *ioDocNode, const VString& inCSSDeclarations, VSpanHandler *inSpanHandler = NULL) = 0;

/** parse document CSS property */	
virtual bool ParsePropDoc( VDocText *ioDocText, const uLONG inProperty, const VString& inValue) = 0;

/** parse paragraph CSS property */	
virtual bool ParsePropParagraph( VDocParagraph *ioDocPara, const uLONG inProperty, const VString& inValue) = 0;

/** parse common CSS property */	
virtual bool ParsePropCommon( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue) = 0;

/** parse span CSS style */	
virtual bool ParsePropSpan( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue, VTextStyle &outStyle, VDBLanguageContext *inLC = NULL) = 0;


//document serializing

/** serialize document (formatting depending on document version) 
@remarks
	if (inDocDefaultStyle != NULL):
		if inSerializeDefaultStyle, serialize inDocDefaultStyle properties & styles
		if !inSerializeDefaultStyle, only serialize properties & styles which are not equal to inDocDefaultStyle properties & styles
*/
virtual void SerializeDocument(const VDocText *inDoc, VString& outDocText, const VDocClassStyleManager *inDocDefaultStyleMgr = NULL, bool inSerializeDefaultStyle = false, bool inSerialize4DExpValue = false) = 0;
};

END_TOOLBOX_NAMESPACE

#endif

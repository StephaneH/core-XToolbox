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
#ifndef __VTextLayout__
#define __VTextLayout__

#include "Kernel/Sources/VTextStyle.h"

/** default selection color (Word-like color) */
#define VTEXTLAYOUT_SELECTION_ACTIVE_COLOR		XBOX::VColor(168,205,241)
#define VTEXTLAYOUT_SELECTION_INACTIVE_COLOR	XBOX::VColor(227,227,227)

/** placeholder color */
#define VTEXTLAYOUT_COLOR_PLACEHOLDER			XBOX::VColor(177,169,187)

class VDBLanguageContext;

BEGIN_TOOLBOX_NAMESPACE

// Forward declarations

class	VAffineTransform;
class	VRect;
class	VRegion;
class 	VTreeTextStyle;
class 	VStyledTextBox;

class	IDocNodeLayoutUserDelegate;
class	VDocParagraph;
class	VDocText;
class	VDocClassStyle;

class	VDocBaseLayout;
class	VDocParagraphLayout;
class	VDocTextLayout;

struct	sDocBorder;
struct	sDocBorderRect;

#if ENABLE_D2D
class XTOOLBOX_API VWinD2DGraphicContext;
#endif
#if VERSIONMAC
class XTOOLBOX_API VMacQuartzGraphicContext;
#endif

class XTOOLBOX_API ITextLayout;


class XTOOLBOX_API IDocBaseLayout : public ISpanTextRefLayout
{
public:

	/** query text layout interface (if available for this class instance)
	@remarks
	for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
	might return NULL

	important: interface is NOT retained
	*/
	virtual	ITextLayout*		GetTextLayoutInterface() { return NULL; }

	/** return document node type */
	virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_UNKNOWN; }

	virtual	VDocTextLayout*		GetDocTextLayout() const { return NULL; }

	/** get current ID */
	virtual	const VString&		GetID() const { xbox_assert(false); return sEmptyString; }

	/** set current ID (custom ID) */
	virtual	bool				SetID(const VString& inValue) { xbox_assert(false); return false; }

	/** get class*/
	virtual	const VString&		GetClass() const { xbox_assert(false); return sEmptyString; }

	/** set class */
	virtual	void				SetClass(const VString& inClass) { xbox_assert(false); }

	/** set layout DPI */
	virtual	void				SetDPI(const GReal inDPI = 72) = 0;
	virtual GReal				GetDPI() const = 0;

	/** update layout metrics */
	virtual void				UpdateLayout(VGraphicContext *inGC) = 0;

	/** render layout element in the passed graphic context at the passed top left origin
	@remarks
	method does not save & restore gc context
	*/
	virtual void				Draw(VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f) = 0;

	/** update ascent (compute ascent relatively to line & text ascent, descent in px (relative to computing dpi) - not including line embedded node(s) metrics)
	@remarks
	mandatory only if node is embedded in a paragraph line (image for instance)
	should be called after UpdateLayout
	*/
	virtual	void				UpdateAscent(const GReal inLineAscent = 0, const GReal inLineDescent = 0, const GReal inTextAscent = 0, const GReal inTextDescent = 0, const AlignStyle inDefaultAlign = AL_DEFAULT) {}

	/** get ascent (in px - relative to computing dpi)
	@remarks
	mandatory only if node is embedded in a paragraph line (image for instance)
	should be called after UpdateAscent
	*/
	virtual	GReal				GetAscent() const { return 0; }

	/** get descent (in px - relative to computing dpi)
	@remarks
	mandatory only if node is embedded in a paragraph line (image for instance)
	should be called after UpdateAscent
	*/
	virtual	GReal				GetDescent() const { return 0; }

	/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
	virtual	const VRect&		GetLayoutBounds() const = 0;

	/** get typo local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
	virtual	const VRect&		GetTypoBounds() const { return GetLayoutBounds(); }

	/** get background bounds (in px - relative to computing dpi) */
	virtual	const VRect&		GetBackgroundBounds(eBackgroundBox inBackgroundBox = kBACKGROUND_BORDER_BOX) const = 0;

	/** apply class style to the current layout and/or children depending on inTargetDocType
	@param inClassStyle (mandatory only for VDocTextLayout impl)
	the class style - if inClassStyle class name is not empty, apply only to current or children nodes with same class name
	@param inResetStyles
	if true, reset styles to default prior to apply new styles
	@param inReplaceClass (mandatory only for VDocTextLayout impl)
	if inClassStyle class is not empty:
	if true, apply class style to current layout and/or children which match inTargetDocType and change layout class to inClassStyle class
	if false, apply class style to current layout and/or children which match both inTargetDocType and inClassStyle class
	if inClassStyle class is empty, ignore inReplaceClass and apply inClassStyle to current layout and/or children which match inTargetDocType - and do not modify layout class
	@param inStart (mandatory only if current layout is a container)
	text range start
	@param inEnd (mandatory only if current layout is a container)
	text range end
	@param inTargetDocType
	apply to current layout and/or children if and only if layout document type is equal to inTargetDocType
	(it works also with embedded images or tables)
	*/
	virtual	bool				ApplyClassStyle(const VDocClassStyle *inClassStyle, bool inResetStyles = false, bool inReplaceClass = false,
		sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH) {
		return false;
	}

	/* get class style count per element node type */
	virtual	uLONG				GetClassStyleCount(eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) const { return 0; }

	/** get class style name from index (0-based)
	@remarks
	0 <= inIndex < GetClassStyleCount(inDocNodeType)
	*/
	virtual	void				GetClassStyleName(VIndex inIndex, VString& outClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) const {}

	/** apply class to the current layout and/or children depending on inTargetDocType
	(mandatory only for VDocTextLayout impl)
	@param inClass
	the class name
	@param inResetStyles
	if true, reset styles to default prior to apply class styles
	@param inStart (mandatory only if current layout is a container)
	text range start
	@param inEnd (mandatory only if current layout is a container)
	text range end
	@param inTargetDocType
	apply to current layout and/or children if and only if layout document type is equal to inTargetDocType
	(it works also with embedded images or tables)

	@remarks
	if might fail if there is none class style associated with the class name (you should use RetainClassStyle and/or AddOrReplaceClassStyle to edit class style prior to call this method)
	*/
	virtual	bool				ApplyClass(const VString &inClass, bool inResetStyles = false, sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH) { return false; }

	/** retain class style */
	virtual	VDocClassStyle*		RetainClassStyle(const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH, bool inAppendDefaultStyles = false) const { return NULL; }

	/** add or replace class style
	@remarks
	class style might be modified if inDefaultStyles is not equal to NULL (as only styles not equal to default are stored)
	*/
	virtual	bool				AddOrReplaceClassStyle(const VString& inClass, VDocClassStyle *inClassStyle, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH, bool inRemoveDefaultStyles = false) { return false; }

	/** remove class style */
	virtual	bool				RemoveClassStyle(const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { return false; }

	/** clear class styles */
	virtual	bool				ClearClassStyles(bool inKeepDefaultStyles) { return false; }

	/** create and initialize a class style from properties of the layout node at the specified character index which match the specified document node type
	@remarks
		class style class name will be equal to layout node class
	*/
	virtual	VDocClassStyle*		CreateClassStyleFromCharPos(sLONG inPos = 0, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { return NULL; }

	/** get class of the layout node at the specfied character index */
	virtual	void				GetClassFromCharPos(VString& outClass, sLONG inPos = 0, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { outClass.SetEmpty(); }


	/** accessor on class style manager */
	virtual	VDocClassStyleManager*	GetClassStyleManager() const { xbox_assert(false); return NULL; }

	/** retain layout node which matches the passed ID (will return always NULL if node is not attached to a VDocTextLayout) */
	virtual	VDocBaseLayout*		RetainLayout(const VString& inID) { xbox_assert(false); return NULL; }

	/** get text start index
	@remarks
	index is relative to parent layout text start index
	(if this layout has not a parent, it should be always 0)
	*/
	virtual	sLONG				GetTextStart() const { return 0; }

	/** return local text length
	@remarks
	it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
	*/
	virtual	uLONG				GetTextLength() const = 0;

	/** get parent layout reference
	@remarks
	for instance for a VDocParagraphLayout instance, it might be a VDocContainerLayout instance (paragraph layout container)
	(but as VDocParagraphLayout instance can be used stand-alone, it might be NULL too)
	*/
	virtual IDocBaseLayout*		GetParent() { return NULL; }

	/** get count of child layout instances */
	virtual	uLONG				GetChildCount() const { return 0; }

	/** get layout child index (relative to parent child vector & 0-based) */
	virtual sLONG				GetChildIndex() const { return 0; }

	/** add child layout instance */
	virtual void				AddChild(IDocBaseLayout* inLayout) { xbox_assert(false); }

	/** insert child layout instance (index is 0-based) */
	virtual void				InsertChild(IDocBaseLayout* inLayout, sLONG inIndex = 0) { xbox_assert(false); }

	/** remove child layout instance (index is 0-based) */
	virtual void				RemoveChild(sLONG inIndex) { xbox_assert(false); }

	/** get child layout instance from index (0-based) */
	virtual	IDocBaseLayout*		GetNthChild(sLONG inIndex) const { return NULL; }

	/** get first child layout instance which intersects the passed text index */
	virtual IDocBaseLayout*		GetFirstChild(sLONG inStart = 0) const { return NULL; }

	virtual	bool				IsLastChild(bool inLookUp = false) const { return true; }

	/** set outside margins	(in twips) - CSS margin */
	virtual	void				SetMargins(const sRectCSS& inMargins, bool inCalcAllMargin = true) = 0;
	/** get outside margins (in twips) */
	virtual	void				GetMargins(sRectCSS& outMargins) const = 0;

	/** set inside margins	(in twips) - CSS padding */
	virtual	void				SetPadding(const sRectCSS& inPadding, bool inCalcAllMargin = true) = 0;
	/** get inside margins	(in twips) - CSS padding */
	virtual	void				GetPadding(sRectCSS& outPadding) const = 0;

	/** set text align - CSS text-align (-1 for not defined) */
	virtual void				SetTextAlign(const AlignStyle inHAlign) = 0;
	virtual AlignStyle			GetTextAlign() const = 0;

	/** set vertical align - CSS vertical-align (AL_DEFAULT for default) */
	virtual void				SetVerticalAlign(const AlignStyle inVAlign) = 0;
	virtual AlignStyle			GetVerticalAlign() const = 0;

	/** set width (in twips) - CSS width (0 for auto) */
	virtual	void				SetWidth(const uLONG inWidth) = 0;
	virtual uLONG				GetWidth(bool inIncludeAllMargins = false) const = 0;

	/** set height (in twips) - CSS height (0 for auto) */
	virtual	void				SetHeight(const uLONG inHeight) = 0;
	virtual uLONG				GetHeight(bool inIncludeAllMargins = false) const = 0;

	/** set min width (in twips) - CSS min-width (0 for auto) */
	virtual	void				SetMinWidth(const uLONG inWidth) = 0;
	virtual uLONG				GetMinWidth(bool inIncludeAllMargins = false) const = 0;

	/** set min height (in twips) - CSS min-height (0 for auto) */
	virtual	void				SetMinHeight(const uLONG inHeight) = 0;
	virtual uLONG				GetMinHeight(bool inIncludeAllMargins = false) const = 0;

	/** set background color - CSS background-color (transparent for not defined) */
	virtual void				SetBackgroundColor(const VColor& inColor) = 0;
	virtual const VColor&		GetBackgroundColor() const = 0;

	/** set background clipping - CSS background-clip */
	virtual void				SetBackgroundClip(const eBackgroundBox inBackgroundClip) = 0;
	virtual eBackgroundBox		GetBackgroundClip() const = 0;

	/** set background origin - CSS background-origin */
	virtual void				SetBackgroundOrigin(const eBackgroundBox inBackgroundOrigin) = 0;
	virtual eBackgroundBox		GetBackgroundOrigin() const = 0;

	/** set borders
	@remarks
	if inLeft is NULL, left border is same as right border
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
	if inTop is NULL, top border is default border (solid, medium width & black color)
	*/
	virtual	void				SetBorders(const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL) = 0;

	/** clear borders */
	virtual	void				ClearBorders() = 0;

	/** set background image
	@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
	virtual	void				SetBackgroundImage(const VString& inURL,
		const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top,
		const eBackgroundRepeat inRepeat = kBACKGROUND_REPEAT,
		const GReal inWidth = kBACKGROUND_SIZE_AUTO, const GReal inHeight = kBACKGROUND_SIZE_AUTO,
		const bool inWidthIsPercentage = false, const bool inHeightIsPercentage = false) = 0;

	/** clear background image */
	virtual	void				ClearBackgroundImage() = 0;

protected:
	static	VString				sEmptyString;
};


/** common interface shared by classes VDocParagraphLayout, VDocTextLayout & VTextLayout (VDocParagraphLayout & VDocTextLayout are reserved for Write object only)

@remarks
text indexs parameters are always relative to the current layout instance (GetTextStart() returns the text offset relative to parent):
so to convert a text index from the local layout to the parent layout, add GetTextStart()
and to convert a text index from the parent layout to the local layout, subtract GetTextStart()

if the method scope is explicitly paragraph layout instance(s) (like SetTextAlign, SetLineHeight, SetTextIndent, etc...) and if current instance is a layout container:
Set... method is applied to all children VDocParagraphLayout instances which intersect the passed text range and
Get... method is applied to the first child VDocParagraphLayout instance which intersect the passed text range
(paragraph only methods are applied to the current layout instance if it is a VDocParagraphLayout instance yet);
exceptions are explicitly specified in method commentary
*/

class XTOOLBOX_API ITextLayout : public IDocBaseLayout
{
public:

	virtual	bool				IsImplDocLayout() const { return true; } //false if class is VTextLayout (obsolete native implementation)

	/////////////////	IDocBaseLayout interface

	/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
	virtual	const VRect&		GetLayoutBounds() const = 0;

	/** get typo local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
	virtual	const VRect&		GetTypoBounds() const { return GetLayoutBounds(); }

	/** query text layout interface (if available for this class instance)
	@remarks
	for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
	might return NULL

	important: interface is NOT retained
	*/
	virtual	ITextLayout*		GetTextLayoutInterface() { return this; }

	/** set outside margins	(in twips) - CSS margin */
	virtual	void				SetMargins(const sRectCSS& inMargins, bool inCalcAllMargin = true) = 0;
	/** get outside margins (in twips) */
	virtual	void				GetMargins(sRectCSS& outMargins) const = 0;

	////////////////

	virtual	bool				IsDirty() const = 0;
	/** set dirty */
	virtual	void				SetDirty(bool inApplyToChildren = false, bool inApplyToParent = true) = 0;

	virtual	void				SetPlaceHolder(const VString& inPlaceHolder)	{}
	virtual	void				GetPlaceHolder(VString& outPlaceHolder) const { outPlaceHolder.SetEmpty(); }

	/** enable/disable password mode */
	virtual	void				EnablePassword(bool inEnable) {}
	virtual bool				IsEnabledPassword() const { return false; }

	/** enable/disable cache */
	virtual	void				ShouldEnableCache(bool inShouldEnableCache) = 0;
	virtual	bool				ShouldEnableCache() const = 0;


	/** allow/disallow offscreen text rendering (default is false)
	@remarks
	text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
	you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
	@see
	ShouldEnableCache
	*/
	virtual	void				ShouldDrawOnLayer(bool inShouldDrawOnLayer) = 0;
	virtual	bool				ShouldDrawOnLayer() const = 0;
	virtual void				SetLayerDirty(bool inApplyToChildren = false) = 0;

	/** paint selection */
	virtual	void				ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor = NULL, const VColor* inInactiveColor = NULL) = 0;
	virtual	bool				ShouldPaintSelection() const = 0;
	/** change selection (selection is used only for painting) */
	virtual	void				ChangeSelection(sLONG inStart, sLONG inEnd = -1, bool inActive = true) = 0;
	virtual	bool				IsSelectionActive() const = 0;

	/** attach or detach a layout user delegate */
	virtual	void				SetLayoutUserDelegate(IDocNodeLayoutUserDelegate *inDelegate, void *inUserData) {}
	virtual	IDocNodeLayoutUserDelegate* GetLayoutUserDelegate() const { return NULL; }

	/** create and retain document from the current layout
	@param inStart
	range start
	@param inEnd
	range end

	@remarks
	if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
	with a single child node initialized from the current layout
	*/
	virtual	VDocText*			CreateDocument(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** create and retain paragraph from the current layout plain text, styles & paragraph properties (only first paragraph properties are copied)
	@param inStart
	range start
	@param inEnd
	range end
	*/
	virtual	VDocParagraph*		CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** init layout from the passed document
	@param inDocument
	document source
	@remarks
	the passed document is used only to initialize layout: document is not retained
	//TODO: maybe we should optionally bind layout with the document class instance
	*/
	virtual	void				InitFromDocument(const VDocText *inDocument) { xbox_assert(false); }

	/** init layout from the passed paragraph
	@param inPara
	paragraph source
	@remarks
	the passed paragraph is used only to initialize layout: paragraph is not retained
	*/
	virtual	void				InitFromParagraph(const VDocParagraph *inPara) = 0;

	/** replace layout range with the specified document fragment */
	virtual	void				ReplaceDocument(const VDocText *inDoc, sLONG inStart = 0, sLONG inEnd = -1,
		bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true,
		void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL) {
		xbox_assert(false);
	}

	/** reset all text properties & character styles to default values */
	virtual	void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;


	/** merge paragraph(s) on the specified range to a single paragraph
	@remarks
	if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
	resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR

	remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
	you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
	*/
	virtual	bool				MergeParagraphs(sLONG inStart = 0, sLONG inEnd = -1) { return false; }

	/** unmerge paragraph(s) on the specified range
	@remarks
	any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR,
	resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

	remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
	you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
	*/
	virtual	bool				UnMergeParagraphs(sLONG inStart = 0, sLONG inEnd = -1) { return false; }

	/** retain layout at the specified character index which matches the specified document node type */
	virtual	VDocBaseLayout*		RetainLayoutFromCharPos(sLONG inPos, const eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { return NULL; }

	virtual	bool				HasSpanRefs() const = 0;

	/** replace text with span text reference on the specified range
	@remarks
	it will insert plain text as returned by VSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

	you should no more use or destroy passed inSpanRef even if method returns false

	IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
	so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
	(passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens
	- if display type is set to normalized source)
	*/
	virtual	bool				ReplaceAndOwnSpanRef(ISpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = false,
		void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;

	/** return span reference at the specified text position if any */
	virtual	const VTextStyle*	GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const = 0;

	/** return the first span reference which intersects the passed range */
	virtual	const VTextStyle*	GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const = 0;

	/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */
	virtual	bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;

	/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
	virtual	bool				FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL) = 0;

	/** eval 4D expressions
	@remarks
	note that it only updates span 4D expression references but does not update plain text:
	call UpdateSpanRefs after to update layout plain text

	return true if any 4D expression has been evaluated
	*/
	virtual	bool				EvalExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL) = 0;


	/** convert uniform character index (the index used by 4D commands) to actual plain text position
	@remarks
	in order 4D commands to use uniform character position while indexing text containing span references
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				CharPosFromUniform(sLONG& ioPos) const = 0;

	/** convert actual plain text char index to uniform char index (the index used by 4D commands)
	@remarks
	in order 4D commands to use uniform character position while indexing text containing span references
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				CharPosToUniform(sLONG& ioPos) const = 0;

	/** convert uniform character range (as used by 4D commands) to actual plain text position
	@remarks
	in order 4D commands to use uniform character position while indexing text containing span references
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				RangeFromUniform(sLONG& inStart, sLONG& inEnd) const = 0;

	/** convert actual plain text range to uniform range (as used by 4D commands)
	@remarks
	in order 4D commands to use uniform character position while indexing text containing span references
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
	*/
	virtual	void				RangeToUniform(sLONG& inStart, sLONG& inEnd) const = 0;

	/** adjust selection
	@remarks
	you should call this method to adjust with surrogate pairs or span references
	*/
	virtual	void				AdjustRange(sLONG& ioStart, sLONG& ioEnd) const = 0;

	/** adjust char pos
	@remarks
	you should call this method to adjust with surrogate pairs or span references
	*/
	virtual	void				AdjustPos(sLONG& ioPos, bool inToEnd = true) const = 0;


	/** set layer view rectangle (user space is VView window user space - hwnd user space)
	@remarks
	useful only if offscreen layer is enabled

	by default, layer surface size is equal to text layout size in pixels
	but it can be useful to constraint layer surface size to not more than the visible area size of the text layout container
	in order to avoid to draw offscreen all the text layout (especially if the text size is great...)

	for instance if VDocParagraphLayout container is a VView-derived class & fTextLayout is the text layout attached or used in the VView-derived class,
	in the VView-derived class DoDraw method & prior to call VDocParagraphLayout::Draw, you should insert this code to set the layer view rect:

	[code/]

	VRect boundsHwnd;
	GetHwndBounds(boundsHwnd);
	fTextLayout->SetLayerViewRect( boundsHwnd);

	[/code]

	with that code, layer size will be not greater than min of layout size & VView visible area
	(see class VStyledTextEdtiView for a sample)

	You can still reset to VRect() to use layout size as layer size if you intend to pre-render all text layout
	(only if you have called before SetLayerViewRect with a not empty VRect)
	*/
	virtual	void				SetLayerViewRect(const VRect& inRectRelativeToWnd) = 0;
	virtual	const VRect&		GetLayerViewRect() const = 0;

	/** insert text at the specified position */
	virtual	void				InsertPlainText(sLONG inPos, const VString& inText) = 0;

	/** delete text range */
	virtual	void				DeletePlainText(sLONG inStart, sLONG inEnd) = 0;

	/** replace text */
	virtual	void				ReplacePlainText(sLONG inStart, sLONG inEnd, const VString& inText) = 0;

	/** apply style (use style range) */
	virtual	bool				ApplyStyle(const VTextStyle* inStyle, bool inFast = false, bool inIgnoreIfEmpty = true) = 0;

	virtual	bool				ApplyStyles(const VTreeTextStyle * inStyles, bool inFast = false) = 0;

	/** show span references source or value */
	virtual	void				ShowRefs(SpanRefCombinedTextTypeMask inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;
	virtual	SpanRefCombinedTextTypeMask ShowRefs() const = 0;

	/** highlight span references in plain text or not */
	virtual	void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false) = 0;
	virtual	bool				HighlightRefs() const = 0;

	/** call this method to update span reference style while mouse enters span ref
	@remarks
	return true if style has changed
	*/
	virtual	bool				NotifyMouseEnterStyleSpanRef(const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0) = 0;

	/** call this method to update span reference style while mouse leaves span ref
	@remarks
	return true if style has changed
	*/
	virtual	bool				NotifyMouseLeaveStyleSpanRef(const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0) = 0;

	/** set default font
	@remarks
	it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)

	by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
	*/
	virtual	void				SetDefaultFont(const VFont *inFont, GReal inDPI = 72.0f, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** get & retain default font
	@remarks
	by default, return 4D form-compliant font (with dpi = 72) so not the actual fRenderDPI-scaled font
	(for consistency with SetDefaultFont & because fRenderDPI internal scaling should be transparent for caller)
	*/
	virtual	VFont*				RetainDefaultFont(GReal inDPI = 72.0f, sLONG inStart = 0) const = 0;

	/** set default text color
	@remarks
	black color on default (it is applied prior styles set with SetText)
	*/
	virtual	void				SetDefaultTextColor(const VColor &inTextColor, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** get default text color */
	virtual	bool				GetDefaultTextColor(VColor& outColor, sLONG inStart = 0) const = 0;

	/** get default text back color */
	virtual	bool				GetDefaultTextBackColor(VColor& outColor) const { outColor.FromRGBAColor(RGBACOLOR_TRANSPARENT); return true; }

	/** set back color
	@param  inColor
	background color
	@param	inPaintBackColor
	unused (preserved for compatibility with VTextLayout)
	@remarks
	OBSOLETE: you should use SetBackgroundColor - it is preserved for compatibility with VTextLayout
	*/
	virtual	void				SetBackColor(const VColor& inColor, bool inPaintBackColor = false) = 0;
	virtual	const VColor&		GetBackColor() const = 0;

	/** replace current text & styles
	@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles MIGHT be retained: in that case, caller should not modify passed styles

	this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
	*/
	virtual	void				SetText(const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false) = 0;

	/** replace current text & styles
	@remarks
	here passed styles are always copied

	this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
	*/
	virtual	void				SetText(const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd = -1,
		bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true,
		const ISpanTextRef * inPreserveSpanTextRef = NULL,
		void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL)
	{
		VTreeTextStyle *styles = new VTreeTextStyle(inStyles);
		SetText(inText, styles, false);
		ReleaseRefCountable(&styles);
	}

	/** const accessor on paragraph text
	@remarks
	if it is a container, return first paragraph text
	*/
	virtual	const VString&		GetText() const = 0;

	virtual	void				GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const { outText = GetText(); }


	/** return unicode character at the specified position (0-based) */
	virtual	UniChar				GetUniChar(sLONG inIndex) const = 0;

	/* get number of lines */
	virtual sLONG				GetNumberOfLines(VGraphicContext *inGC) = 0;
				
	/** return styles on the passed range
	@remarks
	if inIncludeDefaultStyle == false (default is true) and if layout is a paragraph, it does not include paragraph uniform style (default style) but only styles which are not uniform on paragraph text:
	otherwise it returns all styles applied to the plain text

	caution: if inCallerOwnItAlways == false (default is true), returned styles might be shared with layout instance so you should not modify it
	*/
	virtual	VTreeTextStyle*		RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inIncludeDefaultStyle = true, bool inCallerOwnItAlways = true) const = 0;

	/** set extra text styles
	@remarks
	it can be used to add a temporary style effect to the text layout without modifying the main text styles
	(these styles are not exported to VDocParagraph by CreateParagraph)

	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again

	TODO: should be obsolete for VDocTextLayout because for VDocTextLayout extended styles are used only to render automatic urls style:
	it would be better in that case to manage automatic urls as volatile url span references directly in VDocParagraphLayout class
	(for visible urls only)
	*/
	virtual	void				SetExtStyles(VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false) = 0;

	virtual	VTreeTextStyle*		RetainExtStyles() const = 0;

	/** set max text width (0 for infinite) - in pixel relative to the layout target DPI
	@remarks
	it is also equal to text box width
	so you should specify one if text is not aligned to the left border
	otherwise text layout box width will be always equal to formatted text width
	*/
	virtual	void				SetMaxWidth(GReal inMaxWidth = 0.0f) = 0;
	virtual	GReal				GetMaxWidth() const = 0;

	/** set max text height (0 for infinite) - in pixel relative to the layout target DPI
	@remarks
	it is also equal to text box height
	so you should specify one if paragraph is not aligned to the top border
	otherwise text layout box height will be always equal to formatted text height
	*/
	virtual	void				SetMaxHeight(GReal inMaxHeight = 0.0f) = 0;
	virtual	GReal				GetMaxHeight() const = 0;

	/** replace ouside margins	(in point) */
	virtual	void				SetMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f)
	{
		//to avoid math discrepancies from res-independent point to current DPI, we always store in TWIPS & convert from TWIPS

		sRectCSS margin;
		margin.left = ICSSUtil::PointToTWIPS(inMarginLeft);
		margin.right = ICSSUtil::PointToTWIPS(inMarginRight);
		margin.top = ICSSUtil::PointToTWIPS(inMarginTop);
		margin.bottom = ICSSUtil::PointToTWIPS(inMarginBottom);
		SetMargins(margin);
	}

	/** get outside margins (in point) */
	virtual	void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom) const
	{
		sRectCSS margin;
		GetMargins(margin);
		if (outMarginLeft)
			*outMarginLeft = ICSSUtil::TWIPSToPoint(margin.left);
		if (outMarginRight)
			*outMarginRight = ICSSUtil::TWIPSToPoint(margin.right);
		if (outMarginTop)
			*outMarginTop = ICSSUtil::TWIPSToPoint(margin.top);
		if (outMarginBottom)
			*outMarginBottom = ICSSUtil::TWIPSToPoint(margin.bottom);
	}

	/** get paragraph line height

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height
	*/
	virtual	GReal				GetLineHeight(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** set paragraph line height

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height
	*/
	virtual	void				SetLineHeight(const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	virtual	bool				IsRTL(sLONG inStart = 0, sLONG inEnd = -1) const = 0;
	virtual	void				SetRTL(bool inRTL, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** get first line padding offset in point
	@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
	*/
	virtual	GReal				GetTextIndent(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** set first line padding offset in point
	@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
	*/
	virtual	void				SetTextIndent(const GReal inPadding, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** get tab stop default offset in point */
	virtual	GReal				GetTabStopOffset(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** get tab stop default type */
	virtual	eTextTabStopType	GetTabStopType(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** set tab stop table as a single offset (offset in point) */
	virtual	void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** set tab stop table (offsets in TWIPS) */
	virtual	void				SetTabStops(const VectorOfSLONG& inOffsets, const VectorOfSLONG& inTypes, sLONG inStart = 0, sLONG inEnd = -1)	{ xbox_assert(false); }

	/** get tab stop offsets (in TWIPS) */
	virtual	const VectorOfSLONG&	GetTabStopOffsets(sLONG inStart = 0, sLONG inEnd = -1) const { xbox_assert(false); return sTempVectorOfSLONG; }

	/** get tab stop types */
	virtual	const VectorOfSLONG&	GetTabStopTypes(sLONG inStart = 0, sLONG inEnd = -1) const { xbox_assert(false); return sTempVectorOfSLONG; }

	/** set text layout mode (default is TLM_NORMAL) */
	virtual	void				SetLayoutMode(TextLayoutMode inLayoutMode = TLM_NORMAL, sLONG inStart = 0, sLONG inEnd = -1) = 0;
	/** get text layout mode */
	virtual	TextLayoutMode		GetLayoutMode(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** set class for new paragraph on RETURN */
	virtual	void				SetNewParaClass(const VString& inClass, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** get class for new paragraph on RETURN */
	virtual	const VString&		GetNewParaClass(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** set list style type
	@remarks
	reset all list item properties if inType == kLIST_STYLE_TYPE_NONE
	(because other list item properties are meaningless if paragraph is not a list item)
	*/
	virtual	void				SetListStyleType(eListStyleType inType, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list style type */
	virtual	eListStyleType		GetListStyleType(sLONG inStart = 0, sLONG inEnd = -1) const { return kLIST_STYLE_TYPE_NONE; }

	/** set list string format LTR */
	virtual	void				SetListStringFormatLTR(const VString& inStringFormat, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list string format LTR */
	virtual	const VString&		GetListStringFormatLTR(sLONG inStart = 0, sLONG inEnd = -1) const { return sEmptyString; }

	/** set list string format RTL */
	virtual	void				SetListStringFormatRTL(const VString& inStringFormat, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list string format RTL */
	virtual	const VString&		GetListStringFormatRTL(sLONG inStart = 0, sLONG inEnd = -1) const { return sEmptyString; }

	/** set list font family */
	virtual	void				SetListFontFamily(const VString& inFontFamily, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list font family */
	virtual	const VString&		GetListFontFamily(sLONG inStart = 0, sLONG inEnd = -1) const { return sEmptyString; }

	/** set list image url */
	virtual	void				SetListStyleImage(const VString& inURL, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list image url */
	virtual	const VString&		GetListStyleImage(sLONG inStart = 0, sLONG inEnd = -1) const { return sEmptyString; }

	/** set list image height in point (0 for auto) */
	virtual	void				SetListStyleImageHeight(GReal inHeight, sLONG inStart = 0, sLONG inEnd = -1) {}
	/** get list image height in point (0 for auto) */
	virtual	GReal				GetListStyleImageHeight(sLONG inStart = 0, sLONG inEnd = -1) const { return 0; }

	/** set or clear list start number */
	virtual	void				SetListStart(sLONG inListStartNumber = 1, sLONG inStart = 0, sLONG inEnd = -1, bool inClear = false) {}
	/** get list start number
	@remarks
	return true and start number if start number is set otherwise return false
	*/
	virtual	bool				GetListStart(sLONG* outListStartNumber = NULL, sLONG inStart = 0, sLONG inEnd = -1) const { return false; }

	/** begin using text layout for the specified gc (reentrant)
	@remarks
	for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
	otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
	But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

	CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
	otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

	If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
	*/
	virtual	void				BeginUsingContext(VGraphicContext *inGC, bool inNoDraw = false) = 0;

	/** return text layout bounds including margins (in gc user space)
	@remarks
	text layout origin is set to inTopLeft
	on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

	on input, max width is determined by GetMaxWidth() & max height by GetMaxHeight()
	(if max width or max height is equal to 0.0f, it is assumed to be infinite & so layout bounds will be equal to formatted text bounds)
	output layout width will be equal to max width if maw width != 0 otherwise to formatted text width
	output layout height will be equal to max height if maw height != 0 otherwise to formatted text height
	(see CAUTION remark below)

	if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds
	(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
	if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

	CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
	because layout bounds are equal to the text box design bounds and not to the formatted text bounds:
	you should use GetTypoBounds in order to get formatted text width & height
	*/
	virtual	void				GetLayoutBounds(VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) = 0;

	/** return bounding rectangle for child layout node(s) matching the passed document node type intersecting the passed text range
	or current layout bounds if current layout document type matches the passed document node type
	@remarks
	text layout origin is set to inTopLeft (in gc user space)
	output bounds are in gc user space
	*/
	virtual void				GetLayoutBoundsFromRange(VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1, const eDocNodeType inChildDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { GetLayoutBounds(inGC, outBounds, inTopLeft, true); }

	/** return formatted text bounds (typo bounds) including margins (in gc user space)
	@remarks
	text typo bounds origin is set to inTopLeft

	as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
	because layout bounds are equal to the text box design bounds and not to the formatted text bounds
	you should use this method to get formatted text width & height
	*/
	virtual	void				GetTypoBounds(VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) = 0;

	/** return text layout run bounds from the specified range (in gc user space)
	@remarks
	text layout origin is set to inTopLeft
	*/
	virtual	void				GetRunBoundsFromRange(VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/** return the caret position & height of text at the specified text position in the text layout (in gc user space)
	@remarks
	text layout origin is set to inTopLeft

	text position is 0-based

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
	if text layout is drawed at inTopLeft

	by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
	but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
	virtual	void				GetCaretMetricsFromCharIndex(VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true) = 0;

	/** return the text position at the specified coordinates
	@remarks
	text layout origin is set to inTopLeft (in gc user space)
	the hit coordinates are defined by inPos param (in gc user space)

	output text position is 0-based

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
	*/
	virtual	bool				GetCharIndexFromPos(VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL) = 0;

	/** get character range which intersects the passed bounds (in gc user space)
	@remarks
	text layout origin is set to inTopLeft (in gc user space)

	char range is rounded to line run limit so the returned range might be slightly greater than the actual char range that intersects the passed bounds
	*/
	virtual	void				GetCharRangeFromRect(VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd) = 0;


	/** move character index to the nearest character index on the line before/above the character line
	@remarks
	return false if character is on the first line (ioCharIndex is not modified)
	*/
	virtual	bool				MoveCharIndexUp(VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex) = 0;

	/** move character index to the nearest character index on the line after/below the character line
	@remarks
	return false if character is on the last line (ioCharIndex is not modified)
	*/
	virtual	bool				MoveCharIndexDown(VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex) = 0;

	/** return true if layout uses truetype font only */
	virtual	bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const = 0;

	/** end using text layout (reentrant) */
	virtual	void				EndUsingContext() = 0;

	virtual	VTreeTextStyle*		RetainDefaultStyle(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

	/** consolidate end of lines
	@remarks
	if text is multiline:
	we convert end of lines to a single CR
	if text is singleline:
	if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
	if inEscaping, we escape line endings and tab
	*/
	virtual	bool				ConsolidateEndOfLines(bool inIsMultiLine = true, bool inEscaping = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;

	/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) range at the specified character index */
	virtual	void				GetWordRange(VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces = true, bool inTrimPunctuation = false);

	/**	get paragraph range at the specified character index
	@remarks
	if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
	(for VTextLayout, parameter is ignored)
	*/
	virtual	void				GetParagraphRange(VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange = false);

	/////// helper methods


	/** consolidate end of lines
	@remarks
	if text is multiline:
	we convert end of lines to a single CR
	if text is singleline:
	if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
	if inEscaping, we escape line endings and tab
	*/
	static	bool				ConsolidateEndOfLines(const VString &inText, VString &outText, VTreeTextStyle *ioStyles, bool inIsMultiLine = true, bool inEscaping = false);

	static	void				AdjustWithSurrogatePairs(sLONG& ioStart, sLONG &ioEnd, const VString& inText);
	static	void				AdjustWithSurrogatePairs(sLONG& ioPos, const VString& inText, bool inToEnd = true);

	/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) or paragraph range (if inGetParagraph == true) at the specified character index */
	static	void				GetWordOrParagraphRange(VIntlMgr *inIntlMgr, const VString& inText, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inGetParagraph = false, bool inTrimSpaces = true, bool inTrimPunctuation = false);

	static	AlignStyle			JustToAlignStyle(const eStyleJust inJust);

	static	eStyleJust			JustFromAlignStyle(const AlignStyle inJust);
	
	static	GReal				RoundMetrics(VGraphicContext *inGC, GReal inValue);

	static	VTextStyle*			CreateTextStyle(const VFont *inFont, const GReal inDPI = 72.0f);

	/**	create text style from the specified font name, font size & font face
	@remarks
	if font name is empty, returned style inherits font name from parent
	if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
	if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
	*/
	static	VTextStyle*			CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI = 0);

protected:
	static	VectorOfSLONG		sTempVectorOfSLONG;
};


/** layout user delegate */
class XTOOLBOX_API IDocNodeLayoutUserDelegate
{
public:
	/** if it is not equal to kDOC_NODE_TYPE_UNKNOWN, handlers will be called only for layout class which matches the document node type */
	virtual	eDocNodeType		GetFilterDocNodeType() const { return kDOC_NODE_TYPE_UNKNOWN; }

	/** if it is not equal to NULL, handlers will be called only for layout class instance whom ID matches the returned ID */
	virtual	const VString*		GetFilterID() const { return NULL; }

	/** called before layout is painted
	@param	inLayout
	layout for which delegate is called
	@param	inUserData
	user data as passed initially at delegate initialization
	@param	inGC
	graphic context
	@param	inLayoutGCBounds
	layout bounds in gc user space
	*/
	virtual	void				OnBeforeDraw(VDocBaseLayout *inLayout, void *inUserData, VGraphicContext *inGC, const VRect& inLayoutGCBounds) = 0;

	/** called after layout is painted
	@param	inLayout
	layout for which delegate is called
	@param	inUserData
	user data as passed initially at delegate initialization
	@param	inGC
	graphic context
	@param	inLayoutGCBounds
	layout bounds in gc user space
	*/
	virtual	void				OnAfterDraw(VDocBaseLayout *inLayout, void *inUserData, VGraphicContext *inGC, const VRect& inLayoutGCBounds) = 0;

	/** called before a paragraph run is painted (only called for paragraph layout)
	@param	inLayout
	layout for which delegate is called
	@param	inUserData
	user data as passed initially at delegate initialization
	@param	inGC
	graphic context
	@param	inLayoutGCRunBounds
	run bounds in gc user space
	@param	inStart
	run range start
	@param	inEnd
	run range end
	@param	inIsRTL
	true if run direction is right to left, false otherwise
	*/
	virtual	void				OnBeforeDrawRun(VDocParagraphLayout *inParaLayout, void *inUserData, VGraphicContext *inGC, const VRect& inLayoutGCRunBounds, sLONG inStart = 0, sLONG inEnd = 0, bool inIsRTL = false) {}

	/** called after a paragraph run is painted (only called for paragraph layout)
	@param	inLayout
	layout for which delegate is called
	@param	inUserData
	user data as passed initially at delegate initialization
	@param	inGC
	graphic context
	@param	inLayoutGCRunBounds
	run bounds in gc user space
	@param	inStart
	run range start
	@param	inEnd
	run range end
	@param	inIsRTL
	true if run direction is right to left, false otherwise
	*/
	virtual	void				OnAfterDrawRun(VDocParagraphLayout *inParaLayout, void *inUserData, VGraphicContext *inGC, const VRect& inLayoutGCRunBounds, sLONG inStart = 0, sLONG inEnd = 0, bool inIsRTL = false) {}

	virtual void				OnBeforeDetachFromDocument(VDocBaseLayout *inLayout) {}

	virtual	bool				ShouldHandleLayout(IDocBaseLayout* inLayout)
	{
		if (GetFilterDocNodeType() != kDOC_NODE_TYPE_UNKNOWN && inLayout->GetDocType() != GetFilterDocNodeType())
			return false;
		if (GetFilterID() && !inLayout->GetID().EqualToString(*(GetFilterID()), true))
			return false;
		return true;
	}
};

/** VTextLayout: text layout class
@remarks
	this class is dedicated to manage platform-dependent text layout (using RichTextEdit on Windows & CoreText on Mac) 
	with support for paragraph text alignment, character styles & inline references like 4D expressions or urls: 
	it should be used to compute, render or edit text when v14 compatibility is strongly required (it is used in 4D by vars/field & static texts for instance)
	(VDocTextLayout is reserved for Write object only)

	this class manages a text layout & optionally pre-rendering of its content in a offscreen layer (if offscreen layer is enabled)

	for big text, it is recommended to use this class to draw text or get text metrics in order to optimize speed
				  because layout is preserved as long as layout properties or passed gc is compatible with the last used gc

	You should call at least SetText method to define the text & optionally styles; all other layout properties have default values;

	to draw to a VGraphicContext, use method 'Draw( gc, posTopLeft)'
	you can get computed layout metrics for a specified gc with 'GetLayoutBounds( gc, bounds, posTopLeft)' 
	you can get also computed run metrics for a specified gc with 'GetRunBoundsFromRange( gc, vectorOfBounds, posTopLeft, start, end)';
	there are also methods to get caret metrics from pos or get char index from pos.

	If layout cache is enabled, this class preserves text layout & metrics as long as the passed gc is compatible with the last used gc;
	optionally it can also pre-render the text layout in a offscreen layer compatible with the passed gc;
	on default only text layout is preserved: you should enable the offscreen layer only on a per-case basis.

	On Windows, it is safe to draw a VTextLayout to any graphic context but it is recommended whenever possible to draw to the same kind of graphic context 
	(ie a GDI, a GDIPlus, a hardware Direct2D gc or a software Direct2D gc) otherwise as layout or layer need to be computed again if gc kind has changed, 
	caching mecanism would not be useful if gc kind is changed at every frame... 

	This class is NOT thread-safe (as 4D GUI is single-threaded, it should not be a pb as this class should be used only by GUI component & GUI users)

	caution: note here that all passed/returned metrics are in pixel if unit is not explicitly specified :
	it is equal to point if dpi == 72 (default dpi in 4D forms)
*/
class XTOOLBOX_API VTextLayout: public VObject, public ITextLayout, public IRefCountable
{
public:
			/** VTextLayout constructor 
			
			@param inShouldEnableCache
				enable/disable cache (default is true): see ShouldEnableCache
			*/
								VTextLayout(bool inShouldEnableCache = true);
	virtual						~VTextLayout();

/////////////////	ISpanTextRefLayout interface

	virtual	void				RetainRef() { IRefCountable::Retain(); }
	virtual	void				ReleaseRef() { IRefCountable::Release(); }

/////////////////	ITextLayout interface

			bool				IsImplDocLayout() const { return false; } 

			/** set dirty
			@remarks
				use this method if text owner is caller and caller has modified text without using VTextLayout methods
				or if styles owner is caller and caller has modified styles without using VTextLayout methods
			*/
			void				SetDirty(bool inApplyToChildren = false, bool inApplyToParent = true) { fLayoutIsValid = false; }

			bool				IsDirty() const { return !fLayoutIsValid; }

			void				SetPlaceHolder( const VString& inPlaceHolder)	{ fPlaceHolder = inPlaceHolder; fLayoutIsValid = false; }
			void				GetPlaceHolder( VString& outPlaceHolder) const { outPlaceHolder = fPlaceHolder; }

			/** enable/disable password mode */
			void				EnablePassword(bool inEnable) { fIsPassword = inEnable; fLayoutIsValid = false; }
			bool				IsEnabledPassword() const { return fIsPassword; }

			/** enable/disable cache
			@remarks
				true (default): VTextLayout will cache impl text layout & optionally rendered text content on multiple frames
								in that case, impl layout & text content are computed/rendered again only if gc passed to methods is no longer compatible with the last used gc 
								or if text layout is dirty;

				false:			disable layout cache
				
				if VGraphicContext::fPrinterScale == true - ie while printing, cache is automatically disabled so you can enable cache even if VTextLayout instance is used for both screen drawing & printing
				(but it is not recommended: you should use different layout instances for painting onscreen or printing)
			*/
			void				ShouldEnableCache( bool inShouldEnableCache);
			bool				ShouldEnableCache() const { return fShouldEnableCache; }


			/** allow/disallow offscreen text rendering (default is false)
			@remarks
				text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
				you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
			@see
				ShouldEnableCache
			*/
			void				ShouldDrawOnLayer( bool inShouldDrawOnLayer);
			bool				ShouldDrawOnLayer() const { return fShouldDrawOnLayer; }
			void				SetLayerDirty(bool inApplyToChildren = false) { fLayerIsDirty = true; }

			/** paint selection */
			void				ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor = NULL, const VColor* inInactiveColor = NULL);
			bool				ShouldPaintSelection() const { return fShouldPaintSelection; }
			/** change selection (selection is used only for painting) */
			void				ChangeSelection( sLONG inStart, sLONG inEnd = -1, bool inActive = true);
			bool				IsSelectionActive() const { return fSelIsActive; }

			/**	VTextLayout constructor
			@param inPara
				VDocParagraph source: the passed paragraph is used only for initializing layout plain text, styles & paragraph properties
									  but inPara is not retained 
				(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
			@param inShouldEnableCache
				enable/disable cache (default is true): see ShouldEnableCache
			*/
								VTextLayout(const VDocParagraph *inPara, bool inShouldEnableCache = true);

			/** create and retain document from the current layout  
			@param inStart
				range start
			@param inEnd
				range end

			@remarks
				if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
				with a single child node initialized from the current layout

				NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			VDocText*			CreateDocument(sLONG inStart = 0, sLONG inEnd = -1) const;

			/** create and retain paragraph from the current layout plain text, styles & paragraph properties 
			@param inStart
				range start
			@param inEnd
				range end

				NOT IMPLEMENTED: reserved for VDocTextLayout
				*/
			VDocParagraph*		CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1) const;

			/** apply class style to the current layout and/or children depending on inTargetDocType
			@param inClassStyle (mandatory only for VDocTextLayout impl)
				the class style - if inClassStyle class name is not empty, apply only to current or children nodes with same class name
			@param inResetStyles
				if true, reset styles to default prior to apply new styles
			@param inReplaceClass (mandatory only for VDocTextLayout impl)
				if inClassStyle class is not empty:
					if true, apply class style to current layout and/or children which match inTargetDocType and change layout class to inClassStyle class
					if false, apply class style to current layout and/or children which match both inTargetDocType and inClassStyle class
				if inClassStyle class is empty, ignore inReplaceClass and apply inClassStyle to current layout and/or children which match inTargetDocType - and do not modify layout class
			@param inStart (mandatory only if current layout is a container)
				text range start 
			@param inEnd (mandatory only if current layout is a container)
				text range end 
			@param inTargetDocType (mandatory only for VDocTextLayout impl)
				apply to current layout and/or children if and only if layout document type is equal to inTargetDocType
				(it works also with embedded images or tables)

				NOT IMPLEMENTED: reserved for VDocTextLayout
				*/
			bool				ApplyClassStyle( const VDocClassStyle *inClassStyle, bool inResetStyles = false, bool inReplaceClass = false, sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH);

			/** accessor on class style manager 

				NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			VDocClassStyleManager*	GetClassStyleManager() const { return NULL; }

			/** init layout from the passed paragraph 
			@param inPara
				paragraph source
			@remarks
				the passed paragraph is used only for initializing layout plain text, styles & paragraph properties but inPara is not retained 
				note here that current styles are reset (see ResetParagraphStyle) & replaced by paragraph styles
				also current plain text is replaced by the passed paragraph plain text 

				NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				InitFromParagraph( const VDocParagraph *inPara);

			/** reset all text properties & character styles to default values */
			void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1);

			bool				MergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1) { return false; /** not implemented (only implemented by VDocTextLayout) */ }

			bool				UnMergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1) { return false; /** not implemented (only implemented by VDocTextLayout) */}

			bool				HasSpanRefs() const { return fStyles && fStyles->HasSpanRefs(); }

			/** convert uniform character index (the index used by 4D commands) to actual plain text position 
			@remarks
				in order 4D commands to use uniform character position while indexing text containing span references 
				(span ref content might vary depending on computed value or showing source or value)
				we need to map actual plain text position with uniform char position:
				uniform character position assumes a span ref has a text length of 1 character only
			*/
			void				CharPosFromUniform( sLONG& ioPos) const { if (fStyles) fStyles->CharPosFromUniform( ioPos); }

			/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
			@remarks
				in order 4D commands to use uniform character position while indexing text containing span references 
				(span ref content might vary depending on computed value or showing source or value)
				we need to map actual plain text position with uniform char position:
				uniform character position assumes a span ref has a text length of 1 character only
			*/
			void				CharPosToUniform( sLONG& ioPos) const { if (fStyles) fStyles->CharPosToUniform( ioPos); }

			/** convert uniform character range (as used by 4D commands) to actual plain text position 
			@remarks
				in order 4D commands to use uniform character position while indexing text containing span references 
				(span ref content might vary depending on computed value or showing source or value)
				we need to map actual plain text position with uniform char position:
				uniform character position assumes a span ref has a text length of 1 character only
			*/
			void				RangeFromUniform( sLONG& inStart, sLONG& inEnd) const { if (fStyles) fStyles->RangeFromUniform( inStart, inEnd); }

			/** convert actual plain text range to uniform range (as used by 4D commands) 
			@remarks
				in order 4D commands to use uniform character position while indexing text containing span references 
				(span ref content might vary depending on computed value or showing source or value)
				we need to map actual plain text position with uniform char position:
				uniform character position assumes a span ref has a text length of 1 character only
			*/
			void				RangeToUniform( sLONG& inStart, sLONG& inEnd) const { if (fStyles) fStyles->RangeToUniform( inStart, inEnd); }

			/** adjust selection 
			@remarks
				you should call this method to adjust with surrogate pairs or span references
			*/
			void				AdjustRange(sLONG& ioStart, sLONG& ioEnd) const;

			/** adjust char pos
			@remarks
				you should call this method to adjust with surrogate pairs or span references
			*/
			void				AdjustPos(sLONG& ioPos, bool inToEnd = true) const;

			/** replace text with span text reference on the specified range
			@remarks
				it will insert plain text as returned by VSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

				you should no more use or destroy passed inSpanRef even if method returns false

				IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
						   so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
						   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
						    - if display type is set to normalized source)
			*/
			bool				ReplaceAndOwnSpanRef(	ISpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = false, 
														void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

			/** return span reference at the specified text position if any */
			const VTextStyle*	GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const
			{
				VTaskLock protect(&fMutex);
				if (outBaseStart)
					*outBaseStart = 0;
				return fStyles ? fStyles->GetStyleRefAtPos( inPos) : NULL;
			}

			/** return the first span reference which intersects the passed range */
			const VTextStyle*	GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const
			{
				VTaskLock protect(&fMutex);
				if (outBaseStart)
					*outBaseStart = 0;
				return fStyles ? fStyles->GetStyleRefAtRange( inStart, inEnd) : NULL;
			}

			/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
			bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

			/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
			bool				FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

			/** eval 4D expressions  
			@remarks
				note that it only updates span 4D expression references but does not update plain text:
				call UpdateSpanRefs after to update layout plain text

				return true if any 4D expression has been evaluated
			*/
			bool				EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL);

			/** set layer view rectangle (user space is VView window user space - hwnd user space)
			@remarks
				useful only if offscreen layer is enabled

				by default, layer surface size is equal to text layout size in pixels
				but it can be useful to constraint layer surface size to not more than the visible area size of the text layout container
				in order to avoid to draw offscreen all the text layout (especially if the text size is great...)

				for instance if VTextLayout container is a VView-derived class & fTextLayout is the text layout attached or used in the VView-derived class,
				in the VView-derived class DoDraw method & prior to call VTextLayout::Draw, you should insert this code to set the layer view rect:

				[code/]

				VRect boundsHwnd;
				GetHwndBounds(boundsHwnd);
				fTextLayout->SetLayerViewRect( boundsHwnd);

				[/code]
				
				with that code, layer size will be not greater than min of layout size & VView visible area
				(see class VStyledTextEdtiView for a sample)

				You can still reset to VRect() to use layout size as layer size if you intend to pre-render all text layout 
				(only if you have called before SetLayerViewRect with a not empty VRect)
			*/
			void				SetLayerViewRect( const VRect& inRectRelativeToWnd) 
			{
				VTaskLock protect(&fMutex);
				if (fLayerViewRect == inRectRelativeToWnd)
					return;
				fLayerViewRect = inRectRelativeToWnd;
				fLayerIsDirty = true;
			}
			const VRect&		GetLayerViewRect() const { return fLayerViewRect; }

			/** insert text at the specified position */
			void				InsertPlainText( sLONG inPos, const VString& inText);

			/** delete text range */
			void				DeletePlainText( sLONG inStart, sLONG inEnd);

			/** replace text */
			void				ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText);

			/** apply style (use style range) */
			bool				ApplyStyle( const VTextStyle* inStyle, bool inFast = false, bool inIgnoreIfEmpty = true);

			bool				ApplyStyles( const VTreeTextStyle * inStyles, bool inFast = false);

			/** show span references source or value */ 
			void				ShowRefs( SpanRefCombinedTextTypeMask inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL)
			{
				if (fShowRefs == inShowRefs)
					return;
				fShowRefs = inShowRefs;
				if (fStyles)
				{
					fStyles->ShowSpanRefs( inShowRefs);
					UpdateSpanRefs(0, -1, inUserData, inBeforeReplace, inAfterReplace, inLC); 
				}
			}

			SpanRefCombinedTextTypeMask	ShowRefs() const { return fShowRefs; }

			/** highlight span references in plain text or not */
			void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false)
			{
				if (!inUpdateAlways && fHighlightRefs == inHighlight)
					return;
				fHighlightRefs = inHighlight;
				if (fStyles)
				{
					fStyles->ShowHighlight( fHighlightRefs);
					fLayoutIsValid = false;
				}
			}

			/** call this method to update span reference style while mouse enters span ref 
			@remarks
				return true if style has changed
			*/
			bool				NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0)
			{
				if (!fStyles)
					return false;
				if (fStyles->NotifyMouseEnterStyleSpanRef( inStyleSpanRef))
				{
					fLayoutIsValid = false;
					return true;
				}
				return false;
			}

			/** call this method to update span reference style while mouse leaves span ref 
			@remarks
				return true if style has changed
			*/
			bool				NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0)
			{
				if (!fStyles)
					return false;
				if (fStyles->NotifyMouseLeaveStyleSpanRef( inStyleSpanRef))
				{
					fLayoutIsValid = false;
					return true;
				}
				return false;
			}


			bool				HighlightRefs() const { return fHighlightRefs; }

			/** set default font 
			@remarks
				it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)

				by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
			*/
			void				SetDefaultFont( const VFont *inFont, GReal inDPI = 72.0f, sLONG inStart = 0, sLONG inEnd = -1);

			/** get & retain default font 
			@remarks
				by default, return 4D form-compliant font (with dpi = 72) so not the actual fDPI-scaled font 
				(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
			*/
			VFont*				RetainDefaultFont(GReal inDPI = 72.0f, sLONG inStart = 0) const;

			/** set default text color 
			@remarks
				black color on default (it is applied prior styles set with SetText)
			*/
			void				SetDefaultTextColor( const VColor &inTextColor, sLONG inStart = 0, sLONG inEnd = -1);

			/** get default text color */
			bool				GetDefaultTextColor(VColor& outColor, sLONG inStart = 0) const;
	
			/** set back color 
			@param  inColor
				layout back color
			@param	inPaintBackColor
				if false (default), layout frame is not filled with back color (for instance if parent frame paints back color yet)
					but it is still used to ignore character background color if character back color is equal to the frame back color
					(unless back color is transparent color)
			@remarks
				default is white and back color is not painted
			*/
			void				SetBackColor( const VColor& inColor, bool inPaintBackColor = false);
			const VColor&		GetBackColor() const { return fBackColor; }
			
			/** replace current text & styles 
			@remarks
				if inCopyStyles == true, styles are copied
				if inCopyStyles == false (default), styles MIGHT be retained: in that case, if you modify passed styles you should call this method again 

				this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
			*/
			void				SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

			/** replace current text & styles
			@remarks	
				here passed styles are always copied

				this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
			*/
			void				SetText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd = -1, 
										 bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
										 const ISpanTextRef * inPreserveSpanTextRef = NULL, 
										 void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

			const VString&		GetText() const;

			void				GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const;

			/** return unicode character at passed position (0-based) - 0 if out of range */
			UniChar				GetUniChar( sLONG inIndex) const { if (inIndex >= 0 && inIndex < fTextLength) return fText[inIndex]; else return 0; }

			/*	get number of lines */
			sLONG				GetNumberOfLines(VGraphicContext *inGC);

			/** return text length */
			uLONG				GetTextLength() const { return fTextLength; }

			/** return styles on the passed range 
			@remarks
				if inIncludeDefaultStyle == false (default is true), it does not include paragraph uniform style (default style) but only styles which are not uniform on paragraph text:
				otherwise it returns all styles applied to the plain text

				caution: if inCallerOwnItAlways == false (default is true), returned styles might be shared with layout instance so you should not modify it
			*/
			VTreeTextStyle*		RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inIncludeDefaultStyle = true, bool inCallerOwnItAlways = true) const;

			/** set extra text styles 
			@remarks
				it can be used to add a temporary style effect to the text layout without modifying the main text styles
				(for instance to add a text selection effect: see VStyledTextEditView)

				if inCopyStyles == true, styles are copied
				if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
			*/
			void				SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

			VTreeTextStyle*		RetainExtStyles() const
			{
				VTaskLock protect(&fMutex); 
				return fExtStyles ? new VTreeTextStyle(fExtStyles) : NULL;
			}

			/** replace margins	(in point) */
			void				SetMargins(const GReal inMarginLeft = 0.0f, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f);	
	
			/** get margins (in point) */
			void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom) const;	

            /** get margins (in TWIPS) */
            void                GetMargins( sRectCSS& outMargins) const;

             /** set margins (in TWIPS) */
            void                SetMargins( const sRectCSS& inMargins, bool inCalcAllMargin = true);

			/** set inside margins	(in twips) - CSS padding */
			void				SetPadding(const sRectCSS& inPadding, bool inCalcAllMargin = true) { /** not implemented (only implemented by VDocTextLayout) */ } 
			/** get inside margins	(in twips) - CSS padding */
			void				GetPadding(sRectCSS& outPadding) const { outPadding.left = outPadding.top = outPadding.right = outPadding.bottom = 0; }

			/** set max text width (0 for infinite) - in pixel (pixel relative to the layout DPI)
			@remarks
				it is also equal to text box width 
				so you should specify one if text is not aligned to the left border
				otherwise text layout box width will be always equal to formatted text width
			*/ 
			void				SetMaxWidth(GReal inMaxWidth = 0.0f);
			GReal				GetMaxWidth() const { return fMaxWidthInit; }

			/** set max text height (0 for infinite) - in pixel
			@remarks
				it is also equal to text box height 
				so you should specify one if paragraph is not aligned to the top border
				otherwise text layout box height will be always equal to formatted text height
			*/ 
			void				SetMaxHeight(GReal inMaxHeight = 0.0f); 
			GReal				GetMaxHeight() const { return fMaxHeightInit; }

			/** set width (in twips) - CSS width (0 for auto) */
			void				SetWidth(const uLONG inWidth)
			{
				if (inWidth == 0)
					SetMaxWidth(0);
				else
				{
					sRectCSS margins;
					GetMargins(margins);
					SetMaxWidth((inWidth+margins.left+margins.right)*GetDPI()/72);
				}
			}

			uLONG				GetWidth(bool inIncludeAllMargins = false) const 
			{ 
				if (!GetMaxWidth())
					return 0;

				if (!inIncludeAllMargins)
				{
					sRectCSS margins;
					GetMargins(margins);
					GReal width = -margins.left-margins.right+ICSSUtil::PointToTWIPS(GetMaxWidth()*72/GetDPI());
					return (width > 0 ? width : 1);
				}
				else
					return ICSSUtil::PointToTWIPS(GetMaxWidth()*72/GetDPI());
			}

			/** set height (in twips) - CSS height (0 for auto) */
			void				SetHeight(const uLONG inHeight)
			{
				if (inHeight == 0)
					SetMaxHeight(0);
				else
				{
					sRectCSS margins;
					GetMargins(margins);
					SetMaxHeight((inHeight+margins.top+margins.bottom)*GetDPI()/72);
				}
			}

			uLONG				GetHeight(bool inIncludeAllMargins = false) const
			{
				if (!GetMaxHeight())
					return 0;

				if (!inIncludeAllMargins)
				{
					sRectCSS margins;
					GetMargins(margins);
					GReal height = -margins.top-margins.bottom+ICSSUtil::PointToTWIPS(GetMaxHeight()*72/GetDPI());
					return (height > 0 ? height : 1);
				}
				else
					return ICSSUtil::PointToTWIPS(GetMaxHeight()*72/GetDPI());
			}

			/** set min width (in twips) - CSS min-width (0 for auto) */
			void				SetMinWidth(const uLONG inWidth) {}
			uLONG				GetMinWidth(bool inIncludeAllMargins = false) const
			{ 
				if (inIncludeAllMargins)
				{
					sRectCSS margins;
					GetMargins(margins);
					return margins.left+margins.right; 
				}
				else
					return 0;
			}

			/** set min height (in twips) - CSS min-height (0 for auto) */
			void				SetMinHeight(const uLONG inHeight) {} 
			uLONG				GetMinHeight(bool inIncludeAllMargins = false) const
			{ 
				if (inIncludeAllMargins)
				{
					sRectCSS margins;
					GetMargins(margins);
					return margins.top+margins.bottom; 
				}
				else
					return 0;
			}


			/** set text horizontal alignment */
			void				SetTextAlign( const AlignStyle inHAlign);
			/** get text horizontal aligment */
			AlignStyle			GetTextAlign() const { return fHAlign; }

			/** set vertical align - CSS vertical-align (AL_DEFAULT for default) */
			void				SetVerticalAlign( const AlignStyle inVAlign);
			virtual AlignStyle	GetVerticalAlign() const { return fVAlign; }
			
			/** get paragraph line height 

				if positive, it is fixed line height in point
				if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
				if -1, it is normal line height 
			*/
			GReal				GetLineHeight(sLONG inStart = 0, sLONG inEnd = -1) const { return fLineHeight; }

			/** set paragraph line height 

				if positive, it is fixed line height in point
				if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
				if -1, it is normal line height 

			note: not implemented on Mac (fully implemented only by VDocTextLayout) 
			*/
			void				SetLineHeight( const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1);

			bool				IsRTL(sLONG inStart = 0, sLONG inEnd = -1) const { return (GetLayoutMode() & TLM_RIGHT_TO_LEFT) != 0; }
			void				SetRTL(bool inRTL, sLONG inStart = 0, sLONG inEnd = -1)
			{
				SetLayoutMode( inRTL ? (GetLayoutMode() | TLM_RIGHT_TO_LEFT) : (GetLayoutMode() & (~TLM_RIGHT_TO_LEFT)));
			}

			/** get first line padding offset in point
			@remarks
				might be negative for negative padding (that is second line up to the last line is padded but the first line)
			*/
			GReal				GetTextIndent(sLONG inStart = 0, sLONG inEnd = -1) const { return fTextIndent; }

			/** set first line padding offset in point 
			@remarks
				might be negative for negative padding (that is second line up to the last line is padded but the first line)

			note: not implemented on Mac (fully implemented only by VDocTextLayout) 
			*/
			void				SetTextIndent(const GReal inPadding, sLONG inStart = 0, sLONG inEnd = -1);

			/** get tab stop offset in point */
			GReal				GetTabStopOffset(sLONG inStart = 0, sLONG inEnd = -1) const { return fTabStopOffset; }

			/** set tab stop (offset in point) 
			
			note: not implemented on Mac (fully implemented only by VDocTextLayout) 
			*/
			void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart = 0, sLONG inEnd = -1);

			/** get tab stop type */ 
			eTextTabStopType	GetTabStopType(sLONG inStart = 0, sLONG inEnd = -1) const { return fTabStopType; }

			/** set text layout mode (default is TLM_NORMAL) */
			void				SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL, sLONG inStart = 0, sLONG inEnd = -1);
			/** get text layout mode */
			TextLayoutMode		GetLayoutMode(sLONG inStart = 0, sLONG inEnd = -1) const { return fLayoutMode; }

			/** set class for new paragraph on RETURN */
			void				SetNewParaClass(const VString& inClass, sLONG inStart = 0, sLONG inEnd = -1) { /** not implemented (only implemented by VDocTextLayout) */ }

			/** get class for new paragraph on RETURN */
			const VString&		GetNewParaClass(sLONG inStart = 0, sLONG inEnd = -1) const;


			/** set layout DPI (default is 4D form DPI = 72) */
			void				SetDPI( const GReal inDPI = 72.0f);

			/** get layout DPI */
			GReal				GetDPI() const { return fDPI; }

			/** set background color - CSS background-color (transparent for not defined) */
			void				SetBackgroundColor( const VColor& inColor) { SetBackColor( inColor, fShouldPaintBackColor); }
			const VColor&		GetBackgroundColor() const { return GetBackColor(); }

			/** set background clipping - CSS background-clip 

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				SetBackgroundClip( const eBackgroundBox inBackgroundClip) { /** not implemented (only implemented by VDocTextLayout) */ } 
			eBackgroundBox		GetBackgroundClip() const { return kBACKGROUND_BORDER_BOX; }

			/** set background origin - CSS background-origin 

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				SetBackgroundOrigin( const eBackgroundBox inBackgroundOrigin) { /** not implemented (only implemented by VDocTextLayout) */ } 
			eBackgroundBox		GetBackgroundOrigin() const { return kBACKGROUND_PADDING_BOX; }

			/** set borders 
			@remarks
				if inLeft is NULL, left border is same as right border 
				if inBottom is NULL, bottom border is same as top border
				if inRight is NULL, right border is same as top border
				if inTop is NULL, top border is default border (solid, medium width & black color)

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				SetBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL) { /** not implemented (only implemented by VDocTextLayout) */ } 

			/** clear borders 

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				ClearBorders() { /** not implemented (only implemented by VDocTextLayout) */ } 
							
			/** set background image 
			@remarks
				see VDocBackgroundImageLayout constructor for parameters description 

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				SetBackgroundImage(			const VString& inURL,	
															const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
															const eBackgroundRepeat inRepeat = kBACKGROUND_REPEAT,
															const GReal inWidth = kBACKGROUND_SIZE_AUTO, const GReal inHeight = kBACKGROUND_SIZE_AUTO,
															const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false) { /** not implemented (only implemented by VDocTextLayout) */ } 

			/** clear background image 

			NOT IMPLEMENTED: reserved for VDocTextLayout
			*/
			void				ClearBackgroundImage() { /** not implemented (only implemented by VDocTextLayout) */ } 
			
			/** begin using text layout for the specified gc (reentrant)
			@remarks
				for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
				otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
				But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

				CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
						 otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

				If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
			*/
			void				BeginUsingContext( VGraphicContext *inGC, bool inNoDraw = false);

			/** update text layout metrics */ 
			void				UpdateLayout(VGraphicContext *inGC);

			/** draw text layout in the specified graphic context 
			@remarks
				text layout left top origin is set to inTopLeft
				text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
				text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
				
				method does not save & restore gc context
			*/
			void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);
	
			/** return formatted text bounds (typo bounds) including margins
			@remarks
				text typo bounds origin is set to inTopLeft

				as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
				because layout bounds are equal to the text box design bounds and not to the formatted text bounds
				you should use this method to get formatted text width & height 
			*/
			void				GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

			/** return text layout bounds including margins (in gc user space)
			@remarks
				text layout origin is set to inTopLeft
				on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

				on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
				(if max width or max height is equal to 0.0f, it is assumed to be infinite & so layout bounds will be equal to formatted text bounds)
				output layout width will be equal to max width if maw width != 0 otherwise to formatted text width
				output layout height will be equal to max height if maw height != 0 otherwise to formatted text height
				(see CAUTION remark below)

				if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
				(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
				 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

				CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
						 because layout bounds are equal to the text box design bounds and not to the formatted text bounds: 
						 you should use GetTypoBounds in order to get formatted text width & height
			*/
			void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

			/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
			const VRect&		GetLayoutBounds() const;

			/** get layout typo bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
			const VRect&		GetTypoBounds() const;

			/** get background bounds */
			const VRect&		GetBackgroundBounds(eBackgroundBox inBackgroundBox = kBACKGROUND_BORDER_BOX) const { return GetLayoutBounds(); }

			/** return text layout run bounds from the specified range (in gc user space)
			@remarks
				text layout origin is set to inTopLeft
			*/
			void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);

			/** return the caret position & height of text at the specified text position in the text layout (in gc user space)
			@remarks
				text layout origin is set to inTopLeft

				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
				if text layout is drawed at inTopLeft

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
			void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

	
			/** return the text position at the specified coordinates
			@remarks
				text layout origin is set to inTopLeft (in gc user space)
				the hit coordinates are defined by inPos param (in gc user space)

				output text position is 0-based

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
			bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL);

			/** get character range which intersects the passed bounds (in gc user space) 
			@remarks
				text layout origin is set to inTopLeft (in gc user space)

				char range is rounded to line run limit so the returned range might be slightly greater than the actual char range that intersects the passed bounds
			*/
			void				GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd);

			/** move character index to the nearest character index on the line before/above the character line 
			@remarks
				return false if character is on the first line (ioCharIndex is not modified)
			*/
			bool				MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);

			/** move character index to the nearest character index on the line after/below the character line
			@remarks
				return false if character is on the last line (ioCharIndex is not modified)
			*/
			bool				MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);


			/** return true if layout uses truetype font only */
			bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const;

			/** end using text layout (reentrant) */
			void				EndUsingContext();

			VTreeTextStyle*		RetainDefaultStyle(sLONG inStart = 0, sLONG inEnd = -1) const;

			/** consolidate end of lines
			@remarks
				if text is multiline:
					we convert end of lines to a single CR 
				if text is singleline:
					if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
					if inEscaping, we escape line endings and tab 
			*/
			bool				ConsolidateEndOfLines(bool inIsMultiLine = true, bool inEscaping = false, sLONG inStart = 0, sLONG inEnd = -1);

			/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) range at the specified character index */
			void				GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces = true, bool inTrimPunctuation = false)
			{
				ITextLayout::GetWordRange( inIntlMgr, inOffset, outStart, outEnd, inTrimSpaces, inTrimPunctuation);
			}
			
			/**	get paragraph range at the specified character index 
			@remarks
				if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
				(for VTextLayout, parameter is ignored)
			*/
			void				GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange = false)
			{
				ITextLayout::GetParagraphRange( inIntlMgr, inOffset, outStart, outEnd, inUseDocParagraphRange);
			}

protected:
	friend	class				VDocBaseLayout;
	friend	class				VDocParagraphLayout;

	friend	class				VGraphicContext;
#if ENABLE_D2D
	friend	class				VWinD2DGraphicContext;
#endif
#if VERSIONMAC
	friend	class				VMacQuartzGraphicContext;
#endif

	typedef struct sRectMargin
			{
								GReal top;
								GReal right;
								GReal bottom;
								GReal left;
			} sRectMargin;

	typedef	std::vector<VRect>					VectorOfRect;
	typedef std::pair<VectorOfRect, VColor>		BoundsAndColor;
	typedef	std::vector<BoundsAndColor>			VectorOfBoundsAndColor;

			void				_Init();

			GReal				_GetLayoutWidthMinusMargin() const
			{
				return fCurLayoutWidth-fMargin.left-fMargin.right;
			}
			GReal				_GetLayoutHeightMinusMargin() const 
			{
				return fCurLayoutHeight-fMargin.top-fMargin.bottom;
			}

			bool				_ApplyStyle(const VTextStyle *inStyle, bool inIgnoreIfEmpty = true);

			bool				_ApplyStylesRec( const VTreeTextStyle * inStyles, bool inFast = false);

			/** retain default uniform style 
			@remarks
				if fStyles == NULL, theses styles are text content uniform styles (monostyle)
			*/
			VTreeTextStyle*		_RetainDefaultTreeTextStyle() const;

			/** retain full styles
			@remarks
				use this method to get the full styles applied to the text content
				(but not including the graphic context default font: this font is already applied internally by gc methods before applying styles)

				caution: please do not modify fStyles between _RetainFullTreeTextStyle & _ReleaseFullTreeTextStyle

				(in monostyle, it is generally equal to _RetainDefaultTreeTextStyle)
			*/
			VTreeTextStyle*		_RetainFullTreeTextStyle() const;

			void				_ReleaseFullTreeTextStyle( VTreeTextStyle *inStyles) const;

			void				_CheckStylesConsistency();

			bool				_CheckLayerBounds(const VPoint& inTopLeft, VRect& outLayerBounds);

			bool				_BeginDrawLayer(const VPoint& inTopLeft = VPoint());
			void				_EndDrawLayer();

			/** set text layout graphic context 
			@remarks
				it is the graphic context to which is bound the text layout
				if offscreen layer is disabled text layout needs to be redrawed again at every frame
				so it is recommended to enable the internal offscreen layer if you intend to preserve rendered layout on multiple frames
				because offscreen layer & text content are preserved as long as offscreen layer is compatible with the attached gc & text content is not dirty
			*/
			void				_SetGC( VGraphicContext *inGC);
			
			/** return the graphic context bound to the text layout 
			@remarks
				gc is not retained
			*/
			VGraphicContext*	_GetGC() { return fGC; }

			/** update text layout if it is not valid */
			void				_UpdateTextLayout();

			/** internally called while replacing text in order to update impl-specific stuff */
			void				_PostReplaceText();

			void				_BeginUsingStyles();
			void				_EndUsingStyles();

			void				_ResetLayer();

			/** return true if layout styles uses truetype font only */
			bool				_StylesUseFontTrueTypeOnly() const;

			void				_UpdateBackgroundColorRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds);
			void				_UpdateBackgroundColorRenderInfoRec( const VPoint& inTopLeft, VTreeTextStyle *inStyles, const sLONG inStart, const sLONG inEnd, bool inHasParentBackgroundColor = false);
			void				_DrawBackgroundColor(const VPoint& inTopLeft, const VRect& inBoundsLayout);

			void				_UpdateSelectionRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds);
			void				_DrawSelection(const VPoint& inTopLeft);

			void				_ResetStyles( bool inResetParagraphProps = true, bool inResetCharStyles = true);

			bool				fLayoutIsValid;
			
			bool				fShouldEnableCache;
			bool				fShouldDrawOnLayer;
			VImageOffScreen*	fLayerOffScreen;
			VRect				fLayerViewRect;
			VPoint				fLayerOffsetViewRect;
			VAffineTransform	fLayerCTM;
			bool				fLayerIsDirty;


			GraphicContextType	fGCType;
			VGraphicContext*	fGC;
			sLONG				fGCUseCount;

	mutable VFont*				fDefaultFont;
			VColor				fDefaultTextColor;
			bool				fHasDefaultTextColor;
	mutable VTreeTextStyle*		fDefaultStyle;

			VFont*				fCurFont;
			VColor				fCurTextColor;
			GReal				fLayoutKerning;
			TextRenderingMode	fLayoutTextRenderingMode;

			bool				fIsPassword;
			VString				fTextPassword;
			VString				fPlaceHolder;

			/** margin in TWIPS */
			sRectCSS		fMarginTWIPS;
	
			/** margin in px */
			sRectMargin			fMargin;

			/** width of formatted text (including margins) in px */
			GReal				fCurWidth; 
			/** height of formatted text (including margins) in px */
			GReal				fCurHeight;

			/** width of text layout box (including margins) in px
			@remarks
				it is equal to fMaxWidth if fMaxWidth != 0.0f
				otherwise it is equal to fCurWidth
			*/
			GReal				fCurLayoutWidth;

			/** height of text layout box (including margins) in px
			@remarks
				it is equal to fMaxHeight if fMaxHeight != 0.0f
				otherwise it is equal to fCurHeight
			*/
			GReal				fCurLayoutHeight;

			sRectMargin			fCurOverhang;

			sLONG				fTextLength;
			VString				fText;
			VTreeTextStyle*		fStyles;
			VTreeTextStyle*		fExtStyles;
			
			GReal				fMaxWidthInit;		//max width including margins
 			GReal				fMaxHeightInit;		//max height including margins
			GReal				fMaxWidth;			//max width excluding margins
			GReal				fMaxHeight;			//max height excluding margins

			AlignStyle			fHAlign;
			AlignStyle			fVAlign;
			GReal 				fLineHeight;
			GReal 				fTextIndent;
			GReal 				fTabStopOffset;
			eTextTabStopType	fTabStopType;

			TextLayoutMode		fLayoutMode;
			GReal				fDPI;
	
			SpanRefCombinedTextTypeMask fShowRefs;
			bool 				fHighlightRefs;

			bool 				fUseFontTrueTypeOnly;
			bool 				fStylesUseFontTrueTypeOnly;

			bool 				fLayoutBoundsDirty;

			/** text box layout used by Quartz2D or GDIPlus/GDI (Windows legacy impl) */
			VStyledTextBox*		fTextBox;
#if ENABLE_D2D
			/** DWrite text layout used by Direct2D impl only (in not legacy mode */
			IDWriteTextLayout*	fLayoutD2D;
#endif

			VFont*				fBackupFont;
			VColor				fBackupTextColor;
			TextRenderingMode   fBackupTRM;
			bool				fNeedRestoreTRM;

			bool				fSkipDrawLayer;

			bool				fIsPrinting;

			/** true if VTextLayout should draw character background color but native impl */
			bool				fShouldPaintCharBackColor;
			bool				fShouldPaintCharBackColorExtStyles;

			/** true if character background color render info needs to be computed again */
			bool				fCharBackgroundColorDirty;

			/** character background color render info 
			@remarks
				it is used to draw text background color if it is not supported natively by impl
				(as for CoreText & Direct2D impl)
			*/
			VectorOfBoundsAndColor	fCharBackgroundColorRenderInfo;

			VColor				fBackColor;
			/** should paint background color ? (it is layout back color here and not character back color) */
			bool				fShouldPaintBackColor;

			/** should paint custom selection ? */
			bool				fShouldPaintSelection;

			/** selection range (used only for painting) */
			sLONG				fSelStart, fSelEnd;
			bool				fSelIsActive;

			/** selection color */
			VColor				fSelActiveColor, fSelInactiveColor;

			bool				fSelIsDirty;
			VectorOfRect		fSelectionRenderInfo;

			VPoint				fLayerTopLeft;

	mutable VCriticalSection	fMutex;
};


END_TOOLBOX_NAMESPACE

#endif

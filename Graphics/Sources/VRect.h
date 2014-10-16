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
#ifndef __VRect__
#define __VRect__

#include "Graphics/Sources/VPoint.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VRect : public VObject, public IStreamable
{
 public:
			VRect ();
			VRect (GReal inX, GReal inY, GReal inWidth, GReal inHeight);
			VRect (const VRect& inOriginal);

#if !VERSION_LINUX
			VRect (const RectRef& inNativeRect);
#endif

	virtual	~VRect();

	// Operators
	bool	operator != (const VRect& inRect) const { return !(*this == inRect); };
	bool	operator == (const VRect& inRect) const;
	VRect&	operator += (const VRect& inRect) { Union(inRect); return *this; };
	VRect&	operator &= (const VRect& inRect) { Intersect(inRect); return *this; };
	VRect&	operator = (const VRect& inRect) { FromRect(inRect); return *this; };
	
	// Accessors
	void	SetPosBy (GReal inHoriz, GReal inVert);
	void	SetSizeBy (GReal inHoriz, GReal inVert);
	
	void	ScaleSizeBy (GReal inHorizScale, GReal inVertScale);
	void	ScalePosBy (GReal inHorizScale, GReal inVertScale);
	
	void	SetPosTo (const VPoint& inPos);
	void	SetPosTo (const GReal inX,const GReal inY);
	void	SetSizeTo (GReal inWidth, GReal inHeight);
	void	SetCoordsTo (GReal inX, GReal inY, GReal inWidth, GReal inHeight);
	void	SetCoordsBy (GReal inX, GReal inY, GReal inWidth, GReal inHeight);
	void	SetCoords (GReal inX, GReal inY, GReal inWidth, GReal inHeight) { SetCoordsTo(inX, inY, inWidth, inHeight); };
		
	void	GetPos (VPoint& outPos) const { outPos.SetPosTo(fX, fY); };
	void	GetSize (GReal& outWidth, GReal& outHeight) const { outWidth = fWidth; outHeight = fHeight; };
 	
	GReal	GetX () const { return fX; };
	GReal	GetY () const { return fY; };
	GReal	GetWidth () const { return fWidth; };
	GReal	GetHeight () const { return fHeight; };

	GReal	GetLeft () const { return fX; };
	GReal	GetRight () const { return (fX + fWidth); };
	GReal	GetTop () const { return fY; };
	GReal	GetBottom () const { return (fY + fHeight); };
	
	VPoint	GetTopLeft () const { return VPoint(GetLeft(), GetTop()); };
	VPoint	GetBotRight () const { return VPoint(GetRight(), GetBottom()); };
	VPoint	GetTopRight () const { return VPoint(GetRight(), GetTop()); };
	VPoint	GetBotLeft () const { return VPoint(GetLeft(), GetBottom()); };

	void	SetTopLeft (VPoint inPoint) { SetPosTo(inPoint); };
	void	SetBotRight (VPoint inPoint) { SetSizeTo(inPoint.GetX() - GetX(), inPoint.GetY() - GetY()); };
	
	void	SetLeft (GReal inLeft) { GReal delta = inLeft - GetLeft(); SetPosBy(delta, 0); SetSizeBy(-delta, 0); };
	void	SetRight (GReal inRight) { SetSizeTo(inRight - GetLeft(), -1); };
	void	SetTop (GReal inTop) { GReal delta = inTop - GetTop(); SetPosBy(0, delta); SetSizeBy(0, -delta); };
	void	SetBottom (GReal inBottom) { SetSizeTo(-1, inBottom - GetTop()); };

	void	SetX (GReal inX);
	void	SetY (GReal inY);
	
	void	SetWidth (GReal inWidth);
	void	SetHeight (GReal inHeight);
	
	// Utilities
	void	Union (const VRect& inRect);
	void	Intersect (const VRect& inRect);
	void	Inset (GReal inHoriz, GReal inVert);
	bool	HitTest (const VPoint& inPoint) const;
	bool	Overlaps (const VRect& inRect) const;
	bool	Contains (const VRect& inRect) const;
	bool	Contains (const VPoint& inPoint) const { return HitTest(inPoint); };
	
	void	SetEmpty () { fX = fY = fWidth = fHeight = 0; };
	bool	IsEmpty () const;

	// Copy operators
	void	FromRect (const VRect& inOriginal);

#if !VERSION_LINUX
	// Native operators
	void	FromRectRef (const RectRef& inNativeRect);
	void	ToRectRef (RectRef& outNativeRect) const;
	operator const RectRef () const { RectRef r; ToRectRef(r); return r; };
#endif	

	/** set rect to the smallest containing rect with integer coordinates 
	@param inStickOriginToNearestInteger
		false (default): origin is set to floor(origin) (use it in order to get ideal GDI/QD clipping or invaliding rectangle which contains GDI/QD drawing rectangle)
		true: origin is rounded to the nearest int (use it to get ideal GDI/QD drawing rectangle)
	*/
	void	NormalizeToInt(bool inStickOriginToNearestInteger = false);
	
	// Inherited from VObject
	virtual VError	LoadFromBag (const VValueBag& inBag);
	virtual VError	LoadFromBagRelative (const VValueBag& inBag, const VRect& inParent);
	virtual VError	SaveToBag (VValueBag& ioBag, VString& outKind) const;
	virtual VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;

#if VERSIONMAC
			VRect (const Rect& inQDRect) { MAC_FromQDRect(inQDRect); };
	void	MAC_FromQDRect (const Rect& inQDRect);
	Rect*	MAC_ToQDRect (Rect& outQDRect) const;
	void	MAC_FromCGRect (const CGRect& inCGRect);
	CGRect*	MAC_ToCGRect (CGRect& outCGRect) const;
#endif

protected:
	GReal	fX, fY;
	GReal	fWidth, fHeight;
};

END_TOOLBOX_NAMESPACE

#endif

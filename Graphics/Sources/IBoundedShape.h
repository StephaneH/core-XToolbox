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
#ifndef __IBoundedShape__
#define __IBoundedShape__

#include "Graphics/Sources/VRect.h"

BEGIN_TOOLBOX_NAMESPACE

// Generic abstract class for handling bound accessors
class XTOOLBOX_API IBoundedShape
{
public:
	// Generic class implementing sizing and positioning
	virtual void	SetPosBy (GReal inHoriz, GReal inVert) = 0;
	virtual void	SetSizeBy (GReal inHoriz, GReal inVert) = 0;
	
	virtual void	ScaleSizeBy (GReal inHorizScale, GReal inVertScale);
	virtual void	ScalePosBy (GReal inHorizScale, GReal inVertScale);
	
	void	SetPosTo (const VPoint& inPos);
	void	SetSizeTo (GReal inWidth, GReal inHeight);
	void	SetCoords (GReal inX, GReal inY, GReal inWidth, GReal inHeight);
	
	virtual Boolean	HitTest (const VPoint& inPoint) const = 0;
	virtual void	Inset (GReal inHoriz, GReal inVert) = 0;
	
	void	GetPos (VPoint& outPos) const { fBounds.GetPos(outPos); };
	void	GetSize (GReal& outWidth, GReal& outHeight) const { fBounds.GetSize(outWidth, outHeight); };
	const VRect&	GetBounds () const { return fBounds; };
 	
	GReal	GetX () const { return fBounds.GetX(); };
	GReal	GetY () const { return fBounds.GetY(); };
	GReal	GetWidth () const { return fBounds.GetWidth(); };
	GReal	GetHeight () const { return fBounds.GetHeight(); };

	void	SetLeft (GReal inLeft) { GReal delta = inLeft - GetLeft(); SetPosBy(delta, 0); SetSizeBy(-delta, 0); };
	void	SetRight (GReal inRight) { SetSizeBy(inRight - GetRight(), 0); };
	void	SetTop (GReal inTop) { GReal delta = inTop - GetTop(); SetPosBy(0, delta); SetSizeBy(0, -delta); };
	void	SetBottom (GReal inBottom) { SetSizeBy(0, inBottom - GetBottom()); };

	GReal	GetLeft () const { return fBounds.GetLeft(); };
	GReal	GetRight () const { return fBounds.GetRight(); };
	GReal	GetTop () const { return fBounds.GetTop(); };
	GReal	GetBottom () const { return fBounds.GetBottom(); };

#if USE_OBSOLETE_API && 0
	void	SetX (sLONG inX) { SetPosBy(inX - GetX(), 0); };
	void	SetY (sLONG inY) { SetPosBy(0, inY - GetY()); };
	void	SetWidth (sLONG inWidth) { SetSizeTo(inWidth, -1); };
	void	SetHeight (sLONG inHeight) { SetSizeTo(-1, inHeight); };
	void	SetPos (sLONG inX, sLONG inY) { SetPosBy(inX - GetX(), inY - GetY()); };
	void	SetSize (sLONG inWidth, sLONG inHeight) { SetSizeTo(inWidth, inHeight); };
	void	SetPosByOffset (sLONG inX,sLONG inY) { SetPosBy(inX, inY); };
//	void	GetPos (sLONG outX, sLONG outY) { outX = GetX(); outY = GetY(); };
	
	// Caution: this generic function return erroneous value
	Boolean	Overlaps (const VRect& inRect) const { return fBounds.Overlaps(inRect); };
#endif

protected:
	VRect	fBounds;			// Cached bounds for the shape
	//void	_ComputeBounds ();	// You should allways syncronize fBounds when modifying the shape
};


END_TOOLBOX_NAMESPACE

#endif

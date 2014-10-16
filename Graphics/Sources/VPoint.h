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
#ifndef __VPoint__
#define __VPoint__

BEGIN_TOOLBOX_NAMESPACE

// Class types
template <class T> class VPointOf;
typedef VPointOf<GReal> VPoint;


template <class T> class VPointOf : public VObject
{
public:
			VPointOf () { x = 0; y = 0; };
			VPointOf (T inX, T inY) { x = inX; y = inY; };
	virtual	~VPointOf () { };
	
	template <class V>
	explicit VPointOf (const VPointOf<V>& inOriginal) { x = inOriginal.x; y = inOriginal.y; };
	
	void	From (VPointOf<sLONG>& inOriginal) { x = static_cast<T>(inOriginal.x); y = static_cast<T>(inOriginal.y); };
	void	From (VPointOf<GReal>& inOriginal) { x = static_cast<T>(inOriginal.x); y = static_cast<T>(inOriginal.y); };
	
	// Native operators
#if VERSIONMAC
	explicit	VPointOf (const Point& inMacPoint) { x = inMacPoint.h; y = inMacPoint.v; };
	explicit	VPointOf (const CGPoint& inMacPoint) { x = inMacPoint.x; y = inMacPoint.y; };
	
	operator	Point () const { Point macPoint = { (short)y, (short)x }; return macPoint; };
	operator	CGPoint () const { CGPoint macPoint = { x, y }; return macPoint; };
#endif

#if VERSIONWIN
	explicit	VPointOf (const tagPOINT* inWinPoint) { x = static_cast<T>(inWinPoint->x); y = static_cast<T>(inWinPoint->y); };
	operator	tagPOINT () const { tagPOINT winPoint; winPoint.x = static_cast<int>(x); winPoint.y = static_cast<int>(y); return winPoint; };
#endif

	// Accessors
	virtual void SetPosTo (T inX, T inY) { x = inX; y = inY; };
	virtual void SetPosBy (T inX, T inY) { x += inX; y += inY; };
	
	void	SetX(T inX) { x = inX; };
	void	SetY(T inY) { y = inY; };
	T	GetX (void) const { return x; };
	T	GetY (void) const { return y; };
	
#if USE_OBSOLETE_API
	void	Set (T inX, T inY) { x = inX; y = inY; };
	void	SetPos (T inX, T inY) { SetPosTo(inX,inY); };
	void	SetPosByOffset(T inX,T inY) { SetPosBy(inX,inY); };
	void	GetPos (T& outX, T& outY) const { outX = x; outY = y; };
#endif

	// To know in which space quarter this point is (compared to 0,0)
	sLONG GetQuarter ()
	{
		if (x >= 0) { if (y >= 0) return 0; else return 3; }
		else { if (y >= 0) return 1; else return 2; }
		return -1;
	}
	
	// Return distance from this point to the line defined by p1 & p2.
	GReal	DistFromLine(VPointOf<T> p1, VPointOf<T> p2) const
	{
		// Special cases to prevent zero devide :)
		if (p2.x-p1.x == 0) return x-p1.x; // vert line
		if (p2.y-p1.y == 0) return y-p1.y; // hori line
		
		double	ratio = (p2.y - p1.y) / (p2.x - p1.x);
		double	offset = p1.y - ratio * p1.x;
		double	ax = (y - offset) / ratio;
		double	by = (ratio * x) + offset;
		
		double	d1 = x - ax;
		double	d2 = y - by;
		return (GReal) sqrt(d1 * d1 + d2 * d2);
	}
	
	// Return distance from this point to the segment defined by p1 & p2
	// return -1 if projected point from this to the segment is not
	// within p1 and p2.
	GReal	DistFromSegment(VPointOf<T> p1, VPointOf<T> p2) const
	{
		if (p2.x - p1.x == 0)
		{
			if (y > Max(p1.y,p2.y)) return -1;
			if (y < Min(p1.y,p2.y)) return -1;
			return x - p1.x; // vert seg
		}
		
		if (p2.y - p1.y == 0)
		{
			if (x > Max(p1.x,p2.x)) return -1;
			if (x < Min(p1.x,p2.x)) return -1;
			return y - p1.y; // hori seg
		}
		
		double	ratio = (p2.y - p1.y) / (p2.x - p1.x);
		double	offset = p1.y - ratio * p1.x;
		VPointOf<T>	a, b, mid;
		a.y = y;
		a.x = (a.y - offset) / ratio;
		b.x = x;
		b.y = (ratio * b.x) + offset;
		
		mid = (a + b) / 2;
		if (mid.x > Max(p1.x, p2.x)) return -1;
		if (mid.x < Min(p1.x, p2.x)) return -1;
		if (mid.y > Max(p1.y, p2.y)) return -1;
		if (mid.y < Min(p1.y, p2.y)) return -1;
		
		double	d1 = x - a.x;
		double	d2 = y - b.y;
		return (GReal) sqrt(d1 * d1 + d2 * d2);
	}
	
	GReal	GetLength () const { return (GReal) sqrt(x * x + y * y); };

	void	Normalize() {GReal l = GetLength();if (l!=0){x/=l;y/=l;}}

	void	NormalizeToInt( bool inStickOriginToNearestInteger = false)
	{
		// sc 04/04/2012, from VRect::NormalizeToInt()
		if (inStickOriginToNearestInteger)
		{
			x =  floor(x+0.5f);
			y = floor(y+0.5f);
		}
		else
		{
			x = floor(x);
			y = floor(y);
		}
	}

	// Operators
	VPointOf<T>	operator + (const VPointOf<T>& inValue) const { return VPointOf(x + inValue.x, y + inValue.y); }; 
	VPointOf<T>	operator + (T inValue) const { return VPointOf<T>(x+inValue,y+inValue); };
	VPointOf<T>	operator - (const VPointOf<T>& inValue) const { return VPointOf<T>(x - inValue.x, y - inValue.y); };
	VPointOf<T>	operator - (T inValue) const { return VPointOf<T>(x-inValue,y-inValue); };
	VPointOf<T>	operator * (const VPointOf<T>& inValue) const { return VPointOf<T>(x * inValue.x, y * inValue.y); };
	VPointOf<T>	operator * (T inValue) const { return VPointOf<T>(x*inValue,y*inValue); };
	VPointOf<T>	operator / (const VPointOf<T>& inValue) const { return VPointOf<T>(x / inValue.x, y / inValue.y); };
	VPointOf<T>	operator / (T inValue) const { return VPointOf<T>(x/inValue,y/inValue); };

	VPointOf<T>	&operator += (const VPointOf<T>& inValue) { x += inValue.x; y += inValue.y; return *this; };
	VPointOf<T>	&operator += (T inValue) { x += inValue; y += inValue; return *this; };
	VPointOf<T>	&operator -= (const VPointOf<T> &inValue) { x -= inValue.x; y -= inValue.y; return *this; };
	VPointOf<T>	&operator -= (T inValue) { x -= inValue; y -= inValue; return *this; };
	VPointOf<T>	&operator *= (const VPointOf<T> &inValue) { x *= inValue.x; y *= inValue.y; return *this; };
	VPointOf<T>	&operator *= (T inValue) { x *= inValue; y *= inValue; return *this; };
	VPointOf<T>	&operator /= (const VPointOf<T> &inValue) { x /= inValue.x; y /= inValue.y; return *this; };
	VPointOf<T>	&operator /= (T inValue) { x /= inValue; y /= inValue; return *this; };

	VPointOf<T>	operator - () const { return VPointOf<T>(-x, -y); };
	VPointOf<T> &operator = (const VPointOf<T> &inValue) { x = inValue.x; y = inValue.y; return *this; };

	bool operator == (const VPointOf<T>& inValue) const { return (x == inValue.x && y == inValue.y); };
	bool operator != (const VPointOf<T>& inValue) const { return (x != inValue.x || y != inValue.y); };

	T x;
	T y;
};

END_TOOLBOX_NAMESPACE

#endif
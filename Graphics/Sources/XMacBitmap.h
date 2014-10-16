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
#ifndef __XMacBitmap__
#define __XMacBitmap__

BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VRect;


class XTOOLBOX_API XMacBitmap : public VObject
{
public:
			XMacBitmap ();
			XMacBitmap (XMacBitmap& inOriginal);
	virtual	~XMacBitmap ();
	
	// Creation / transform support
	virtual Boolean	NewBitmap (sLONG inWidth, sLONG inHeight) = 0;
	virtual Boolean	FromFile (VFile& inFile) = 0;
	virtual Boolean	FromPortRef (PortRef inPort, VRect& inBounds);
	virtual XMacBitmap*	Copy () = 0;
	virtual PicHandle	ToPicHandle ();
	
	virtual void	ApplyMatrix (const VPtr inMatrix, Boolean inSelfApply = false);
	
	// Drawing support
	virtual void	DrawSemiTransparent (PortRef inPort, VRect& inRect, uBYTE inOpacity);
	virtual void	DrawOpaque (PortRef inPort, VRect& inRect);
	virtual void	EraseBitmap (VColor& inEraseColor);
	virtual void	InvertBitmap ();
	
	// Accessors
	Boolean	IsValid () const { return fIsValid; };
	sLONG	GetWidth () const { return fWidth; };
	sLONG	GetHeight () const { return fHeight; };
	sLONG	GetBytePerRow () const { return fBytePerRow; };
	sLONG	GetPixelPerRow () const { return fPixPerRow; };
	sLONG	GetDepth () { return fDepth; };
	void*	GetBitmapBaseAdress () { return (void*) fBitmapBits; };
	
	virtual VColor	GetPixel (sLONG inX, sLONG inY) = 0;
	virtual void	SetPixel (sLONG inX, sLONG inY, VColor& inColor) = 0;
	virtual void	SetPixel (sLONG inX, sLONG inY, uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha) = 0;
	
	virtual void*	GetPixelPtr (sLONG inX, sLONG inY);
	virtual void*	GetLinePtr (sLONG inY);
	
	GWorldPtr	RetainDrawingPort () { return fBitmapPort; };
	void	ReleaseDrawingPort () {};
	
	// Low level utilities
	virtual Boolean	_Allocate (sLONG inWidth, sLONG inHeight) = 0;
	virtual Boolean	_Free () = 0;
	virtual void	_Blit (XMacBitmap& inDest);
	
protected:
	PixMapHandle 	fBitmap;
	GWorldPtr		fBitmapPort;
	VPixelFormat	fPixelType;
	void* 	fBitmapBits;
	sLONG	fWidth;
	sLONG	fHeight;
	sLONG	fBytePerRow;
	sLONG	fPixPerRow;
	sLONG	fDepth;
	Boolean	fIsValid;

private:
	void	_Init ();
};


class XTOOLBOX_API XMacBitmap8GrayScale : public XMacBitmap
{
public:
			XMacBitmap8GrayScale ();
			XMacBitmap8GrayScale (XMacBitmap8GrayScale& inOriginal);
	virtual ~XMacBitmap8GrayScale ();
	
	// Inherited form XMacBitmap
	virtual Boolean FromFile (VFile& inFile);
	virtual Boolean NewBitmap (sLONG inWidth, sLONG inHeight);
	virtual XMacBitmap* Copy ();
	
	virtual VColor 	GetPixel (sLONG inX, sLONG inY);
	virtual void 	SetPixel (sLONG inX, sLONG inY, VColor& inColor);
	virtual void 	SetPixel (sLONG inX, sLONG inY, uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha);
	
	virtual void	InvertBitmap ();

	virtual Boolean _Allocate (sLONG inWidth, sLONG inHeight);
	virtual Boolean _Free ();
	
protected:
	CTabHandle _BuildGrayColorTable ();
};


class XTOOLBOX_API XMacBitmap32 : public XMacBitmap
{
public:
			XMacBitmap32 ();
			XMacBitmap32 (XMacBitmap32& inOriginal);
	virtual	~XMacBitmap32 ();
	
	// Grayscale convertion
	XMacBitmap8GrayScale*	BuiltMask (VPoint inTransPixel);
	virtual void	ConvertToGray ();
	
	// Accessors
	VPixelColor*	GetBitmapBits () { return (VPixelColor*) fBitmapBits; };
	VPixelFormat	GetPixelType () { return fPixelType; };
	
	VBitmapTransparencyMode	GetTransparencyMode () { return fBitmapTransparencyMode; };
	VPoint	GetTransparentPixel () { return fTransparentPixel; };
	VColor	GetTransparentColor () { return VColor (&fTransparentColor); };

	// Inherited form XMacBitmap
	virtual Boolean	FromFile (VFile& inFile);
	virtual Boolean	NewBitmap (sLONG inWidth, sLONG inHeight);
	virtual XMacBitmap*	Copy ();
	
	virtual VColor 	GetPixel (sLONG inX, sLONG inY);
	virtual void 	SetPixel (sLONG inX, sLONG inY, VColor& inColor);
	virtual void	SetPixel (sLONG inX, sLONG inY, uBYTE inRed, uBYTE inGreen, uBYTE inBlue, uBYTE inAlpha);

	virtual void	Draw (PortRef inPort, VRect& inRect, VRect* inDrawRect = NULL, Boolean inUseAlpha = false);
	virtual void	DrawMasked (PortRef inPort, VRect& inRect);
	
	virtual Boolean _Allocate (sLONG inWidth, sLONG inHeight);
	virtual Boolean _Free ();
	
protected:
	VBitmapTransparencyMode	fBitmapTransparencyMode;
	VPoint	fTransparentPixel;
	VColor	fTransparentColor;
	
	Boolean	BuildMask ();
	void	MergeBitmap (XMacBitmap32& inWith, VRect* inDrawRect = NULL);
	
	void	_SetAlpha (char inAlpha);
	CTabHandle	_BuildGrayColorTable ();
	
	static pascal Boolean	_SimpleMaskMatchProc (RGBColor* inColor, sLONG* outPosition);
};


class StMacPortColorSaver
{ 
public:
			StMacPortColorSaver (PortRef inPort, Boolean inSetBW = true);
	virtual ~StMacPortColorSaver ();
	
	void	RestorePortColor ();
	void	SavePortColor (PortRef inPort, Boolean inSetBW = true);
	
private:
	RGBColor 	fOldFore;
	RGBColor 	fOldBack;
	RGBColor	fOldOpColor;
	PortRef 	fPort;	
};


typedef XMacBitmap	XBitmap;
typedef XMacBitmap32	XBitmap32;
typedef XMacBitmap8GrayScale	XBitmapGray;
	
END_TOOLBOX_NAMESPACE

#endif
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
#ifndef __VIcon__
#define __VIcon__

#include "Graphics/Sources/IBoundedShape.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations

class VPicture;


// Class types
typedef enum SystemIcon {
	kICON_TRASH	= 0,
	kICON_DISK,
	kICON_CLOSED_FOLDER,
	kICON_OPENED_FOLDER,
	kICON_SHARED_FOLDER,
	kICON_FLOPPY,
	kICON_GENERIC_FILE,
	kICON_CDROM,
	kICON_LOCKED,
	kICON_UNLOCKED,
	kICON_WRITE_PROTECTED,
	kICON_BACK,
	kICON_FORWARD,
	kICON_HELP,
	kICON_GRID,
	kICON_SORT_ASCENDENT,
	kICON_SORT_DESCENDENT
} SystemIcon;


class XTOOLBOX_API VIcon : public VObject, public IBoundedShape
{
public:
			// Common constructors 
			VIcon (const VResourceFile& inResFile, sLONG inID);
			VIcon (const VResourceFile& inResFile, const VString& inName);
			VIcon (const VString& inIconPath, sLONG inID, const sLONG inType);
			VIcon (const VIcon& inOriginal);
			
			// Constructor from system constants
			VIcon (SystemIcon inSystemIcon);
			
			// Constructor from desktop resource
			VIcon (const VFile& inForFile);
			VIcon (const VFolder& inForFolder);

	virtual	~VIcon ();
	
	// Attributes management
	sLONG		GetID () { return fResID; };
	Boolean		IsValid () const;
	
	void		SetState (PaintStatus inState) { fState = inState; };
	PaintStatus	GetState () const { return fState; };
	
	void		SetAlignment (AlignStyle inHoriz, AlignStyle inVert) { fHorizAlign = inHoriz; fVertAlign = inVert; };
	void		GetAlignment (AlignStyle& outHoriz, AlignStyle& outVert) const { outHoriz = fHorizAlign; outVert = fVertAlign; };
	
	// IBoundedShape API
	virtual void	SetPosBy (GReal inHoriz, GReal inVert);
	virtual void	SetSizeBy (GReal inHoriz, GReal inVert);
	
	virtual void	Set16Only ();

	virtual Boolean	HitTest (const VPoint& inPoint) const;
	virtual void	Inset (GReal inHoriz, GReal inVert);
	
	// Operators
	void	FromResource (VHandle inIconHandle);

	// Native operators
#if VERSIONMAC
	operator	const IconRef& () const { return reinterpret_cast<const IconRef&>( fData); };
	
	IconAlignmentType	MAC_GetAlignmentType () const;
	IconTransformType	MAC_GetTransformType () const;
#endif	// VERSIONMAC


#if VERSIONWIN
	operator	const HICON& () const;
#endif	// VERSIONWIN

	
protected:
	PaintStatus	fState;
	AlignStyle	fHorizAlign;
	AlignStyle	fVertAlign;
	sLONG		fResID;
	
#if VERSIONMAC
	void*		fData;
#endif

#if VERSIONWIN
	void*		fData16;
	void*		fData32;
	void*		fData48;
#endif
	
private:
	void	_Init ();
};

END_TOOLBOX_NAMESPACE

#endif

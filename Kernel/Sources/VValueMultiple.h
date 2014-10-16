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
#ifndef __VValueMultiple__
#define __VValueMultiple__

#include "Kernel/Sources/VValueSingle.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VValueMultiple : public VValue
{
public:
			VValueMultiple ();
	virtual	~VValueMultiple ();
	
	virtual ValueKind	GetValueKind () const { return VK_UNDEFINED; };
	virtual Boolean	CanBeEvaluated () const { return false; };
	
	// Item support
	virtual sLONG	GetCount () const = 0;

	// Inherited from VValue
	virtual void	FromBoolean (Boolean inValue, sLONG inElement);
	virtual void	FromByte (sBYTE inValue, sLONG inElement);
	virtual void	FromWord (sWORD inValue, sLONG inElement);
	virtual void	FromLong (sLONG inValue,  sLONG inElement);
	virtual void	FromLong8 (sLONG8 inValue, sLONG inElement);
	virtual void	FromReal (Real inValue, sLONG inElement);
	virtual void	FromFloat (const VFloat& inValue, sLONG inElement);
	virtual void	FromString (const VString& inValue, sLONG inElement);
	virtual void	FromDuration (const VDuration& inValue, sLONG inElement);
	virtual void	FromTime (const VTime& inValue, sLONG inElement);

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;
	virtual VValue*	Clone () const;
};

END_TOOLBOX_NAMESPACE

#endif

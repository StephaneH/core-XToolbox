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
#ifndef __ICommandControler__
#define __ICommandControler__

#include "KernelIPC/Sources/VCommand.h"

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VIcon;


class XTOOLBOX_API ICommandControler : public virtual ICommandListener
{
public:
			ICommandControler ();
			ICommandControler (const ICommandControler& inOriginal);
	virtual	~ICommandControler ();
	
	// Item management
	void	AddItem (VCommand* inCommand, VValue* ioValue, VIcon* inIcon);
	void	RemoveItems (const VCommand* inCommand);
	void	RemoveItems (const VValue* inValue);
	VIndex	FindItem (const VCommand* inCommand, const VValueSingle* inValue);
	VIndex	GetItemCount () const;
	
	// Accessors
	void	SetControlerType (ControlerType inType) { fControlerType = inType; };
	ControlerType	GetControlerType () const { return fControlerType; };
	
	VCommand*	GetItemCommand (VIndex inIndex) const;
	VValue*		GetItemValue (VIndex inIndex) const;
	VIcon*		GetItemIcon (VIndex inIndex) const;
	
protected:
	ControlerType	fControlerType;
	VArrayOf<VCommand*>	fCommandList;
	VArrayOf<VValue*>	fValueList;
	VArrayOf<VIcon*>	fIconList;
	
	// Override to implement controler features
	virtual void	DoAddItem (VIndex inIndex, VValue* ioValue, VIcon* inIcon);
	virtual void	DoRemoveItem (VIndex inIndex);
	virtual void	DoUpdateItem (VIndex inIndex, VTriState inState);
	virtual void	DoEnableItem (VIndex inIndex, Boolean inEnable);
	
	// Inherited from ICommandListener
	virtual void	DoCommandTriggered (VCommand* ioCommand, VCommandContext *ioContext);
	virtual void	DoCommandEnabled (VCommand* ioCommand, Boolean inIsEnabled);
	virtual void	DoCommandUpdatedState (VCommand *ioCommand, VValueSingle* ioValue, VTriState inState);
};

END_TOOLBOX_NAMESPACE

#endif

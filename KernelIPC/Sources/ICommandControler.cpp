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
#include "VKernelIPCPrecompiled.h"
#include "ICommandControler.h"


// ---------------------------------------------------------------------------
// ICommandControler::ICommandControler
// ---------------------------------------------------------------------------
// Default constructor
ICommandControler::ICommandControler()
{
}


// ---------------------------------------------------------------------------
// ICommandControler::ICommandControler(const ICommandControler& inOriginal)
// ---------------------------------------------------------------------------
// Constructor from parent
ICommandControler::ICommandControler(const ICommandControler& /*inOriginal*/)
{
}


// ---------------------------------------------------------------------------
// ICommandControler::~ICommandControler
// ---------------------------------------------------------------------------
// Default destructor
ICommandControler::~ICommandControler()
{
}


// ---------------------------------------------------------------------------
// ICommandControler::AddItem (VCommand*, VValue*, VIcon*)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::AddItem (VCommand* inCommand, VValue* ioValue, VIcon* inIcon)
{
	assert(inCommand != NULL);
	if (inCommand == NULL) return;
	
	fCommandList.AddTail(inCommand);
	fValueList.AddTail(ioValue);
	fIconList.AddTail(inIcon);
	
	DoAddItem(fCommandList.GetCount(), ioValue, inIcon);
}


// ---------------------------------------------------------------------------
// ICommandControler::RemoveItems (const VCommand* inCommand)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::RemoveItems (const VCommand* inCommand)
{
	assert(inCommand != NULL);
	if (inCommand == NULL) return;
	
	VCommand*	curCommand;
	VArrayIteratorOf<VCommand*>	iterator(fCommandList);
	
	while ((curCommand = iterator.Previous()) != NULL)
	{
		if (curCommand->IsID(inCommand->GetCommandID()))
		{
			VIndex	index = iterator.GetPos();
			DoRemoveItem(index);
			
			fCommandList.DeleteNth(index);
			fValueList.DeleteNth(index);
			fIconList.DeleteNth(index);
		}
	}
}


// ---------------------------------------------------------------------------
// ICommandControler::RemoveItems (const VValue* inValue)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::RemoveItems (const VValue* inValue)
{
	assert(inValue != NULL);
	if (inValue == NULL) return;
	
	VValue*	curValue;
	VArrayIteratorOf<VValue*>	iterator(fValueList);
	
	while ((curValue = iterator.Previous()) != NULL)
	{
		if (curValue->EqualToSameKind(inValue))
		{
			VIndex	index = iterator.GetPos();
			DoRemoveItem(index);
			
			fCommandList.DeleteNth(index);
			fValueList.DeleteNth(index);
			fIconList.DeleteNth(index);
		}
	}
}


// ---------------------------------------------------------------------------
// ICommandControler::FindItem (const VCommand* inCommand, const VValue* inValue)
// ---------------------------------------------------------------------------
// 
VIndex
ICommandControler::FindItem (const VCommand* inCommand, const VValueSingle* inValue)
{
	assert(inCommand != NULL);
	if (inCommand == NULL) return -1;
	
	VIndex	index = -1;
	VCommand*	curCommand;
	VArrayIteratorOf<VCommand*>	iterator(fCommandList);
	
	while ((curCommand = iterator.Previous()) != NULL)
	{
		if (curCommand->IsID(inCommand->GetCommandID()) &&
			(inValue == NULL || inValue->EqualToSameKind(fValueList[iterator.GetPos() - 1])))
		{
			index = iterator.GetPos();
		}
	}
	
	return index;
}


// ---------------------------------------------------------------------------
// ICommandControler::GetItemCount () const
// ---------------------------------------------------------------------------
// 
VIndex
ICommandControler::GetItemCount () const
{
	return fCommandList.GetCount();
}


// ---------------------------------------------------------------------------
// ICommandControler::GetItemCommand (VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
VCommand*
ICommandControler::GetItemCommand (VIndex inIndex) const
{
	assert(inIndex > 0 && inIndex <= fCommandList.GetCount());
	
	return fCommandList.GetNth(inIndex);
}


// ---------------------------------------------------------------------------
// ICommandControler::GetItemValue (VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
VValue*
ICommandControler::GetItemValue (VIndex inIndex) const
{
	assert(inIndex > 0 && inIndex <= fValueList.GetCount());
	
	return fValueList.GetNth(inIndex);
}


// ---------------------------------------------------------------------------
// ICommandControler::GetItemIcon (VIndex inIndex) const
// ---------------------------------------------------------------------------
// 
VIcon*
ICommandControler::GetItemIcon (VIndex inIndex) const
{
	assert(inIndex > 0 && inIndex <= fIconList.GetCount());
	
	return fIconList.GetNth(inIndex);
}


// ---------------------------------------------------------------------------
// ICommandControler::DoAddItem (VIndex, VValue*, VIcon*)
// ---------------------------------------------------------------------------
// Override to be notified when a new item is added.
// You should tipically add a GUI object to your controler.
//
void
ICommandControler::DoAddItem (VIndex /*inIndex*/, VValue* /*ioValue*/, VIcon* /*inIcon*/)
{
}


// ---------------------------------------------------------------------------
// ICommandControler::DoRemoveItem (VIndex inIndex)
// ---------------------------------------------------------------------------
// Override to be notified when a new item is removed.
// You should tipically remove the corresponding GUI object from your controler.
// 
void
ICommandControler::DoRemoveItem (VIndex /*inIndex*/)
{
}


// ---------------------------------------------------------------------------
// ICommandControler::DoUpdateItem (VIndex inIndex, VTriState inState)
// ---------------------------------------------------------------------------
// Override to be notified when a new item need to be updated.
// You should tipically update the corresponding GUI object according to inState.
// 
void
ICommandControler::DoUpdateItem (VIndex /*inIndex*/, VTriState /*inState*/)
{
}


// ---------------------------------------------------------------------------
// ICommandControler::DoEnableItem (VIndex inIndex, Boolean inEnable)
// ---------------------------------------------------------------------------
// Override to be notified when a new item need to be updated.
// You should tipically enable the corresponding GUI object according to inEnable.
// 
void
ICommandControler::DoEnableItem (VIndex /*inIndex*/, Boolean /*inEnable*/)
{
}


// ---------------------------------------------------------------------------
// ICommandControler::DoCommandTriggered (VCommand* ioCommand, VCommandContext *ioContext)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::DoCommandTriggered (VCommand* ioCommand, VCommandContext *ioContext)
{
	VIndex	index = FindItem(ioCommand, ioContext->GetValueSingle());
	
	if (index > 0)
		DoUpdateItem(index, TRI_STATE_ON);
}


// ---------------------------------------------------------------------------
// ICommandControler::DoCommandEnabled (VCommand* ioCommand, Boolean inIsEnabled)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::DoCommandEnabled (VCommand* ioCommand, Boolean inIsEnabled)
{
	assert(ioCommand != NULL);
	if (ioCommand == NULL) return;
	
	VCommand*	curCommand;
	VArrayIteratorOf<VCommand*>	iterator(fCommandList);
	
	while ((curCommand = iterator.Next()) != NULL)
	{
		if (curCommand->IsID(ioCommand->GetCommandID()))
			DoEnableItem(iterator.GetPos(), inIsEnabled);
	}
}


// ---------------------------------------------------------------------------
// ICommandControler::DoCommandUpdatedState (VCommand*, VValue*, VTriState)
// ---------------------------------------------------------------------------
// 
void
ICommandControler::DoCommandUpdatedState (VCommand *ioCommand, VValueSingle* ioValue, VTriState inState)
{
	VIndex	index = FindItem(ioCommand, ioValue);
	
	if (index > 0)
		DoUpdateItem(index, inState);
}


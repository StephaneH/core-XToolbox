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
#ifndef __VPluginImp__
#define __VPluginImp__

#include "KernelIPC/Sources/CPlugin.h"
#include "KernelIPC/Sources/VComponentImp.h"

BEGIN_TOOLBOX_NAMESPACE

// VPluginImp handle minimal CPlugin interface
//
template <class Type>
class VPluginImp : public VComponentImp<Type>
{
public:
			VPluginImp ();
	virtual	~VPluginImp ();
	
	// Plugin info accessors
	virtual void	GetPluginName (VString& outString);
	
	// Init events
	virtual Boolean	Init ();
	virtual void	DeInit ();
	
	virtual void	EnterProcess ();
	virtual void	LeaveProcess ();
	
	virtual Boolean	ServerInit ();
	virtual void	ServerDeInit ();
	virtual void	ServerCleanUp ();
	
	virtual Boolean	ClientInit ();
	virtual void	ClientDeInit ();
	
	virtual void	ClientConnect ();
	virtual void	ClientDisconnect ();
};


template <class Type>
VPluginImp<Type>::VPluginImp()
{
}


template <class Type>
VPluginImp<Type>::~VPluginImp()
{
}


template <class Type>
void VPluginImp<Type>::GetPluginName(VString& outString)
{
	outString = L"Plugin";
}


template <class Type>
Boolean VPluginImp<Type>::Init()
{
	return true;
}


template <class Type>
void VPluginImp<Type>::DeInit()
{
}


template <class Type>
void VPluginImp<Type>::EnterProcess()
{
}


template <class Type>
void VPluginImp<Type>::LeaveProcess()
{
}


template <class Type>
Boolean VPluginImp<Type>::ServerInit()
{
	return true;
}


template <class Type>
void VPluginImp<Type>::ServerDeInit()
{
}


template <class Type>
void VPluginImp<Type>::ServerCleanUp()
{
}


template <class Type>
Boolean VPluginImp<Type>::ClientInit()
{
	return true;
}


template <class Type>
void VPluginImp<Type>::ClientDeInit()
{
}


template <class Type>
void VPluginImp<Type>::ClientConnect()
{
}


template <class Type>
void VPluginImp<Type>::ClientDisconnect()
{
}

END_TOOLBOX_NAMESPACE

#endif
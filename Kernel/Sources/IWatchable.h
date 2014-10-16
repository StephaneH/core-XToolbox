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
#ifndef __IWatchable__
#define __IWatchable__

#include <vector>
#include <map>
#include <string>
#include "Kernel/Sources/VValueSingle.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API IWatchableMenuInfo
{
public:
	IWatchableMenuInfo()
	{
		SetMenuItemInfo("", false, false, 0);
	};
	IWatchableMenuInfo(const char* inTitle,bool inEnable,bool inChecked,long inCommand)
	{
		SetMenuItemInfo(inTitle, inEnable, inChecked, inCommand);
	};
	IWatchableMenuInfo(const IWatchableMenuInfo& inItem)
	{
		SetMenuItemInfo(inItem.fTitle, inItem.fEnable, inItem.fChecked, inItem.fCommand);
	};
	virtual ~IWatchableMenuInfo(){;};

	void SetMenuItemInfo(const char* inTitle,bool inEnable,bool inChecked,long inCommand)
	{
		strcpy(fTitle,inTitle);
		fEnable=inEnable;
		fChecked=inChecked;
		fCommand=inCommand;
	};
	bool IsEnable(){return fEnable;};
	bool IsChecked(){return fChecked;};
	void GetTitle(char* outTitle)
	{
		memcpy(&outTitle[1],fTitle,255);
		outTitle[0]=(char)strlen(fTitle);
	};
	long GetCommandID(){return fCommand;};
private:
	char fTitle[255];
	bool fEnable;
	bool fChecked;
	long fCommand;
};

typedef std::vector<IWatchableMenuInfo> XTOOLBOX_TEMPLATE_API IWatchableMenuInfoArray;

typedef std::map < class IWatchable_Base* , class IWatchable_Base* > _IWatchableObjectMap ;
typedef std::pair < std::string , class IWatchable_Base* > _IWatchableObjectPair ;
typedef _IWatchableObjectMap::iterator _IWatchableObjectMap_Iterrator ;
typedef _IWatchableObjectMap::const_iterator _IWatchableObjectMap_Const_Iterrator ;

typedef std::pair < std::string , _IWatchableObjectMap > _IWatchableTypeObjectPair ;
typedef std::map < std::string , _IWatchableObjectMap > _IWatchableTypeObjectMap ;
typedef _IWatchableTypeObjectMap::iterator _IWatchableTypeObjectMap_Iterrator ;



class IWatchable_Base
{
public:

	IWatchable_Base(){};
	virtual~ IWatchable_Base(){};

	virtual bool IW_IsWatchable(){return false;};
	virtual sLONG IW_CountProperties(){return 0;};
	virtual void IW_GetPropName(sLONG inPropIndex,VValueSingle& outName){;};
	virtual void IW_GetPropValue(sLONG inPropIndex,VValueSingle& outValue){;};
	virtual void IW_SetPropValue(sLONG inPropIndex,VValueSingle& inValue){;};
	virtual void IW_GetPropInfo(sLONG inPropIndex,bool& outEditable,IWatchable_Base** outChild){*outChild=NULL;outEditable=false;};
	virtual short IW_GetIconID(sLONG inPropIndex){return 925;}
	virtual bool IW_UseCustomMenu(sLONG inPropIndex,IWatchableMenuInfoArray &outmenuinfo){return false;}
	virtual void IW_HandleCustomMenu(sLONG inID){;}
	virtual void IW_DumpLeaks(){;}
	void IW_SetHexVal(VObject* l,VValueSingle& outName){;};
	void IW_SetHexVal(void* l,VValueSingle& outName){;};
	void IW_SetHexVal(uBYTE* l,VValueSingle& outName){;};
	void IW_SetHexVal(sBYTE* l,VValueSingle& outName){;};
	void IW_SetHexVal(uLONG l,VValueSingle& outName){;};
	void IW_SetHexVal(uLONG8 l,VValueSingle& outName){;};
	void IW_SetHexVal(sLONG l,VValueSingle& outName){;};
	void IW_SetHexVal(sLONG8 l,VValueSingle& outName){;};
	void IW_SetOSTypeVal(uLONG l,VValueSingle& outName){;};
	void IW_SetClassNameVal(VValueSingle& outName,VObject* inObj=NULL){;};
};

class XTOOLBOX_API IWatchable_Reg_Base :public IWatchable_Base
{
public:
	IWatchable_Reg_Base();
	virtual~ IWatchable_Reg_Base();

	virtual bool IW_IsWatchable(){return true;};

	virtual sLONG IW_CountProperties()=0;
	virtual void IW_GetPropName(sLONG inPropIndex,VValueSingle& outName)=0;
	virtual void IW_GetPropValue(sLONG inPropIndex,VValueSingle& outValue)=0;
	virtual void IW_SetPropValue(sLONG inPropIndex,VValueSingle& inValue){;};
	virtual void IW_GetPropInfo(sLONG inPropIndex,bool& outEditable,IWatchable_Base** outChild)
	{
		*outChild=NULL;
		outEditable=false;
	};
	virtual short IW_GetIconID(sLONG inPropIndex)
	{
		return 925;
	}
	virtual bool IW_UseCustomMenu(sLONG inPropIndex,IWatchableMenuInfoArray &outmenuinfo)
	{
		return false;
	}
	virtual void IW_HandleCustomMenu(sLONG inID)
	{
		;
	}
	void IW_SetHexVal(VObject* l,VValueSingle& outName);
	void IW_SetHexVal(void* l,VValueSingle& outName);
	void IW_SetHexVal(uBYTE* l,VValueSingle& outName);
	void IW_SetHexVal(sBYTE* l,VValueSingle& outName);
	void IW_SetHexVal(uLONG l,VValueSingle& outName);
	void IW_SetHexVal(uLONG8 l,VValueSingle& outName);
	void IW_SetHexVal(sLONG l,VValueSingle& outName);
	void IW_SetHexVal(sLONG8 l,VValueSingle& outName);
	void IW_SetOSTypeVal(uLONG l,VValueSingle& outName);
	void IW_SetClassNameVal(VValueSingle& outName,VObject* inObj=NULL);
	
	static bool IsValide(IWatchable_Base* inPict);
	static uLONG Count(const std::string inClassName);
	static _IWatchableObjectMap* GetObjectMap(const std::string inClassName);
	static void Dump();
protected:

	void _Add(const char* inTypeName);
	void _Remove(const char* inTypeName);

private:
	static _IWatchableObjectMap			sGlobObjectList;	
	static _IWatchableTypeObjectMap		sObjectMap;
};

template <class T>
class IWatchable: public IWatchable_Reg_Base
{
public:

	IWatchable<T>()
	{
		_Add(typeid(T).name());
	};
	virtual~ IWatchable<T>()
	{
		_Remove(typeid(T).name());
	};
};
/*
template <class T>
class IWatchable_NoRegister
{
public:

	IWatchable_NoRegister<T>(){};
	virtual~ IWatchable_NoRegister<T>(){};

	virtual sLONG IW_CountProperties(){return 0;};
	virtual void IW_GetPropName(sLONG inPropIndex,VValueSingle& outName){;};
	virtual void IW_GetPropValue(sLONG inPropIndex,VValueSingle& outValue){;};
	virtual void IW_SetPropValue(sLONG inPropIndex,VValueSingle& inValue){;};
	virtual void IW_GetPropInfo(long inPropIndex,bool& outEditable,IWatchable_Base** outChild){*outChild=NULL;outEditable=false;};
	virtual short IW_GetIconID(sLONG inPropIndex){return 925;}
	virtual bool IW_UseCustomMenu(sLONG inPropIndex,IWatchableMenuInfoArray &outmenuinfo){return false;}
	virtual void IW_HandleCustomMenu(sLONG inID){;}
	void IW_SetHexVal(VObject* l,VValueSingle& outName){;};
	void IW_SetHexVal(void* l,VValueSingle& outName){;};
	void IW_SetHexVal(uBYTE* l,VValueSingle& outName){;};
	void IW_SetHexVal(sBYTE* l,VValueSingle& outName){;};
	void IW_SetHexVal(uLONG l,VValueSingle& outName){;};
	void IW_SetHexVal(uLONG8 l,VValueSingle& outName){;};
	void IW_SetHexVal(sLONG l,VValueSingle& outName){;};
	void IW_SetHexVal(sLONG8 l,VValueSingle& outName){;};
	void IW_SetOSTypeVal(uLONG l,VValueSingle& outName){;};
	void IW_SetClassNameVal(VValueSingle& outName,VObject* inObj=NULL){;};
};
*/

// pp ATTENTION IWATCHABLE n'est pas threadsafe !!! en mode preemptif utiliser IWATCHABLE_NOREG

#define IWATCHABLE(x) ,public XBOX::IWatchable<x>
#define IWATCHABLE_NOREG(x) ,public XBOX::IWatchable_Base
 
#if VERSIONDEBUG
	#define IWATCHABLE_DEBUG(x) ,public XBOX::IWatchable<x>
#else
	#define IWATCHABLE_DEBUG(x) ,public XBOX::IWatchable_Base
#endif

END_TOOLBOX_NAMESPACE

#endif
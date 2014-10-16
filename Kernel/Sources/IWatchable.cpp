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
#include "VKernelPrecompiled.h"
#include "VString.h"
#include "IWatchable.h"

_IWatchableTypeObjectMap IWatchable_Reg_Base::sObjectMap;
_IWatchableObjectMap IWatchable_Reg_Base::sGlobObjectList;

IWatchable_Reg_Base::IWatchable_Reg_Base()
{
	
}
IWatchable_Reg_Base::~IWatchable_Reg_Base()
{
		
}

uLONG IWatchable_Reg_Base::Count(const std::string inClassName)
{
	uLONG count=0;
	if(inClassName!="")
		count=(uLONG)sObjectMap[inClassName].size();
	else
	{
		count=(uLONG)sGlobObjectList.size();
	}
	return count;
}

bool IWatchable_Reg_Base::IsValide(IWatchable_Base* inObj)
{
	bool result=false;
	if(inObj)
	{
		_IWatchableObjectMap_Const_Iterrator it=sGlobObjectList.find(inObj);
		result = it!=sGlobObjectList.end();
	}
	else
		result= false;
	return result;
}
void IWatchable_Reg_Base::_Add(const char* inTypeName)
{
	std::string str(inTypeName);
	sGlobObjectList[this]=this;
	sObjectMap[inTypeName][this]=this;
}
void IWatchable_Reg_Base::_Remove(const char* inTypeName)
{
	std::string str(inTypeName);
	sGlobObjectList.erase(this);
	sObjectMap[inTypeName].erase(this);
}
void IWatchable_Reg_Base::Dump()
{
	#if VERSIONDEBUG
	for(_IWatchableTypeObjectMap_Iterrator typiter=IWatchable_Reg_Base::sObjectMap.begin();typiter!=IWatchable_Reg_Base::sObjectMap.end();typiter++)
	{
		if ((*typiter).second.empty())
			continue;
		
		#if VERSIONWIN
		OutputDebugString((*typiter).first.c_str());
		OutputDebugString("\r\n");
		#else
		fprintf(stderr,(*typiter).first.c_str());
		fprintf(stderr,"\r\n");
		#endif
		for(_IWatchableObjectMap_Iterrator oiter=(*typiter).second.begin();oiter!=(*typiter).second.end();oiter++)
		{
			if((*oiter).second!=0)
			{
				IRefCountable* iref=dynamic_cast<IRefCountable*>((*oiter).second);
				if(iref)
				{
					char buff[255];
					sprintf(buff,"%i",iref->GetRefCount());
					#if VERSIONWIN
					OutputDebugString("refcount : ");
					OutputDebugString(buff);
					OutputDebugString("\r\n");
					#else
					fprintf(stderr,"refcount : ");
					fprintf(stderr,buff);
					fprintf(stderr,"\r\n");
					#endif
					(*oiter).second->IW_DumpLeaks();
				}
				else
				#if VERSIONWIN
					OutputDebugString("?\r\n");
				#else
					fprintf(stderr,"?\r\n");
				#endif
			}
		}
	}
	#endif
}
_IWatchableObjectMap* IWatchable_Reg_Base::GetObjectMap(const std::string inClassName)
{
	if(inClassName!="")
		return &sObjectMap[inClassName];
	else
		return &sGlobObjectList;
}

void IWatchable_Reg_Base::IW_SetHexVal(VObject* l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG_PTR) l, outName);
}

void IWatchable_Reg_Base::IW_SetHexVal(void* l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG_PTR) l, outName);
}

void IWatchable_Reg_Base::IW_SetHexVal(uBYTE* l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG_PTR) l, outName);
}
void IWatchable_Reg_Base::IW_SetHexVal(sBYTE* l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG_PTR) l, outName);
}

void IWatchable_Reg_Base::IW_SetHexVal(sLONG l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG) l, outName);
}
void IWatchable_Reg_Base::IW_SetHexVal(sLONG8 l,VValueSingle& outName)
{
	IW_SetHexVal((uLONG8) l, outName);
}

void IWatchable_Reg_Base::IW_SetHexVal(uLONG l,VValueSingle& outName)
{
	char c[255];
	sprintf(c,"%#000000lx",l);
	outName.FromString(c);
}
void IWatchable_Reg_Base::IW_SetHexVal(uLONG8 l,VValueSingle& outName)
{
	char c[255];
	sprintf(c,"%#000000000000I64x",l);
	outName.FromString(c);
}

void IWatchable_Reg_Base::IW_SetOSTypeVal(uLONG l,VValueSingle& outName)
{
	char c[7]="'    '";
#if VERSIONWIN
	l=ByteSwapValue(l);
#endif
	memcpy(&c[1],&l,sizeof(uLONG));
	outName.FromString(c);
}
void IWatchable_Reg_Base::IW_SetClassNameVal(VValueSingle& outName,VObject* inObj)
{
	const char* c;
	if(!inObj)
		c=typeid(*this).name();
	else
		c=typeid(*inObj).name();
	outName.FromString(c);
}

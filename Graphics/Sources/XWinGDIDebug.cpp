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
#include "VGraphicsPrecompiled.h"

#define __NO_GDIDEBUG_INCLUDE__

#pragma pack(push, 8)
#include <windows.h>
#include <commctrl.h>
#pragma pack(pop)

#include <stdlib.h>
#include <tchar.h>
#include <string>
#include <map>
#include <process.h>


#if _DEBUG && ENABLE_GDI_DEBUG

// for preamptiv thread use set 
#define		_SECURE_GDI_DEBUG_ 1
#define		_GDI_DEBUG_DIAL_USETIMER 1
#define		_GDI_DEBUG_DIAL_TIMERFREQ 500

#if USE_ALTURA
#ifdef __cplusplus
extern "C" {
#endif
	extern unsigned char IsCachedBrush(HBRUSH hBR);
#ifdef __cplusplus
}
#endif
#endif

#ifdef UNICODE
typedef  std::wstring str;
#else
typedef  std::string str;
#endif


BOOL sGDIDGB_AssertOnDeleteStockObject	= false;
BOOL sGDIDGB_AssertOnDoubleSelect	= false;
BOOL sGDIDGB_KeepDeletedObject	= true;


#define WM_GDI_DEBUGDIAL_UPDATE WM_APP+666
#define WM_GDI_DEBUGDIAL_SYNCLISTCHECK WM_APP+667


typedef struct _GDI_Dial_Data
{
	long nbAlloc;
	long nbDel;
}_GDI_Dial_Data;


class _GDI_DebugDial
{
	public:
	_GDI_DebugDial();
	virtual ~_GDI_DebugDial();
	
	void OpenDial();
	
	static void __cdecl _ThreadProc(void *dummy);
	void ThreadProc();
	static int _DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	int DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK _KeyboardProc(int code,WPARAM wParam,LPARAM lParam);

	void _UpdateDial(long id,long count);
	void _Reaffect(long id,long count);
	void _InitList();
	void _SyncListCheck()
	{
		if(fDial)
			SendMessage(fDial,WM_GDI_DEBUGDIAL_SYNCLISTCHECK,0,0);
	}
	static HHOOK fHook;
	DWORD		fThreadID;
	HINSTANCE	fRes;
	HWND		fDial;
	HWND		fList;
	BOOL		fListInited;
	_GDI_Dial_Data fDialData[15];
};


HHOOK _GDI_DebugDial::fHook=NULL;
_GDI_DebugDial sGDIDebugDial;


typedef struct _GDI_Obj_Info
{
	LPTSTR	fObjName;
	BOOL	fDebug;
	BOOL	fCanDebug;
} _GDI_Obj_Info;


_GDI_Obj_Info sGDIObjDesc[] = 
{
	{"UNKNOWN",false,false}, //unused
	{"PEN",true,true},
	{"BRUSH",true,true},
	{"DC",true,true},
	{"METADC",false,false}, // tbd
	{"PAL",true,true},
	{"FONT",true,true},
	{"BITMAP",true,true},
	{"REGION",true,true},
	{"METAFILE",false,false}, // tbd
	{"MEMDC",true,true},
	{"EXTPEN",true,true},
	{"ENHMETADC",false,false}, // tbd
	{"ENHMETAFILE",false,false},// tbd
	{"COLORSPACE",false,false}, // tbd
};


class _GDI_StockObj_List
{
	public:
	_GDI_StockObj_List()
	{
		for(long i=0;i<=STOCK_LAST;i++)
		{
			sGDIStockObj[i]=GetStockObject(i);
		}
	}
	BOOL IsStockObject(HGDIOBJ inObj)
	{
		for(long i=0;i<=STOCK_LAST;i++)
		{
			if(sGDIStockObj[i]==inObj)
				return true;
		}
		return false;
	}
	virtual ~_GDI_StockObj_List(){;};
	HGDIOBJ sGDIStockObj[STOCK_LAST+1];
};


class _GDI_Alloc_Info
{
public:
	_GDI_Alloc_Info()
	{
		fSelected=0;
		fCreated=0;
		fAllocLineNumber=0;
		fDelLineNumber=0;
		fDelFileName=_T("");
		fAllocFileName=_T("");
	}
	_GDI_Alloc_Info(unsigned char inType,LPCTSTR inFile, int inNumber)
	{
		fCreated=0;
		fSelected=0;
		fAssertOnDelete=0;
		fAllocLineNumber=inNumber;
		fObjectType=inType;
		fAllocFileName=inFile;
		fDelLineNumber=0;
		fDelFileName=_T("");
	}
	_GDI_Alloc_Info(const _GDI_Alloc_Info& inInfo)
	{
		fSelected=inInfo.fSelected;
		fCreated=inInfo.fCreated;
		fAssertOnDelete=inInfo.fAssertOnDelete;
		fObjectType=inInfo.fObjectType;
		fAllocLineNumber=inInfo.fAllocLineNumber;
		fAllocFileName=inInfo.fAllocFileName;
		fDelLineNumber=inInfo.fDelLineNumber;
		fDelFileName=inInfo.fDelFileName;
	}
	_GDI_Alloc_Info& operator =(const _GDI_Alloc_Info &inInfo)
	{
		fSelected=inInfo.fSelected;
		fCreated=inInfo.fCreated;
		fObjectType=inInfo.fObjectType;
		fAssertOnDelete=inInfo.fAssertOnDelete;
		fAllocLineNumber=inInfo.fAllocLineNumber;
		fAllocFileName=inInfo.fAllocFileName;
		fDelLineNumber=inInfo.fDelLineNumber;
		fDelFileName=inInfo.fDelFileName;
		return *this;
	}
	virtual ~_GDI_Alloc_Info()
	{
	}
	unsigned char			fSelected:1;
	unsigned char			fCreated:1;
	unsigned char			fAssertOnDelete:1;
	unsigned char			unused1:5;
	unsigned char			fObjectType;
	int				fAllocLineNumber;
	str		fAllocFileName;
	int				fDelLineNumber;
	str	fDelFileName;
};


typedef std::pair<DWORD, _GDI_Alloc_Info> _GDI_Debug_InfoPair ;
typedef std::map<DWORD, _GDI_Alloc_Info> _GDI_Debug_InfoMap ;
typedef _GDI_Debug_InfoMap::iterator _GDI_Debug_InfoMap_Iterrator ;
typedef _GDI_Debug_InfoMap::pointer _GDI_Debug_InfoMap_Pointer;

static void _gdidbg_OutputDebugString(LPCTSTR inMess,LPCTSTR inFile,int inLine,BOOL addlasterr=false);


class _GDI_Debug_Bloc
{
	public:
	_GDI_Debug_Bloc()
	{
		InitializeCriticalSection(&fCrit);
	};
	virtual ~_GDI_Debug_Bloc()
	{
		_GDI_Debug_InfoMap_Iterrator it;
		for(it=fAlloc.begin();it!=fAlloc.end();it++)
		{
			str mess=_T("LEAKS : ");
			mess+=sGDIObjDesc[it->second.fObjectType].fObjName;
			_gdidbg_OutputDebugString(mess.c_str(),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
		}
	}
	void Clear()
	{
		Lock();
		fDel.clear();
		fAlloc.clear();
		Unlock();
	}
	void Add(_GDI_Debug_InfoPair &inPair)
	{
		Lock();
		fAlloc.insert(inPair);
		{
			_GDI_Debug_InfoMap_Iterrator it=fDel.find(inPair.first);
			if(it == fDel.end())
			{
			}
			else
			{
				fDel.erase(it);
			}
		}
		sGDIDebugDial._UpdateDial(inPair.second.fObjectType,fAlloc.size());
		Unlock();
	}
	void Delete(_GDI_Debug_InfoPair inPair)
	{
		Lock();
		if(sGDIDGB_KeepDeletedObject)
			fDel.insert(inPair);
		fAlloc.erase(inPair.first);
		if(inPair.second.fAssertOnDelete)
			DebugBreak();
		sGDIDebugDial._UpdateDial(inPair.second.fObjectType,fAlloc.size());
		Unlock();
	};
	
	void Lock()
	{
		#if _SECURE_GDI_DEBUG_
		EnterCriticalSection(&fCrit);
		#endif
	}
	void Unlock()
	{
		#if _SECURE_GDI_DEBUG_
		LeaveCriticalSection(&fCrit);
		#endif
	}
	long GetAllocCount()
	{
		Lock();
		long res= fAlloc.size();
		Unlock();
		return res;
	};
	long GetDeletedCount()
	{
		Lock();
		long res = fDel.size();
		Unlock();
		return res;
	};
	_GDI_Debug_InfoMap fAlloc;
	_GDI_Debug_InfoMap fDel;
	CRITICAL_SECTION fCrit;
};


_GDI_Debug_Bloc _gGDIDebugArray[16];
_GDI_StockObj_List _gStockObject;


static void _gdidbg_ClearMap()
{
	for(long i=0;i<=14;i++)
	{
		_gGDIDebugArray[i].Clear();
	}
	sGDIDebugDial._UpdateDial(0,0);
}


static void _gdidbg_OutputDebugString(LPCTSTR inMess,LPCTSTR inFile,int inLine,BOOL addlasterr)
{
	str mess;
	TCHAR buff[7];
	
	_ltot(inLine,buff,10);
	mess+=inFile;
	mess+=_T("(");
	mess+=buff;
	mess+=") : ";
	mess+=inMess;
	if(addlasterr)
	{
		DWORD err=GetLastError();
		_ltot(err,buff,10);
		mess+=_T(" (");
		mess+=buff;
		mess+=_T(") ");
	}
	mess+=_T("\r\n");
	OutputDebugString(mess.c_str());
} 


static HGDIOBJ _gdidbg_AddObject(HGDIOBJ obj,LPCTSTR inFile, int inNumber,BOOL inCreated=true)
{
	int type=GetObjectType(obj);
	
	if(type>=1 && type<=16)
	{
		if(sGDIObjDesc[type].fDebug)
		{
			_GDI_Debug_InfoPair pair((DWORD)obj,_GDI_Alloc_Info(type,inFile,inNumber));
			pair.second.fCreated=inCreated;
			_gGDIDebugArray[type].Add(pair);
		}
	}
	else
	{
		_gdidbg_OutputDebugString(_T("BAD OBJECT"),inFile,inNumber,true);
	}
	return obj;
}


static BOOL _gdidbg_GlobalFind(HGDIOBJ inObj,_GDI_Debug_InfoMap_Iterrator& ouIter,long& inMapID,BOOL &outAlloc)
{
	BOOL result=false;
	for(long i=1;i<=15;i++)
	{
		ouIter=_gGDIDebugArray[i].fAlloc.find((DWORD)inObj);
		if(ouIter!= _gGDIDebugArray[i].fAlloc.end())
		{
			result=true;
			inMapID=i;
			outAlloc=true;
			break;
		}
		else
		{
			ouIter=_gGDIDebugArray[i].fDel.find((DWORD)inObj);
			if(ouIter!= _gGDIDebugArray[i].fDel.end())
			{
				result=true;
				outAlloc=false;
				inMapID=i;
				break;
			}
		}
	}
	return result;
}


static void _gdidbg_DeleteObject(HGDIOBJ obj,LPCTSTR inFile, int inNumber,BOOL inRelease=false)
{
	BOOL tyfindobj=true;
	int type=GetObjectType(obj);
	if(type>=1 && type<=16)
	{
		tyfindobj=sGDIObjDesc[type].fDebug;
	}
	else
	{
		long outMap;
		BOOL outAlloc;
		tyfindobj=false;
		type=0;
		_gdidbg_OutputDebugString(_T("BAD OBJECT"),inFile,inNumber,true);
		_GDI_Debug_InfoMap_Iterrator it;
		if(_gdidbg_GlobalFind(obj,it,outMap,outAlloc))
		{
			str mess=sGDIObjDesc[it->second.fObjectType].fObjName;
			mess+=_T(" ALREADY DELETED : ");
			_gdidbg_OutputDebugString(mess.c_str(),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
		}
		else
		{
			str mess=sGDIObjDesc[type].fObjName;
			if(sGDIDGB_KeepDeletedObject)
				mess+=_T(" NOT IN MAP : ");
			else
				mess+=_T(" NOT IN MAP OR ALREADY DELETED");
			_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
		}
	}
	
	if(tyfindobj)
	{
		_GDI_Debug_InfoMap_Iterrator it=_gGDIDebugArray[type].fAlloc.find((DWORD)obj);
		if(it == _gGDIDebugArray[type].fAlloc.end())
		{
			it=_gGDIDebugArray[type].fDel.find((DWORD)obj);
			if(it == _gGDIDebugArray[type].fDel.end())
			{
				str mess=sGDIObjDesc[type].fObjName;
				if(_gStockObject.IsStockObject(obj))
				{
					if(sGDIDGB_AssertOnDeleteStockObject)
					{
						mess+=_T(" DELETE STOCK OBJECT : ");
						_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
					}
				}
				else
				{
					if(sGDIDGB_KeepDeletedObject)
						mess+=_T(" NOT IN MAP : ");
					else
						mess+=_T(" NOT IN MAP OR ALREADY DELETED :");
					_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
					if(type==OBJ_BRUSH)
					{
						#if USE_ALTURA
						if(IsCachedBrush((HBRUSH)obj))
						{
							_gdidbg_OutputDebugString(_T("DELETE ALTURA BRUSH !!!!!"),inFile,inNumber);
						}
						#endif
					}
				}
			}
			else
			{
				str mess=sGDIObjDesc[it->second.fObjectType].fObjName;
				mess+=_T(" ALREADY DELETED : ");
				_gdidbg_OutputDebugString(mess.c_str(),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
			}
		}
		else
		{
			if(it->second.fCreated)
			{
				if(inRelease)
					_gdidbg_OutputDebugString(_T("USE RELEASEDC"),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
			}
			else
			{
				if(!inRelease)
					_gdidbg_OutputDebugString(_T("USE DELETEDC"),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
			}
			it->second.fDelFileName=inFile;
			it->second.fDelLineNumber=inNumber;
			if(it->second.fAssertOnDelete)
				DebugBreak();
			
			_gGDIDebugArray[type].Delete(*it);
		}
	}
}


static void _gdidbg_ReleaseObject(HGDIOBJ obj,LPCTSTR inFile, int inNumber)
{
	BOOL tyfindobj=true;
	int type=GetObjectType(obj);
	if(type>=1 && type<=16)
	{
		tyfindobj=sGDIObjDesc[type].fDebug;
	}
	else
	{
		long outMap;
		BOOL outAlloc;
		tyfindobj=false;
		type=0;
		_gdidbg_OutputDebugString(_T("BAD OBJECT"),inFile,inNumber,true);
		_GDI_Debug_InfoMap_Iterrator it;
		if(_gdidbg_GlobalFind(obj,it,outMap,outAlloc))
		{
			if(!outAlloc)
			{
				str mess=sGDIObjDesc[it->second.fObjectType].fObjName;
				mess+=_T(" ALREADY DELETED : ");
				_gdidbg_OutputDebugString(mess.c_str(),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
				_gdidbg_OutputDebugString(_T("<-----"),it->second.fDelFileName.c_str(),it->second.fDelLineNumber);
			}
			else
			{
				_gdidbg_OutputDebugString(_T("BEURK"),inFile,inNumber);
			}
		}
		else
		{
			str mess=sGDIObjDesc[type].fObjName;
			mess+=_T(" NOT IN MAP : ");
			_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
		}
	}

	if(tyfindobj)
	{
		_GDI_Debug_InfoMap_Iterrator it=_gGDIDebugArray[type].fAlloc.find((DWORD)obj);
		if(it == _gGDIDebugArray[type].fAlloc.end())
		{
			it=_gGDIDebugArray[type].fDel.find((DWORD)obj);
			if(it == _gGDIDebugArray[type].fDel.end())
			{
				str mess=sGDIObjDesc[type].fObjName;
				if(_gStockObject.IsStockObject(obj))
				{
					if(sGDIDGB_AssertOnDeleteStockObject)
					{
						mess+=_T(" DELETE STOCK OBJECT : ");
						_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
					}
				}
				else
				{
					mess+=_T(" NOT IN MAP : ");
					_gdidbg_OutputDebugString(mess.c_str(),inFile,inNumber);
				}
			}
			else
			{
				str mess=sGDIObjDesc[it->second.fObjectType].fObjName;
				mess+=_T(" ALREADY DELETED : ");
				_gdidbg_OutputDebugString(mess.c_str(),it->second.fAllocFileName.c_str(),it->second.fAllocLineNumber);
				_gdidbg_OutputDebugString(_T("<-----"),it->second.fDelFileName.c_str(),it->second.fDelLineNumber);
			}
		}
		else
		{
			_gGDIDebugArray[type].Delete(*it);
		}
	}
}


#define gdidbg_AddObject(x) _gdidbg_AddObject(x,file,line)
#define gdidbg_AddBrush(x) (HBRUSH)_gdidbg_AddObject(x,file,line,true)
#define gdidbg_AddPen(x) (HPEN)_gdidbg_AddObject(x,file,line,true)
#define gdidbg_AddPalette(x) (HPALETTE)_gdidbg_AddObject(x,file,line,true)
#define gdidbg_AddFont(x) (HFONT)_gdidbg_AddObject(x,file,line,true)
#define gdidbg_AddRegion(x) (HRGN)_gdidbg_AddObject(x,file,line,true)
#define gdidbg_AddDC(x,y) (HDC)_gdidbg_AddObject(x,file,line,y)
#define gdidbg_AddBitmap(x) (HBITMAP)_gdidbg_AddObject(x,file,line,true)


/*******************************************************************************/
// brush

HBRUSH _DBG_CreateBrushIndirect(const LOGBRUSH *inLogBrush, LPCTSTR file, int line)
{
	return gdidbg_AddBrush(CreateBrushIndirect(inLogBrush));
}


HBRUSH _DBG_CreateDIBPatternBrush(HGLOBAL hPackedDIB,UINT nUsage, LPCTSTR file, int line) 
{
	return gdidbg_AddBrush(CreateDIBPatternBrush(hPackedDIB,nUsage));
}


HBRUSH _DBG_CreateDIBPatternBrushPt(const void* lpPackedDIB, UINT iUsage, LPCTSTR file, int line)
{
	return gdidbg_AddBrush(CreateDIBPatternBrushPt(lpPackedDIB,iUsage));
}


HBRUSH _DBG_CreateHatchBrush(int nIndex,COLORREF crColor , LPCTSTR file, int line)
{
	return gdidbg_AddBrush(CreateHatchBrush(nIndex,crColor));
}


HBRUSH _DBG_CreatePatternBrush(HBITMAP hbmp, LPCTSTR file, int line)
{
	return gdidbg_AddBrush(CreatePatternBrush(hbmp));
}


HBRUSH _DBG_CreateSolidBrush(COLORREF crCol, LPCTSTR file, int line)
{
	return gdidbg_AddBrush(CreateSolidBrush(crCol));
}


/*****************************************************************/
// pen

HPEN _DBG_CreatePen(int instyle, int insize, COLORREF incol, LPCTSTR file, int line)
{
	return gdidbg_AddPen(CreatePen(instyle,insize,incol));
}


HPEN _DBG_CreatePenIndirect( const LOGPEN *pen,LPCTSTR file, int line)
{
	return gdidbg_AddPen(CreatePenIndirect(pen));
}


HPEN _DBG_ExtCreatePen(DWORD instyle,DWORD insize,const LOGBRUSH *inbr,DWORD nstyle,const DWORD *stylearr,LPCTSTR file, int line)
{
	return gdidbg_AddPen(ExtCreatePen(instyle,insize,inbr,nstyle,stylearr));
}


/***************************************************************/
// palette

HPALETTE  _DBG_CreatePalette(const LOGPALETTE *pal, LPCTSTR file, int line)
{
	return gdidbg_AddPalette(CreatePalette(pal));
}


HPALETTE  _DBG_CreateHalftonePalette(HDC hdc, LPCTSTR file, int line)
{
	return gdidbg_AddPalette(CreateHalftonePalette(hdc));
}


/***************************************************************/
// font

HFONT _DBG_CreateFontA(int p1, int p2, int p3, int p4, int p5, DWORD p6,DWORD p7, DWORD p8, DWORD p9, DWORD p10, DWORD p11,DWORD p12, DWORD p13, LPCTSTR p14, LPCTSTR file, int line)
{
	return gdidbg_AddFont(CreateFontA(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14));
}


HFONT _DBG_CreateFontW(int p1, int p2, int p3, int p4, int p5, DWORD p6,DWORD p7, DWORD p8, DWORD p9, DWORD p10, DWORD p11,DWORD p12, DWORD p13, LPCWSTR p14, LPCTSTR file, int line)
{
	return gdidbg_AddFont(CreateFontW(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14));
}


HFONT  _DBG_CreateFontIndirectA(const LOGFONTA *p1, LPCTSTR file, int line)
{
	return gdidbg_AddFont(CreateFontIndirectA(p1));
}


HFONT  _DBG_CreateFontIndirectW(const LOGFONTW *p1, LPCTSTR file, int line)
{
	return gdidbg_AddFont(CreateFontIndirectW(p1));
}


/**************************************************************************/
// region

HRGN _DBG_CreateEllipticRgn(int p1, int p2, int p3, int p4,LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreateEllipticRgn(p1,p2,p3,p4));
}


HRGN _DBG_CreateEllipticRgnIndirect(const RECT *p1, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreateEllipticRgnIndirect(p1));
}


HRGN _DBG_CreateRectRgn(int p1, int p2, int p3, int p4, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreateRectRgn(p1,p2,p3,p4));
}


HRGN _DBG_CreateRectRgnIndirect(const RECT *p1, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreateRectRgnIndirect(p1));
}


HRGN _DBG_CreateRoundRectRgn(int p1, int p2, int p3, int p4, int p5, int p6, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreateRoundRectRgn(p1,p2,p3,p4,p5,p6));
}


HRGN _DBG_CreatePolygonRgn(const POINT *p1, int p2, int p3, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreatePolygonRgn(p1,p2,p3));
}


HRGN _DBG_CreatePolyPolygonRgn(const POINT *p1, const INT *p2, int p3, int p4, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(CreatePolyPolygonRgn(p1,p2,p3,p4));	
}


HRGN _DBG_ExtCreateRegion(const XFORM *p1, DWORD p2, const RGNDATA *p3, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(ExtCreateRegion(p1,p2,p3));
}


HRGN _DBG_PathToRegion(HDC p1, LPCTSTR file, int line)
{
	return gdidbg_AddRegion(PathToRegion(p1));
}


/*************************************************************/
// DC

HDC _DBG_CreateCompatibleDC(HDC p1, LPCTSTR file, int line)
{
	return gdidbg_AddDC(CreateCompatibleDC(p1),true);
}


HDC _DBG_CreateDCA(LPCSTR p1, LPCSTR p2, LPCSTR p3, const DEVMODEA *p4, LPCTSTR file, int line)
{
	return gdidbg_AddDC(CreateDCA(p1,p2,p3,p4),true);
}


HDC _DBG_CreateDCW(LPCWSTR p1, LPCWSTR p2, LPCWSTR p3, const DEVMODEW *p4, LPCTSTR file, int line)
{
	return gdidbg_AddDC(CreateDCW(p1,p2,p3,p4),true);
}


HDC _DBG_CreateICA(LPCSTR p1, LPCSTR p2, LPCSTR p3, const DEVMODEA *p4, LPCTSTR file, int line)
{
	return gdidbg_AddDC(CreateICA(p1,p2,p3,p4),true);
}


HDC _DBG_CreateICW(LPCWSTR p1, LPCWSTR p2, LPCWSTR p3, const DEVMODEW *p4, LPCTSTR file, int line)
{
	return gdidbg_AddDC(CreateICW(p1,p2,p3,p4),true);
}


HDC _DBG_GetDC(HWND p1, LPCTSTR file, int line)
{
	return gdidbg_AddDC(GetDC(p1),false);
}


HDC _DBG_GetDCEx(HWND p1, HRGN p2, DWORD p3, LPCTSTR file, int line)
{
	return gdidbg_AddDC(GetDCEx(p1,p2,p3),false);
}


BOOL _DBG_DeleteDC(HDC Object, LPCTSTR file, int line)
{
	_gdidbg_DeleteObject(Object,file,line);
	return DeleteDC(Object);
}


BOOL _DBG_ReleaseDC(HWND hWnd, HDC hDc, LPCTSTR file, int line)
{
	_gdidbg_DeleteObject(hDc,file,line,true);
	return ReleaseDC(hWnd,hDc);
}


/***********************************************************************************/
// bitmap

HBITMAP _DBG_CreateBitmap(int p1, int p2, UINT p3, UINT p4, const void *p5, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateBitmap(p1,p2,p3,p4,p5));
}


HBITMAP _DBG_CreateBitmapIndirect(const BITMAP *p1,LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateBitmapIndirect(p1));
}


HBITMAP _DBG_CreateDIBitmap(HDC p1, const BITMAPINFOHEADER *p2, DWORD p3, const void *p4, const BITMAPINFO *p5, UINT p6, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateDIBitmap(p1,p2,p3,p4,p5,p6));
}


HBITMAP _DBG_CreateDIBSection(HDC p1, const BITMAPINFO * p2	, UINT p3, void ** p4, HANDLE p5, DWORD p6, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateDIBSection(p1,p2,p3,p4,p5,p6));
}


HBITMAP _DBG_CreateCompatibleBitmap(HDC p1, int p2, int p3, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateCompatibleBitmap(p1,p2,p3));
}


HBITMAP _DBG_CreateDiscardableBitmap(HDC p1, int p2, int p3, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(CreateDiscardableBitmap(p1,p2,p3));
}


HBITMAP _DBG_LoadBitmapA(HINSTANCE p1, LPCSTR p2, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(LoadBitmapA(p1,p2));
}


HBITMAP _DBG_LoadBitmapW(HINSTANCE p1, LPCWSTR p2, LPCTSTR file, int line)
{
	return gdidbg_AddBitmap(LoadBitmapW(p1,p2));
}


/***********************************************************************************/

BOOL _DBG_DeleteObject(HGDIOBJ obj,LPCTSTR file, int line)
{
	_gdidbg_DeleteObject(obj,file,line);
	
	BOOL result=DeleteObject(obj);
	if(!result)
	{
		_gdidbg_OutputDebugString("DeleteObject return FALSE :" ,file,line,true);
	}
	return result;
}


HGDIOBJ _DBG_SelectObject(HDC indc,HGDIOBJ inObj,LPCTSTR file, int line)
{
	int type=GetObjectType(inObj);
	if(type>=1 && type<=16)
	{
		HGDIOBJ cur=GetCurrentObject(indc,type);
		if(cur==inObj && sGDIDGB_AssertOnDoubleSelect)
		{
			str mess=sGDIObjDesc[type].fObjName;
			mess+=_T(" ALREADY SELECTED : ");
			_gdidbg_OutputDebugString(mess.c_str(),file,line);
		}
	}
	else
	{
		// pas cool
		_GDI_Debug_InfoMap_Iterrator it;
		long mapid;
		BOOL outAlloc;
		if(_gdidbg_GlobalFind(inObj,it,mapid,outAlloc))
		{
			if(!outAlloc)
			{
				_gdidbg_OutputDebugString("SELECT OBJECT ON DELETED OBJECT",file,line,true);
			}
			else
			{
				_gdidbg_OutputDebugString("FOUND BUT BAD OBJECT",file,line,true);
			}
		}
		else
		{
			_gdidbg_OutputDebugString("BAD OBJECT",file,line,true);
		}
	}
	return SelectObject(indc,inObj);
}


void gdidbg_SetAssertOnDelete(HGDIOBJ inObj)
{
	_GDI_Debug_InfoMap_Iterrator it;
	long mapid;
	BOOL alloc;
	if(_gdidbg_GlobalFind(inObj,it,mapid,alloc))
	{
		if(alloc)
		{
			it->second.fAssertOnDelete=1;
		}
		else
		{
			_gdidbg_OutputDebugString("OBJECT ALREADY DELETED","",0);
		}
	}
}


static void _gdidbg_EnableDisable(int objtype,BOOL inEnable) // 0 for all
{
	if(objtype==0)
	{
		for(long i=1;i<=15;i++)
		{
			
			inEnable ? sGDIObjDesc[i].fDebug=sGDIObjDesc[i].fCanDebug : sGDIObjDesc[i].fDebug=false;
		}
	}
	else if (objtype>=1 && objtype<=15)
	{
		inEnable ? sGDIObjDesc[objtype].fDebug=sGDIObjDesc[objtype].fCanDebug : sGDIObjDesc[objtype].fDebug=false;
	}
}


void gdidbg_Disable(int objtype) // 0 for all
{
	_gdidbg_EnableDisable(objtype,false);
}


void gdidbg_Enable(int objtype) // 0 for all
{
	_gdidbg_EnableDisable(objtype,true);
}


/*********************************************************************************/
// debug window
/*********************************************************************************/

_GDI_DebugDial::_GDI_DebugDial()
{
	fRes=LoadLibrary("gdidebugdll.dll");
	fDial=0;
	
	fHook=SetWindowsHookEx(WH_KEYBOARD,_GDI_DebugDial::_KeyboardProc,NULL,GetCurrentThreadId());
}


_GDI_DebugDial:: ~_GDI_DebugDial()
{
	UnhookWindowsHookEx(fHook);
	if(fRes)
		FreeLibrary(fRes);
}


void _GDI_DebugDial::OpenDial()
{
	if(!fDial && fRes)
	{
		_beginthread(_ThreadProc,0,this);
	}
	else
	{
		if(fDial)
			SetForegroundWindow(fDial) ;
	}
}


void _GDI_DebugDial::_ThreadProc(void *dummy)
{
	_GDI_DebugDial* obj=(_GDI_DebugDial*)dummy;
	if(obj)
	{
		obj->fThreadID=GetCurrentThreadId();
		obj->ThreadProc();
	}
}


void _GDI_DebugDial::ThreadProc()
{
	INITCOMMONCONTROLSEX i;
	i.dwSize=sizeof(INITCOMMONCONTROLSEX);
	i.dwICC=ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&i);
	fDial=CreateDialogParam(fRes,MAKEINTRESOURCE(102),NULL,(DLGPROC)_DlgProc,(LPARAM)this);
	if(fDial)
	{
		MSG msg;
		while(GetMessage(&msg,NULL,0,0))
		{
			if(!IsDialogMessage(fDial,&msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		fDial=0;
	}
}


int _GDI_DebugDial::_DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static _GDI_DebugDial* obj=0;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			obj=(_GDI_DebugDial*)lParam;
			break;
		}
	}
	if(obj)
	{
		return obj->DlgProc(hwndDlg,uMsg,wParam,lParam);
	}
	else
		return false;
}


void _GDI_DebugDial::_UpdateDial(long id,long count)
{
	#if !_GDI_DEBUG_DIAL_USETIMER
	if(fDial)
	{
		::PostMessage(fDial,WM_GDI_DEBUGDIAL_UPDATE,id,count);
	}
	#endif
}


void _GDI_DebugDial::_Reaffect(long id,long count)
{
	if(id==0)
	{
		for(long i=1;i<=14;i++)
		{
			fDialData[i].nbAlloc=_gGDIDebugArray[i].GetAllocCount();
			fDialData[i].nbDel=_gGDIDebugArray[i].GetDeletedCount();
		}
		ListView_RedrawItems(fList,0,14);
		InvalidateRgn(fList,NULL,false);
	}
	else
	{
		fDialData[id].nbAlloc=_gGDIDebugArray[id].GetAllocCount();
		fDialData[id].nbDel=_gGDIDebugArray[id].GetDeletedCount();
		ListView_RedrawItems(fList,id-1,id-1);
		InvalidateRgn(fList,NULL,false);
	}
	UpdateWindow(fList);
}


void _GDI_DebugDial::_InitList()
{
	int res;
	LVCOLUMN col;
	
	ListView_SetExtendedListViewStyle(fList,LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	
	col.mask=LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	col.cx = 100;
	col.fmt=LVCFMT_LEFT;
	
	col.pszText=_T("Obj");
	col.cchTextMax=strlen(col.pszText);
	res=ListView_InsertColumn(fList,0,&col);
	
	col.pszText=_T("Alloc");
	col.cchTextMax=strlen(col.pszText);
	res=ListView_InsertColumn(fList,1,&col);
	
	col.pszText=_T("Del");
	col.cchTextMax=strlen(col.pszText);
	res=ListView_InsertColumn(fList,2,&col);
	
	LVITEM it;
	long l=0;
	for(long i=1;i<=14;i++)
	{
		it.mask = LVIF_TEXT | LVIF_PARAM;
		it.pszText = LPSTR_TEXTCALLBACK;
		it.iItem=i-1;
		it.iSubItem=0;
		it.lParam=(LPARAM)i;
		
		l=ListView_InsertItem(fList,&it);
		ListView_SetCheckState(fList,i-1,sGDIObjDesc[i].fDebug);
		it.iSubItem=1;
		ListView_SetItem(fList,&it);
		it.iSubItem=2;
		ListView_SetItem(fList,&it);
	}
	fListInited=true;
}
int _GDI_DebugDial::DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	BOOL result=false;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			fListInited=false;
			::ShowWindow(hwndDlg,SW_SHOWNORMAL);
			CheckDlgButton(hwndDlg,2000,1);
			
			CheckDlgButton(hwndDlg,2005,sGDIDGB_AssertOnDeleteStockObject);
			CheckDlgButton(hwndDlg,2004,sGDIDGB_KeepDeletedObject);
			CheckDlgButton(hwndDlg,2003,sGDIDGB_AssertOnDoubleSelect);
			
			result=true;
			fList=GetDlgItem(hwndDlg,2001);
			_InitList();
			#if _GDI_DEBUG_DIAL_USETIMER
			SetTimer(hwndDlg,666,_GDI_DEBUG_DIAL_TIMERFREQ,NULL);
			#endif
			break;
		}
		case WM_GDI_DEBUGDIAL_SYNCLISTCHECK:
		{
			for(long i=1;i<=14;i++)
			{
				ListView_SetCheckState(fList,i-1,sGDIObjDesc[i].fDebug);
			}
			result=true;
			break;
		}
		case WM_TIMER:
		{
			_Reaffect(0,0);
			result=true;
			break;
		}
		case WM_SHOWWINDOW:
		{
			if(!wParam)
			{
				//::ShowWindow(hwndDlg,SW_SHOWNORMAL);
			}
			result=true;
			break;
		}
		case WM_CLOSE:
		{
			#if _GDI_DEBUG_DIAL_USETIMER
			KillTimer(hwndDlg,666);
			#endif
			EndDialog(hwndDlg,1);
			PostQuitMessage(0);
			result=true;
			break;
		}
		case WM_DESTROY:
		{
			
			break;
		}
		case WM_GDI_DEBUGDIAL_UPDATE:
		{
			_Reaffect(wParam,lParam);
			result=true;
			break;
		}
		case WM_COMMAND:
		{
			long u=LOWORD(wParam);
			switch(u)
			{
				case 2005:// delete stock
				{
					sGDIDGB_AssertOnDeleteStockObject=IsDlgButtonChecked(hwndDlg,u);
					break;
				}
				case 2004:// keep del
				{
					sGDIDGB_KeepDeletedObject=IsDlgButtonChecked(hwndDlg,u);
					break;
				}
				case 2003:// double select
				{
					sGDIDGB_AssertOnDoubleSelect=IsDlgButtonChecked(hwndDlg,u);
					break;
				}
				case 2002:// clear
				{
					_gdidbg_ClearMap();
					break;
				}
				case 2000: // on top
				{
					if(IsDlgButtonChecked(hwndDlg,2000))
					{
						SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
					}
					else
					{
						SetWindowPos(hwndDlg,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
					}
					break;
				}
			}
			break;
		}
		case WM_NOTIFY:
		{
			static TCHAR cc[255];
			LPNMHDR pnmh = (LPNMHDR) lParam; 
			if(pnmh->idFrom=2001)
			{
				switch(pnmh->code)
				{
					case LVN_ITEMCHANGING:
					{
						if(fListInited)
						{
							NMLISTVIEW *nh=(NMLISTVIEW*)lParam; 
							if(nh->uChanged  & LVIF_STATE)
							{
								if(!sGDIObjDesc[nh->lParam].fCanDebug)
								{
									SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT ,1);
									return true;
								}
							}
						}
						SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,0);
						return true;
						break;
					}
					case LVN_ITEMCHANGED:
					{
						if(fListInited)
						{
							NMLISTVIEW *nh=(NMLISTVIEW*)lParam; 
							if(nh->uChanged  & LVIF_STATE)
							{
								if(nh->uNewState!=nh->uOldState)
								{
									if(nh->uNewState==0x00002000)
									{
										gdidbg_Enable(nh->lParam);
									}
									else if(nh->uNewState==0x00001000)
									{
										gdidbg_Disable(nh->lParam);
									}
								}
							}
						}
						break;
					}
					case LVN_GETDISPINFO:
					{
						NMLVDISPINFO* nh=(NMLVDISPINFO*)lParam; 
						
						if (nh->item.mask & LVIF_TEXT)
						{
							switch(nh->item.iSubItem)
							{
								case 0:
								{
									nh->item.mask =LVIF_DI_SETITEM | LVIF_TEXT;
									nh->item.pszText =sGDIObjDesc[nh->item.lParam].fObjName;
									nh->item.cchTextMax = strlen(sGDIObjDesc[nh->item.lParam].fObjName);
									break;
								}
								case 1:
								{
									_ltot(fDialData[nh->item.lParam].nbAlloc,cc,10);
									nh->item.mask =LVIF_TEXT;
									nh->item.pszText =cc;
									nh->item.cchTextMax = strlen(cc);
									break;
								}
								case 2:
								{
									_ltot(fDialData[nh->item.lParam].nbDel,cc,10);
									nh->item.mask =LVIF_TEXT;
									nh->item.pszText =cc;
									nh->item.cchTextMax = strlen(cc);
									break;
								}
							}
						}
						break;
					}
				}
			}
			break;
		}
	}
	return result;
}


LRESULT CALLBACK _GDI_DebugDial::_KeyboardProc(int code,WPARAM wParam,LPARAM lParam)
{
	if(wParam==VK_F12)
	{
		if((lParam>>16) & KF_UP)
	
		{
			if((::GetAsyncKeyState(VK_CONTROL)& 0x8000)!=0)
			{
				sGDIDebugDial.OpenDial();
			}
			if((::GetAsyncKeyState(VK_SHIFT)& 0x8000)!=0)
			{
				gdidbg_Disable(0);
				sGDIDebugDial._SyncListCheck();
			}
		}
	}
	return CallNextHookEx(_GDI_DebugDial::fHook,code,wParam,lParam);
}

#endif

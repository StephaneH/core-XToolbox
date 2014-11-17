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
#include "VIcon.h"

#if 0
VIcon::VIcon (const VFile& inResFile, sLONG inID)
{
	_Init();
	
	HINSTANCE	instance = VProcess::WIN_GetAppInstance();
	fData16 = ::LoadIcon(instance, MAKEINTRESOURCE(inID));
}

#endif

VIcon::VIcon (const VResourceFile& inResFile, sLONG inID)
{
	_Init();

	VStr63	resType('#');
	resType.AppendLong(PtrToUlong( RT_GROUP_ICON));

	VHandle	groupResHandle = inResFile.GetResource(resType, inID);
	if (groupResHandle != NULL)
	{
		uBYTE*	resdata = (uBYTE*) VMemory::LockHandle(groupResHandle);
		DWORD	nID = ::LookupIconIdFromDirectoryEx(resdata, TRUE, 0, 0, LR_DEFAULTCOLOR);
		
		resType.Clear();
		resType.AppendUniChar('#');
		resType.AppendLong(PtrToUlong( RT_ICON));
		
		VHandle	iconResHandle = inResFile.GetResource(resType, nID);
		if (iconResHandle != NULL)
		{
			FromResource(iconResHandle);
			VMemory::DisposeHandle(iconResHandle);
		}
		
		VMemory::DisposeHandle(groupResHandle);
	}
}


VIcon::VIcon (const VResourceFile& inResFile, const VString& inName)
{
	_Init();

	VStr63	resType('#');
	resType.AppendLong(PtrToUlong( RT_GROUP_ICON));

	VHandle	groupResHandle = inResFile.GetResource(resType, inName);
	if (groupResHandle != NULL)
	{
		uBYTE*	resdata = (uBYTE*) VMemory::LockHandle(groupResHandle);
		DWORD	nID = ::LookupIconIdFromDirectoryEx(resdata, TRUE, 16, 16, LR_DEFAULTCOLOR);
		
		resType.Clear();
		resType.AppendUniChar('#');
		resType.AppendLong(PtrToUlong( RT_ICON));
		
		VHandle	iconResHandle = inResFile.GetResource(resType, nID);
		if (iconResHandle != NULL)
		{
			FromResource(iconResHandle);
			VMemory::DisposeHandle(iconResHandle);
		}
		
		VMemory::DisposeHandle(groupResHandle);
	}
}

	COLORREF crMask = CLR_DEFAULT;
	uWORD	uFlags = LR_DEFAULTCOLOR;
	uWORD	iFlags = ILD_NORMAL;

VIcon::VIcon (const VString& inIconPath, sLONG inID, const sLONG inType) {

	HRSRC		rsrc = NULL;
	VString		IconName;
	uWORD		uType = 0;

	_Init();
	VStringConvertBuffer IconPathBuff( inIconPath, VTC_Win32Ansi);
	IconName.FromCString( "#");
	IconName.AppendLong( inID );
	VStringConvertBuffer IconNameBuff( IconName, VTC_Win32Ansi);

	HINSTANCE   AxLib = LoadLibrary(IconPathBuff.GetCPointer());
	switch (inType) {
		case RT_BITMAP	: uType = IMAGE_BITMAP;break;
		case RT_CURSOR	: uType = IMAGE_CURSOR;break;
		case RT_ICON	:
		default			: uType = IMAGE_ICON;break;
	}

	HIMAGELIST  ImageList = ImageList_LoadImage(AxLib,MAKEINTRESOURCE(inID),16,1,crMask,uType,uFlags);
	
	if (ImageList) {
		int cx,cy;
		VHandle iconResHandle = NULL;
		ImageList_GetIconSize(ImageList,&cx,&cy);
		if (cx <= 16 && cy <= 16) {
			HICON hIcon = ImageList_GetIcon(ImageList,0,uFlags);
			fData16 = hIcon;
			fData32 = ::CopyIcon((HICON)fData16);
			fData48 = ::CopyIcon((HICON)fData16);
		}
		ImageList_Destroy(ImageList);
	}
	FreeLibrary(AxLib);
}


VIcon::VIcon (const VIcon& inOriginal)
{
	_Init();
	
	fState = inOriginal.fState;
	fHorizAlign = inOriginal.fHorizAlign;
	fVertAlign = inOriginal.fVertAlign;
	fResID = inOriginal.fResID;
	
	if (inOriginal.fData16 != NULL)
		fData16 = ::CopyIcon((HICON)inOriginal.fData16);
	
	if (inOriginal.fData32 != NULL)
		fData32 = ::CopyIcon((HICON)inOriginal.fData32);

	if (inOriginal.fData48 != NULL)
		fData48 = ::CopyIcon((HICON)inOriginal.fData48);
}


VIcon::VIcon (SystemIcon inSystemIcon)
{
	_Init();
	
	switch (inSystemIcon)
	{
		case kICON_TRASH:
			::ExtractIconEx("SHELL32.DLL", 31, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_HELP:
			fData16 = ::LoadIcon(NULL, IDI_QUESTION); 
			break;
			
		case kICON_LOCKED:
			::ExtractIconEx("SHELL32.DLL", 47, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_UNLOCKED:
			fData16 = ::LoadIcon(NULL, IDI_ERROR); 
			break;
			
		case kICON_WRITE_PROTECTED:
			fData16 = ::LoadIcon(NULL, IDI_ERROR); 
			break;
			
		case kICON_BACK:
			fData16 = ::LoadIcon(NULL, IDI_ERROR); 
			break;
			
		case kICON_FORWARD:
			fData16 = ::LoadIcon(NULL, IDI_ERROR); 
			break;
			
		case kICON_DISK:
			::ExtractIconEx("SHELL32.DLL", 8, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_CLOSED_FOLDER:
			::ExtractIconEx("SHELL32.DLL", 3, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_OPENED_FOLDER:
			::ExtractIconEx("SHELL32.DLL", 4, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_SHARED_FOLDER:
			::ExtractIconEx("SHELL32.DLL", 4, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_FLOPPY:
			::ExtractIconEx("SHELL32.DLL", 6, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_GENERIC_FILE:
			::ExtractIconEx("SHELL32.DLL", 1, NULL,(HICON*)&fData16, 1);
			break;
			
		case kICON_CDROM:
			::ExtractIconEx("SHELL32.DLL", 11, NULL,(HICON*)&fData16, 1);
			break;
		
		default :
			assert(false);
			break;
	}
}


VIcon::VIcon (const VFile& inForFile)
{
	_Init();
}


VIcon::VIcon (const VFolder& inForFolder)
{
	_Init();
}


VIcon::~VIcon ()
{
	if (fData16 != NULL)
		::DestroyIcon((HICON)fData16);
		
	if (fData32 != NULL)
		::DestroyIcon((HICON)fData32);

	if (fData48 != NULL)
		::DestroyIcon((HICON)fData48);
}


void VIcon::_Init ()
{
	fData16 = NULL;
	fData32 = NULL;
	fData48 = NULL;
	fState = PS_NORMAL;
	fHorizAlign = AL_CENTER;
	fVertAlign = AL_CENTER;
	fResID = -1;
}


Boolean VIcon::IsValid() const
{
	return (fData16 != NULL);
}


void VIcon::SetPosBy (GReal inHoriz, GReal inVert)
{
	fBounds.SetPosBy(inHoriz, inVert);
}


void VIcon::SetSizeBy (GReal inHoriz, GReal inVert)
{
	fBounds.SetSizeBy(inHoriz, inVert);
}

void VIcon::Set16Only ()
{
	if (fData32 != NULL)
		::DestroyIcon((HICON)fData32);

	if (fData48 != NULL)
		::DestroyIcon((HICON)fData48);
	fData32 = ::CopyIcon((HICON)fData16);
	fData48 = ::CopyIcon((HICON)fData16);
}

Boolean VIcon::HitTest (const VPoint& inPoint) const
{
	// Should use GETICONINFO add test for icon mask
	return fBounds.HitTest(inPoint);
}


void VIcon::Inset (GReal inHoriz, GReal inVert)
{
	fBounds.Inset(inHoriz, inVert);
}


void VIcon::FromResource (VHandle inIconHandle)
{
	if (fData16 != NULL)
	{
		::DestroyIcon((HICON)fData16);
		fData16 = NULL;
	}

	if (fData32 != NULL)
	{
		::DestroyIcon((HICON)fData32);
		fData32 = NULL;
	}

	if (fData48 != NULL)
	{
		::DestroyIcon((HICON)fData48);
		fData48 = NULL;
	}

	if (inIconHandle != NULL)
	{
		PBYTE	resBits = (PBYTE) VMemory::LockHandle(inIconHandle);
		DWORD	size = (DWORD) VMemory::GetHandleSize(inIconHandle);

		// Should check for available sizes
		fData16 = ::CreateIconFromResourceEx(resBits, size, true, 0x00030000, 16, 16, LR_DEFAULTCOLOR);
		fData32 = ::CreateIconFromResourceEx(resBits, size, true, 0x00030000, 32, 32, LR_DEFAULTCOLOR);
		fData48 = ::CreateIconFromResourceEx(resBits, size, true, 0x00030000, 48, 48, LR_DEFAULTCOLOR);
	
		VMemory::UnlockHandle(inIconHandle);
	}
}


VIcon::operator const HICON& () const
{
	// Returns the greatest available size mathing current size
	if (fBounds.GetWidth() >= 48 && fData48 != NULL)
	{
		return (*(HICON*) &fData48);
	}
	else if (fBounds.GetWidth() >= 32 && fData32 != NULL)
	{
		return (*(HICON*) &fData32);
	}
	else
	{
		return (*(HICON*) &fData16);
	}
}



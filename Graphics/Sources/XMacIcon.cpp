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


VIcon::VIcon(const VResourceFile& inResFile, sLONG inID)
{
	_Init();
	
	VHandle	resource = inResFile.GetResource( CVSTR( "icns"), inID);
	
	FromResource(resource);
	fResID = inID;
}


VIcon::VIcon(const VResourceFile& inResFile, const VString& inName)
{
	_Init();
	
	VHandle	resource = inResFile.GetResource( CVSTR( "icns"), inName);
	
	FromResource(resource);
	inResFile.GetResourceID( CVSTR( "icns"), inName, &fResID);
}


VIcon::VIcon(const VString& /*inIconPath*/, sLONG /*inID*/,  const sLONG /*inType*/)
{
	assert(false);	// Available on Windows
}


VIcon::VIcon(const VIcon& inOriginal)
{
	fState = inOriginal.fState;
	fHorizAlign = inOriginal.fHorizAlign;
	fVertAlign = inOriginal.fVertAlign;
	fResID = inOriginal.fResID;
	
	// Increment native data's refcount
	fData = inOriginal.fData;
	::AcquireIconRef((IconRef)fData);
}


VIcon::VIcon(SystemIcon inSystemIcon)
{
	_Init();
	
	OSType	iconType;
	
	switch (inSystemIcon)
	{
		case kICON_TRASH:
			iconType = kTrashIcon;
			break;
			
		case kICON_HELP:
			iconType = kHelpIcon;
			break;
			
		case kICON_LOCKED:
			iconType = kLockedIcon;
			break;
			
		case kICON_UNLOCKED:
			iconType = kUnlockedIcon;
			break;
			
		case kICON_WRITE_PROTECTED:
			iconType = kNoWriteIcon;
			break;
			
		case kICON_GRID:
			iconType = kGridIcon;
			break;
			
		case kICON_SORT_ASCENDENT:
			iconType = kSortAscendingIcon;
			break;
			
		case kICON_SORT_DESCENDENT:
			iconType = kSortDescendingIcon;
			break;
			
		case kICON_BACK:
			iconType = kBackwardArrowIcon;
			break;
			
		case kICON_FORWARD:
			iconType = kForwardArrowIcon;
			break;
			
		case kICON_DISK:
			iconType = kGenericHardDiskIcon;
			break;
			
		case kICON_CLOSED_FOLDER:
			iconType = kGenericFolderIcon;
			break;
			
		case kICON_OPENED_FOLDER:
			iconType = kOpenFolderIcon;
			break;
			
		case kICON_SHARED_FOLDER:
			iconType = kSharedFolderIcon;
			break;
			
		case kICON_FLOPPY:
			iconType = kGenericFloppyIcon;
			break;
			
		case kICON_GENERIC_FILE:
			iconType = kGenericDocumentIcon;
			break;
			
		case kICON_CDROM:
			iconType = kGenericCDROMIcon;
			break;
		
		default :
			DebugMsg("Unknown Icon Type in VIcon");
			iconType = 0;
			break;
	}
	
	::GetIconRef(kOnSystemDisk, kSystemIconsCreator, iconType, (IconRef*) &fData);
}


VIcon::VIcon(const VFile& inForFile)
{
	_Init();
	
	FSRef	fsref;
	inForFile.MAC_GetFSRef( &fsref);
	
	OSStatus error = ::GetIconRefFromFileInfo( &fsref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, (IconRef*) &fData, NULL);
}


VIcon::VIcon(const VFolder& inForFolder)
{
	_Init();
	
	FSRef	fsref;
	inForFolder.MAC_GetFSRef( &fsref);
	
	OSStatus error = ::GetIconRefFromFileInfo( &fsref, 0, NULL, kFSCatInfoNone, NULL, kIconServicesNormalUsageFlag, (IconRef*) &fData, NULL);
	
	// No need to call GetIconRefFromFolder if we don't know its attributes
	//OSErr	error = ::GetIconRefFromFolder(inForFolder.GetRefNum(), ..., (IconRef*) &fData, &label);
}



VIcon::~VIcon()
{
	::ReleaseIconRef((IconRef)fData);
}


void VIcon::_Init()
{
	fData = NULL;
	fState = PS_NORMAL;
	fHorizAlign = AL_CENTER;
	fVertAlign = AL_CENTER;
	fResID = -1;
}


Boolean VIcon::IsValid() const
{
	return ::IsValidIconRef((IconRef)fData);
}


void VIcon::SetPosBy(GReal inHoriz, GReal inVert)
{
	fBounds.SetPosBy(inHoriz, inVert);
}


void VIcon::SetSizeBy(GReal inHoriz, GReal inVert)
{
	fBounds.SetSizeBy(inHoriz, inVert);
}


void VIcon::Set16Only() 
{
	assert(false); // Windows Only method J.A 7/10/2003
}


Boolean VIcon::HitTest(const VPoint& inPoint) const
{
	CGPoint pt=inPoint;
	CGRect bounds=fBounds;
	
	return IconRefContainsCGPoint(&pt,&bounds,kAlignNone,kIconServicesNormalUsageFlag,(IconRef) fData);
	
}


void VIcon::Inset(GReal inHoriz, GReal inVert)
{
	fBounds.Inset(inHoriz, inVert);
}


IconAlignmentType VIcon::MAC_GetAlignmentType() const
{
	IconAlignmentType	alignment = kAlignNone;
	
	switch (fHorizAlign)
	{
		case AL_CENTER:
			alignment |= kAlignHorizontalCenter;
			break;
			
		case AL_LEFT:
			alignment |= kAlignLeft;
			break;
			
		case AL_RIGHT:
			alignment |= kAlignRight;
			break;
	}
	
	switch (fVertAlign)
	{
		case AL_CENTER:
			alignment |= kAlignVerticalCenter;
			break;
			
		case AL_TOP:
			alignment |= kAlignTop;
			break;
			
		case AL_BOTTOM:
			alignment |= kAlignBottom;
			break;
	}
	
	return alignment;
}


IconTransformType VIcon::MAC_GetTransformType() const
{
	IconTransformType	transform = kTransformNone;
	
	switch (fState)
	{
		case PS_DISABLED:
			transform = kTransformDisabled;
			break;
			
		case PS_SELECTED:
			transform = kTransformSelected;
			break;
		
		/*	
		case PS_OFFLINE:
			transform = kTransformOffline;
			break;
			
		case PS_OPEN:
			transform = kTransformOpen;
			break;
			
		case PS_DISABLED_SELECTED:
			transform = kTransformSelectedDisabled;
			break;
			
		case PS_OFFLINE_SELECTED:
			transform = kTransformSelectedOffline;
			break;
			
		case PS_OPEN_SELECTED:
			transform = kTransformSelectedOpen;
			break;
		*/
	}
	
	return transform;
}


void VIcon::FromResource(VHandle inIconHandle)
{
	if (fData != NULL)
	{
		::ReleaseIconRef((IconRef)fData);
		fData = NULL;
	}
	
	if (inIconHandle == NULL) return;
	
	// Copy resource in a mac Handle
	Handle	macHandle = ::NewHandle(VMemory::GetHandleSize(inIconHandle));
	
	VMemory::CopyBlock(VMemory::LockHandle(inIconHandle), *macHandle, ::GetHandleSize(macHandle));
	VMemory::UnlockHandle(inIconHandle);
	
	// Register icon data
	static uLONG	sIconID = 0;
	::RegisterIconRefFromIconFamily(kXTOOLBOX_SIGNATURE, (OSType) ++sIconID, (IconFamilyHandle)macHandle, (IconRef*)&fData);
	::DisposeHandle(macHandle);
	
	fResID = -1;
}


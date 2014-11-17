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
#include "VResource.h"
#include "VMemory.h"
#include "VTask.h"


VResourceFile::VResourceFile()
{
}


VResourceFile::~VResourceFile()
{
}


Boolean VResourceFile::Init()
{
/*
#if USE_MACRESFILES
	::InitMacResMgr();
#endif

	gResFile = VProcess::Get()->RetainResourceFile();
	
#if VERSIONMAC
	gMacResFile = gResFile;
	gMacResFile->Retain();
#else
	// on essaie appli.rsr puis %appli.rsrc (perforce)
	VFilePath rsrcPath = gProcess->GetFile()->GetPath();
	rsrcPath.SetExtension(VString(L"rsr", -1));
	
	gMacResFile = new VMacRsrcFork(rsrcPath, FA_READ, false);
	if (!gMacResFile->IsValid())
	{
		delete gMacResFile;
		
		VStr255 name;
		rsrcPath.GetName(name);
		name.Insert(CHAR_PERCENT_SIGN, 1);
		rsrcPath.SetName(name);
		rsrcPath.SetExtension(VString(L"rsrc", -1));
		gMacResFile = new VMacRsrcFork(rsrcPath, FA_READ, false);
		if (!gMacResFile->IsValid())
		{
			delete gMacResFile;
			gMacResFile = NULL;
		}
	}
#endif

	return (gMacResFile != NULL && gResFile != NULL);
*/
	return true;
}


void VResourceFile::DeInit()
{
/*
	if (gResFile != NULL)
	{
		gResFile->Release();
		gResFile = NULL;
	}
	
	if (gMacResFile != NULL)
	{
		gMacResFile->Release();
		gMacResFile = NULL;
	}
	
#if USE_MACRESFILES
	::DeInitMacResMgr();
#endif
*/
}

#if WITH_VMemoryMgr
VError VResourceFile::WriteResourceHandle(const VString& inType, sLONG inID, const VHandle& inData, const VString *inName)
{
	VError err = VE_OK;

	sLONG size = (sLONG) VMemory::GetHandleSize(inData);
	if (size == 0)
	{
		err = WriteResource(inType, inID, NULL, 0, inName);
	}
	else
	{
		VPtr p = VMemory::LockHandle(inData);
		if (testAssert(p != NULL))
			err = WriteResource(inType, inID, p, size, inName);
		else
			err = VE_CANNOT_LOCK_HANDLE;
		VMemory::UnlockHandle(inData);
	}
	return err;
}


VError VResourceFile::AddResourceHandle(const VString& inType, const VHandle& inData, const VString *inName, sLONG *outID)
{
VError err = VE_OK;

	sLONG size = (sLONG) VMemory::GetHandleSize(inData);
	if (size == 0)
	{
		err = AddResource(inType, NULL, 0, inName, outID);
	}
	else
	{
		VPtr p = VMemory::LockHandle(inData);
		if (testAssert(p != NULL))
		{
			err = AddResource(inType, p, (sLONG) VMemory::GetHandleSize(inData), inName, outID);
		} else
			err = VE_CANNOT_LOCK_HANDLE;
		VMemory::UnlockHandle(inData);
	}
	return err;
}
#endif

void VResourceFile::DoOnRefCountZero()
{
	if (IsValid())
		Close();
	IRefCountable::DoOnRefCountZero();
}


#if WITH_VMemoryMgr
VError VResourceFile::CopyResourcesInto(VResourceFile& inDestination) const
{
	VError err = VE_OK;

	if (!IsValid() || !inDestination.IsValid())
		err = VE_STREAM_NOT_OPENED;
	else
	{
		VArrayString types;
		err = GetResourceListTypes(types);
		if (testAssert(err == VE_OK))
		{
			for (sLONG i = types.GetCount() ; i > 0 ; --i)
			{
				VStr255 theType;
				types.GetString(theType, i);
				VArrayLong ids;
				err = GetResourceListIDs(theType, ids);
				if (testAssert(err == VE_OK))
				{
					DebugMsg("copying %d ressources of type '%S'\n", ids.GetCount(), &theType);
					for (sLONG j = ids.GetCount() ; (j > 0) && (err == VE_OK) ; --j)
					{
					#if VERSIONDEBUG
						//if (VEventMgr::IsCapsLockKeyDown())
							break;
					#endif
						sLONG id = ids.GetLong(j);
						
						VHandle h = GetResource(theType, id);

						VStr255 theName;
						err = GetResourceName(theType, id, theName);

					#if VERSIONDEBUG
						sLONG size = 0;
						if (testAssert( GetResourceSize(theType, id, &size) == VE_OK))
							xbox_assert(size == VMemory::GetHandleSize(h));
					#endif
						
						if (testAssert(h != NULL))
						{
							err = inDestination.WriteResourceHandle(theType, id, h, &theName);
							assert(err == VE_OK);
							VMemory::DisposeHandle(h);
						}

						VTask::Yield();
					}
				}
			}
			DebugMsg("Done !!\n");
		}
	}
	return err;
}
#endif


Boolean VResourceFile::GetStringList(sLONG inID, VArrayString& outStrings) const
{
	VString s;
	outStrings.SetCount(0);
	for (sLONG i = 1 ;; ++i)
	{
		if (!GetString(s, inID, i))
			break;
		
		outStrings.AppendString(s);
	}
	return true;
}


Boolean VResourceFile::GetStringList(const VString& inName, VArrayString& outStrings) const
{
	sLONG id;
	if (GetResourceID(VResTypeString('STR#'), inName, &id) != VE_OK)
		return false;
	return GetStringList(id, outStrings);
}


void VResourceFile::Flush()
{
}


#pragma mark-

VClassicResourceIterator::VClassicResourceIterator(VResourceFile *inResFile, const VString& inType)
{
	fType.FromString(inType);
	inResFile->GetResourceListIDs(fType, fIDs);
	fResFile = inResFile;
}


VHandle VClassicResourceIterator::Current() const
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return NULL;
	return fResFile->GetResource(fType, fIDs.GetLong(fPos));
}


VHandle VClassicResourceIterator::First()
{
	if (fIDs.GetCount() <= 0)
	{
		fPos = 0;
		return NULL;
	}
	
	fPos = 1;
	return fResFile->GetResource(fType, fIDs.GetLong(fPos));
}


sLONG VClassicResourceIterator::GetCurrentResourceID()
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return 0;
	return fIDs.GetLong(fPos);
}


void VClassicResourceIterator::GetCurrentResourceName(VString& outName)
{
	if (testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		fResFile->GetResourceName(fType, fIDs.GetLong(fPos), outName);
	else
		outName.Clear();
}


void VClassicResourceIterator::SetPos(sLONG inPos)
{
	if (testAssert(inPos >= 1 && inPos <= fIDs.GetCount()))
		fPos = inPos;
}


VHandle VClassicResourceIterator::Previous()
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return NULL;
	if (--fPos <= 0)
		return NULL;
	return fResFile->GetResource(fType, fIDs.GetLong(fPos));
}


VHandle VClassicResourceIterator::Next()
{
	assert(fPos <= fIDs.GetCount() );

	if (fPos < fIDs.GetCount() )
		return fResFile->GetResource(fType, fIDs.GetLong(++fPos));
	
	return NULL;
}


VHandle VClassicResourceIterator::Last()
{
	fPos = fIDs.GetCount();
	return (fPos > 0) ? fResFile->GetResource(fType, fIDs.GetLong(fPos)) : NULL;
}


sLONG VClassicResourceIterator::GetPos() const
{
	return fPos;
}


#if USE_OBSOLETE_API

void VResourceFile::MakeString(VString& xstr,PString pstr)
{
	xbox_assert( false);
	#if 0
	sWORD		comma;
	sLONG 		resid;
	sLONG		index;
	
	if (pstr[1]==':')
	{
		for (comma = 1;pstr[comma]!=',' && comma<=pstr[0];comma++)
		{
		}
		if (comma>pstr[0])
		{
			xstr.FromBlock(pstr + 2, pstr[0]-1, VTC_MacOSAnsi);
			resid = xstr.GetLong();
			index = 0;
		}
		else
		{
			xstr.FromBlock(pstr + 2, comma-2, VTC_MacOSAnsi);
			resid = xstr.GetLong();
			xstr.FromBlock(pstr + comma+1, pstr[0]-comma, VTC_MacOSAnsi);
			index = xstr.GetLong();
		}
		GetString (xstr, resid, index);
	}
	else xstr.FromBlock(&pstr[1],pstr[0],VTC_MacOSAnsi);
	#endif
}

#endif
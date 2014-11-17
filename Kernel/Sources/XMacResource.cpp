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

#if !__LP64__

#include "XMacResource.h"
#include "VMemory.h"
#include "VProcess.h"
#include "VFolder.h"
#include "VError.h"
#include "VBlob.h"
#include "xmacfile.h"


VMacResFile::VMacResFile(sWORD inMacRefNum)
{
	fUseResourceChain = false;
	fRefNum = inMacRefNum;
	fCloseFile = false;
	fLastStringList = NULL;
	fLastStringListID = 0;
	fReadOnly = ((::GetResFileAttrs(fRefNum) & mapReadOnly) != 0);
}

	
VMacResFile::VMacResFile(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	fUseResourceChain = false;
	fRefNum = -1;
	fReadOnly = true;
	fLastStringList = NULL;
	fLastStringListID = 0;
	
	_Open(inPath, inFileAccess, inCreate);
}


VMacResFile::~VMacResFile()
{
	fLastStringList = NULL;
	if (fRefNum != -1)
	{
	 	if (fCloseFile) 
			::CloseResFile(fRefNum);
		else if (!fReadOnly)
			::UpdateResFile(fRefNum);
	}
}


bool VMacResFile::GetResource( const VString& inType, sLONG inID, VBlob& outData) const
{
	outData.Clear();
	bool ok = false;
	Handle data = NULL;
	VError error = _GetResource( inType, inID, &data);
	if (error == VE_OK)
	{
		::LoadResource(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			assert(*data != NULL);
			::HLock(data);
			ok = outData.PutData( *data, GetHandleSize( data)) == VE_OK;
			::HUnlock(data);
			data = NULL;
		}
	}
	return ok;
}


VHandle VMacResFile::GetResource(const VString& inType, sLONG inID) const
{
	VHandle theData = NULL;
	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK)
	{
		::LoadResource(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			assert(*data != NULL);
			::HLock(data);
			theData = VMemory::NewHandleFromData(*data, ::GetHandleSize(data));
			::HUnlock(data);
			data = NULL;
		}
	}
	return theData;
}


VHandle VMacResFile::GetResource(const VString& inType, const VString& inName) const
{
	VHandle theData = NULL;
	Handle data = NULL;
	VError error = _GetResource(inType, inName, &data);
	if (error == VE_OK)
	{
		::LoadResource(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			assert(*data != NULL);
			::HLock(data);
			theData = VMemory::NewHandleFromData(*data, ::GetHandleSize(data));
			::HUnlock(data);
			data = NULL;
		}
	}
	return theData;
}


Boolean VMacResFile::ResourceExists(const VString& inType, sLONG inID) const
{
	Boolean isOK = false;

	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	return (error == VE_OK);
}


Boolean VMacResFile::ResourceExists(const VString& inType, const VString& inName) const
{
	Boolean isOK = false;

	Handle data = NULL;
	VError error = _GetResource(inType, inName, &data);
	return (error == VE_OK);
}


VError VMacResFile::GetResourceSize(const VString& inType, sLONG inID, sLONG* outSize) const
{
	*outSize = 0;
	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK)
	{
		sLONG size = ::GetResourceSizeOnDisk(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			*outSize = size;
		} else
			error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	return error;
}


VError VMacResFile::GetResourceSize(const VString& inType, const VString& inName, sLONG* outSize) const
{
	*outSize = 0;
	Handle data = NULL;
	VError error = _GetResource(inType, inName, &data);
	if (error == VE_OK)
	{
		sLONG size = ::GetResourceSizeOnDisk(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			*outSize = size;
		} else
			error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	return error;
}

	
VError VMacResFile::WriteResource(const VString& inType, sLONG inID, const void* inData, sLONG inDataLen, const VString* inName)
{
	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;

	// test if the res file is not read-only
	if (!testAssert(!fReadOnly))
		return VE_ACCESS_DENIED;
	
	if (!testAssert(inID >= -32768 && inID <= 32767))
		return VE_INVALID_PARAMETER;

	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK || error == VE_RES_NOT_FOUND)
	{
		error = VE_OK;
		OSErr	macError = noErr;
		Str255	spName;
	
		sWORD	id = (short) inID;
		spName[0] = 0;
		if (inName != NULL)
			inName->MAC_ToMacPString(spName, 255);
	
		if ((data != NULL) && (*data != NULL))
		{
			::HNoPurge(data);
			::EmptyHandle(data);	// discard old data
		}
	
		if (data != NULL)
		{
			// Change existing resource

			if (inName != NULL)
				::SetResInfo(data, id, spName);
			
			// if data size is small, we delay the writing to disk to improve flush speed
			// else we write it immediately and we don't keep it in memory
			if (inDataLen > 4096L)
			{
				::SetResourceSize(data, inDataLen);
				macError = ::ResError();
				if (testAssert(macError == noErr))
				{
					::WritePartialResource(data, 0, inData, inDataLen);
					macError = ::ResError();
					assert(macError == noErr);
				}
			}
			else
			{
				::ReallocateHandle(data, inDataLen);
				macError = MemError();
				if (testAssert(macError == noErr))
				{
					::BlockMoveData(inData, *data, inDataLen);
					::ChangedResource(data);
					::WriteResource(data);
					::ReleaseResource(data);
					data = NULL;
					macError = ::ResError();
					assert(macError == noErr);
				}
			}

			if (data != NULL)
			{
				::ReleaseResource(data);
				data = NULL;
			}
		}
		else
		{
			// Add a new resource

			sWORD	curres = ::CurResFile();
			if (curres != fRefNum)
				::UseResFile(fRefNum);
			
			if (inDataLen > 4096L)
			{
				data = ::NewHandle(0); // AddResource needs a non-empty handle
				::AddResource(data, inType.GetOsType(), id, spName);
				macError = ::ResError();
				if (testAssert(macError == noErr))
				{
					::EmptyHandle(data);
					::SetResourceSize(data, inDataLen);
					macError = ::ResError();
					if (testAssert(macError == noErr))
					{
						::WritePartialResource(data, 0, inData, inDataLen);
						macError = ::ResError();
						assert(macError == noErr);
					}
					::ReleaseResource(data);
					data = NULL;
				}
			}
			else
			{
				data = ::NewHandle(inDataLen);
				macError = ::MemError();
				if (testAssert(macError == noErr))
				{
					::BlockMoveData(inData, *data, inDataLen);
					::AddResource(data, inType.GetOsType(), id, spName);
					macError = ::ResError();
					if (testAssert(macError == noErr))
					{
						::WriteResource(data);
						macError = ::ResError();
						assert(macError == noErr);
						::ReleaseResource(data);
					}
					else
					{
						::DisposeHandle(data);
					}
					data = NULL;
				}
			}
			if (data != NULL)
			{
				::DisposeHandle(data);
				data = NULL;
			}
	
			if (curres != fRefNum)
				::UseResFile(curres);
		}
		error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	
	return error;
}

	
VError VMacResFile::AddResource(const VString& inType, const void* inData, sLONG inDataLen, const VString* inName, sLONG* outID)
{
	VError error = VE_OK;

	*outID = 0;

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	ResType type = inType.GetOsType();
	if (type == 0)
		return VE_INVALID_PARAMETER;

	// test if the res file is not read-only
	if (!testAssert(!fReadOnly))
		return VE_ACCESS_DENIED;
	
	sWORD	curres = ::CurResFile();
	::UseResFile(fRefNum);
	
	sWORD	id = ::Unique1ID(type);
	OSErr macError = ::ResError();
	if (testAssert(macError == noErr))
	{
		error = WriteResource(inType, (sLONG) id, inData, inDataLen, inName);
		if (error == VE_OK)
			*outID = (sLONG) id;
	} else
		error = VErrorBase::NativeErrorToVError((VNativeError)macError);

	::UseResFile(curres);
	
	return error;
}


VError VMacResFile::DeleteResource(const VString& inType, sLONG inID)
{
	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK)
	{
		::RemoveResource(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			::DisposeHandle(data);
			data = NULL;
		}
	}
	return error;
}

VError VMacResFile::DeleteResource(const VString& inType, const VString& inName)
{
	Handle data = NULL;
	VError error = _GetResource(inType, inName, &data);
	if (error == VE_OK)
	{
		::RemoveResource(data);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			::DisposeHandle(data);
			data = NULL;
		}
	}
	return error;
}


VError VMacResFile::GetResourceID(const VString& inType, const VString& inName, sLONG* outID) const
{
	*outID = 0;
	Handle data = NULL;
	VError error = _GetResource(inType, inName, &data);
	if (error == VE_OK)
	{
		ResType type;
		sWORD	id;
		Str255 spName;
		::GetResInfo(data, &id, &type, spName);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			*outID = (sLONG) id;
		} else
			error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	return error;
}


VError VMacResFile::GetResourceName(const VString& inType, sLONG inID, VString& outName) const
{
	outName.Clear();
	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK)
	{
		ResType type;
		sWORD	id;
		Str255 spName;
		::GetResInfo(data, &id, &type, spName);
		OSErr macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			outName.MAC_FromMacPString(spName);
		} else
			error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	return error;
}


VError VMacResFile::SetResourceName(const VString& inType, sLONG inID, const VString& inName)
{
	// test if the res file is not read-only
	if (!testAssert(!fReadOnly))
		return VE_ACCESS_DENIED;

	Handle data = NULL;
	VError error = _GetResource(inType, inID, &data);
	if (error == VE_OK)
	{
		Str255 spName;
		inName.MAC_ToMacPString(spName, 255);
		::SetResInfo(data, (short) inID, spName);
		OSErr macError = ::ResError();
		assert(macError == noErr);
		error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	return error;
}


Boolean VMacResFile::GetString(VString& outString, sLONG inID) const
{
	outString.Clear();

	if (!testAssert(fRefNum != -1))
		return false;
	
	if (!testAssert(inID >= -32768 && inID <= 32767))
		return false;

	sWORD	curres = ::CurResFile();
	if (curres != fRefNum)
		::UseResFile(fRefNum);

	Handle data = fUseResourceChain ? ::GetResource('STR ', (short) inID) : ::Get1Resource('STR ', (short) inID);

	if (curres != fRefNum)
		::UseResFile(curres);
		
	Boolean isOK = false;
	if (testAssert(data != NULL))
	{
		if (testAssert(*data != NULL))
		{
			::HLock(data);
			outString.MAC_FromMacPString((StringPtr) *data);
			::HUnlock(data);
			isOK = true;
		}
	}
	return isOK;
}

Boolean VMacResFile::GetString(VString& outString, sLONG inID, sLONG inIndex) const
{
	outString.Clear();

	if (!testAssert(fRefNum != -1))
		return false;
	
	if (!testAssert(inID >= -32768 && inID <= 32767 && inIndex > 0))
		return false;

	Boolean isOK = false;

	sWORD	curres = ::CurResFile();
	if (curres != fRefNum)
		::UseResFile(fRefNum);
		
	if (fReadOnly && (fLastStringListID == inID) && (fLastStringList != NULL))
	{
		if (*fLastStringList == NULL)
			::LoadResource(fLastStringList);
	}
	else
	{
		fLastStringListID = (short) inID;
		fLastStringList = fUseResourceChain ? ::GetResource('STR#', fLastStringListID) : ::Get1Resource('STR#', fLastStringListID);
	}
	
	if (curres != fRefNum)
		::UseResFile(curres);

	if (testAssert(fLastStringList != NULL))
	{
		if (testAssert(*fLastStringList != NULL))
		{
			::HLock(fLastStringList);
	
			sLONG count = **(sWORD**) fLastStringList;
			if (count >= inIndex)
			{
				StringPtr strs = (StringPtr) *fLastStringList + sizeof(sWORD);
				while(--inIndex)
					strs += strs[0] + 1;
				outString.MAC_FromMacPString(strs);
				isOK = true;
			}
	
			::HUnlock(fLastStringList);
		}
	}
	return isOK;
}

VResourceIterator* VMacResFile::NewIterator(const VString& inType) const
{
	if (!testAssert(fRefNum != -1))
		return NULL;

	ResType type = inType.GetOsType();
	if (!testAssert(type != 0))
		return NULL;

	return new VMacResourceIterator(fRefNum, type, fUseResourceChain);
}

VError VMacResFile::GetResourceListIDs(const VString& inType, VArrayLong& outIDs) const
{
OSErr macError = noErr;

	outIDs.SetCount(0);

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	ResType type = inType.GetOsType();
	if (type == 0)
		return VE_INVALID_PARAMETER;
	
	sWORD	oldref = ::CurResFile();
	sWORD	ref = fRefNum;
	while(ref != -1 && ref != 0)
	{
		::UseResFile(ref);
		sWORD	count = ::Count1Resources(type);
		::UseResFile(oldref);
		
		for(sWORD i = 1 ; i <= count ; ++i)
		{
			::UseResFile(ref);
			::SetResLoad(false);
			Handle data = ::Get1IndResource(type, i);
			macError = ::ResError();
			::SetResLoad(true);
			::UseResFile(oldref); // one must restore proper context before calling VArrayValue::AppendWord

			if (data != NULL)
			{
				ResType type2;
				sWORD	id;
				Str255 spName;
				::GetResInfo(data, &id, &type2, spName);
				macError = ::ResError();
				if (macError == noErr)
				{
					if (!fUseResourceChain || outIDs.Find(id) == -1)
						outIDs.AppendLong(id);
				}
			}
		}
		
		if (!fUseResourceChain)
			break;
		else
		{
			macError = ::GetNextResourceFile(ref, &ref); // returns ref = 0 and noerror at the end
			if (!testAssert(macError == noErr))
				break;
		}
	}
	return VE_OK;
}


VError VMacResFile::GetResourceListTypes(VArrayString& outTypes) const
{
	outTypes.SetCount(0);

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;

	sWORD	oldref = ::CurResFile();
	sWORD	ref = fRefNum;
	while(ref != -1 && ref != 0)
	{
		::UseResFile(ref);
		sWORD	count = ::Count1Types();
		::UseResFile(oldref);
		
		for(sWORD	i = 1 ; i <= count ; ++i)
		{
			ResType type;
			::UseResFile(ref);
			::Get1IndType(&type, i);
			::UseResFile(oldref); // one must restore proper context before calling outTypes.AppendVString

			outTypes.AppendString(VResTypeString(type));
		}
		
		if (!fUseResourceChain)
			break;
		else
		{
			OSErr macError = ::GetNextResourceFile(ref, &ref); // returns ref = 0 and noerror at the end
			if (!testAssert(macError == noErr))
				break;
		}
	}
	return VE_OK;
}

VError VMacResFile::Open(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	if (!testAssert(fRefNum == -1))
		return VE_STREAM_ALREADY_OPENED;
	return _Open(inPath, inFileAccess, inCreate);
}



VError VMacResFile::GetResourceListNames(const VString& inType, VArrayString& outNames) const
{
OSErr macError = noErr;

	outNames.SetCount(0);

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	ResType type = inType.GetOsType();
	if (type == 0)
		return VE_INVALID_PARAMETER;
	
	sWORD	oldref = ::CurResFile();
	sWORD	ref = fRefNum;
	while(ref != -1 && ref != 0)
	{
		::UseResFile(ref);
		sWORD	count = ::Count1Resources(type);
		::UseResFile(oldref);
		
		for(sWORD	i = 1 ; i <= count ; ++i)
		{
			::UseResFile(ref);
			::SetResLoad(false);
			Handle data = ::Get1IndResource(type, i);
			macError = ::ResError();
			::SetResLoad(true);
			::UseResFile(oldref); // one must restore proper context before calling VArrayValue::AppendWord

			if (data != NULL)
			{
				ResType type2;
				sWORD	id;
				Str255 spName;
				::GetResInfo(data, &id, &type2, spName);
				macError = ::ResError();
				if (macError == noErr && spName[0] > 0)
				{
					VString name;
					name.MAC_FromMacPString(spName);
					if (outNames.Find(name) == -1)
						outNames.AppendString(name);
				}
			}
		}
		
		if (!fUseResourceChain)
			break;
		else
		{
			macError = ::GetNextResourceFile(ref, &ref); // returns ref = 0 and noerror at the end
			if (!testAssert(macError == noErr))
				break;
		}
	}
	return VE_OK;
}


void VMacResFile::Flush()
{
	fLastStringList = NULL;
	if (testAssert(fRefNum != -1) && !fReadOnly)
		::UpdateResFile(fRefNum);
}


VError VMacResFile::_GetResource(const VString& inType, const VString& inName, Handle* outHandle) const
{
OSErr	macError = noErr;

	Str255	spName;
	*outHandle = NULL;

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	ResType type = inType.GetOsType();
	if (type == 0)
		return VE_INVALID_PARAMETER;
	
	inName.MAC_ToMacPString(spName, 255);
	
	sWORD	curres = ::CurResFile();
	if (curres != fRefNum)
		::UseResFile(fRefNum);

	*outHandle = fUseResourceChain ? ::GetNamedResource(type, spName) : ::Get1NamedResource(type, spName);
	macError = ::ResError();
	
	if (curres != fRefNum)
		::UseResFile(curres);

	VError error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	
	return error;
}


VError VMacResFile::Close()
{
	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	VError error = VE_OK;
	if (fCloseFile)
	{
		::CloseResFile(fRefNum);
		OSErr macError = ::ResError();
		error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	}
	
	fRefNum = -1;
	fReadOnly = true;
	fLastStringList = NULL;
	return error;
}


VError VMacResFile::_Open(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	if (!testAssert(fRefNum == -1))
		return VE_STREAM_ALREADY_OPENED;
	
	assert(fLastStringList == NULL);
	
	fCloseFile = true;
	
	sWORD	curres = ::CurResFile();
	
	FSSpec spec;
	VString fullPath;
	inPath.GetPath(fullPath);
	Boolean isOk = XMacFile::HFSPathToFSSpec(fullPath, &spec) == noErr;

	SignedByte permission = fsCurPerm;
	switch(inFileAccess)
	{
		case FA_READ:	permission = fsRdPerm; break;
		case FA_READ_WRITE:	permission = fsWrPerm; break;
		case FA_SHARED:	permission = fsRdWrShPerm; break;
	}

	if (!isOk && inCreate)
		::FSpCreateResFile(&spec, '????', '????', smSystemScript);

	// the resource file for component is private so... open it as orphan (m.c)
	OSErr err = ::FSpOpenOrphanResFile(&spec, permission,&fRefNum);

	if (fRefNum == -1 && inCreate)
	{
		// L.E. 17/02/00 le fichier peut exister mais sans res fork
		// pas FSpCreateResFile car on veut conserver type et createur
		::HCreateResFile(spec.vRefNum, spec.parID, spec.name);
		::FSpOpenOrphanResFile(&spec, permission,&fRefNum);
	}
	
	OSErr macError = ::ResError();
	
	::UseResFile(curres);

	assert(fRefNum != -1);

	if (fRefNum == -1)
		fReadOnly = true;
	else
	{
		sWORD	flags = ::GetResFileAttrs(fRefNum);
		fReadOnly = (inFileAccess == FA_READ) || ((flags & mapReadOnly) != 0);
	}
	
	return VErrorBase::NativeErrorToVError((VNativeError)macError);
}


void VMacResFile::DebugTest()
{
	// look for a "test.rsrc" file in same folder as the app
	// and duplicate it
	VFilePath sourcePath = VProcess::Get()->GetExecutableFolderPath();
	sourcePath.SetFileName(VStr63("test.rsrc"));
	
	DebugMsg("Looking for file = %V\n", &sourcePath);
	
	VMacResFile srcFile(sourcePath, FA_READ, false);

	sourcePath.SetFileName(VStr63("test copy.rsrc"));
	VMacResFile dstFile(sourcePath, FA_READ_WRITE, true);

	VError error = srcFile.CopyResourcesInto(dstFile);

	dstFile.Close();
	srcFile.Close();
}


VError VMacResFile::_GetResource(const VString& inType, sLONG inID, Handle* outHandle) const
{
OSErr	macError = noErr;

	*outHandle = NULL;

	if (!testAssert(fRefNum != -1))
		return VE_STREAM_NOT_OPENED;
	
	if (!testAssert(inID >= -32768 && inID <= 32767))
		return VE_RES_INDEX_OUT_OF_RANGE;
	
	ResType type = inType.GetOsType();

	if (type == 0)
		return VE_INVALID_PARAMETER;
	
	sWORD	id = (short) inID;

	if (!fUseResourceChain)
		macError = ::InsertResourceFile( fRefNum, kRsrcChainAboveApplicationMap);
	
	sWORD	curres = ::CurResFile();
	if (curres != fRefNum)
		::UseResFile(fRefNum);

	*outHandle = fUseResourceChain ? ::GetResource(type, id) : ::Get1Resource(type, id);
	macError = ::ResError();
	
	if (curres != fRefNum)
		::UseResFile(curres);

	if (!fUseResourceChain)
		macError = ::DetachResourceFile( fRefNum);

	VError error = VErrorBase::NativeErrorToVError((VNativeError)macError);
	
	return error;
}


Boolean VMacResFile::IsValid() const
{
	return (fRefNum != -1);
}


#pragma mark-

VMacResourceIterator::VMacResourceIterator(sWORD inRefNum, ResType inType, Boolean inUseResourceChain)
{
	assert(inRefNum != -1);
	fRefNum = inRefNum;
	fType = inType;
	fUseResourceChain = inUseResourceChain;
	fPos = -1;
}


VHandle VMacResourceIterator::_GetResource(sLONG inIndex) const
{
	sWORD	refnum = fRefNums.GetWord(inIndex);
	sWORD	id = fIDs.GetWord(inIndex);
	
	sWORD	curres = ::CurResFile();
	if (curres != refnum)
		::UseResFile(refnum);

	::SetResLoad(false);
	Handle data = ::Get1Resource(fType, id);
	OSErr macError = ::ResError();
	::SetResLoad(true);

	if (curres != refnum)
		::UseResFile(curres);

	VHandle theData = NULL;
	if (data != NULL)
	{
		::LoadResource(data);
		macError = ::ResError();
		if (testAssert(macError == noErr))
		{
			assert(*data != NULL);
			::HLock(data);
			theData = VMemory::NewHandleFromData(*data, ::GetHandleSize(data));
			::HUnlock(data);
			data = NULL;
		}
	}
	return theData;
}


void VMacResourceIterator::_Load()
{
	OSErr macError = noErr;

	fIDs.SetCount(0);
	fRefNums.SetCount(0);
	
	sWORD	oldref = ::CurResFile();
	sWORD	ref = fRefNum;
	while(ref != -1 && ref != 0)
	{
		::UseResFile(ref);
		sWORD	count = ::Count1Resources(fType);
		::UseResFile(oldref);
		
		for(sWORD i = 1 ; i <= count ; ++i)
		{
			::UseResFile(ref);
			::SetResLoad(false);
			Handle data = ::Get1IndResource(fType, i);
			macError = ::ResError();
			::SetResLoad(true);
			::UseResFile(oldref); // one must restore proper context before calling VArrayValue::AppendWord

			if (data != NULL)
			{
				ResType type;
				sWORD	id;
				Str255 spName;
				::GetResInfo(data, &id, &type, spName);
				macError = ::ResError();
				if (macError == noErr)
				{
					fRefNums.AppendWord(ref);
					fIDs.AppendWord(id);
				}
			}
		}
		
		if (!fUseResourceChain)
			break;
		else
		{
			macError = ::GetNextResourceFile(ref, &ref); // returns ref = 0 and noerror at the end
			if (!testAssert(macError == noErr))
				break;
		}
	}
}


void VMacResourceIterator::SetPos(sLONG inPos)
{
	if (fPos == -1)
		_Load();
	if (testAssert(inPos >= 1 && inPos <= fIDs.GetCount()))
		fPos = inPos;
}


VHandle VMacResourceIterator::Previous()
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return NULL;
	if (--fPos <= 0)
		return NULL;
	return _GetResource(fPos);
}


VHandle VMacResourceIterator::Next()
{
	if (fPos == -1)
		return First();

	assert(fPos <= fIDs.GetCount() );

	if (fPos < fIDs.GetCount() )
		return _GetResource(++fPos);
	
	return NULL;
}


VHandle VMacResourceIterator::Last ()
{
	if (fPos == -1)
		_Load();
	
	fPos = fIDs.GetCount();
	return (fPos > 0) ? _GetResource(fPos) : NULL;
}


sLONG VMacResourceIterator::GetPos() const
{
	return fPos;
}


VHandle VMacResourceIterator::First()
{
	if (fPos == -1)
		_Load();
	
	if (fIDs.GetCount() <= 0)
	{
		fPos = 0;
		return NULL;
	}
	
	fPos = 1;
	return _GetResource(fPos);
}


VHandle VMacResourceIterator::Current() const
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return NULL;
	return _GetResource(fPos);
}

sLONG VMacResourceIterator::GetCurrentResourceID()
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return 0;
	return fIDs.GetWord(fPos);
}


void VMacResourceIterator::GetCurrentResourceName(VString& outName)
{
	if (!testAssert(fPos >= 1 && fPos <= fIDs.GetCount()))
		return;

	sWORD	refnum = fRefNums.GetWord(fPos);
	sWORD	id = fIDs.GetWord(fPos);
	
	sWORD	curres = ::CurResFile();
	if (curres != refnum)
		::UseResFile(refnum);

	::SetResLoad(false);
	Handle data = ::Get1Resource(fType, id);
	OSErr macError = ::ResError();
	::SetResLoad(true);

	if (curres != refnum)
		::UseResFile(curres);

	if (data != NULL)
	{
		ResType type;
		sWORD	id2;
		Str255 spName;
		::GetResInfo(data, &id2, &type, spName);
		macError = ::ResError();
		if (testAssert(macError == noErr))
			outName.MAC_FromMacPString(spName);
	}
}

#endif // #if !__LP64__


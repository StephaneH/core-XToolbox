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
#include "XWinResource.h"
#include "VFile.h"
#include "VSystem.h"
#include "VMemory.h"
#include "VError.h"
#include "VBlob.h"


static VError WIN_GetLastError()
{
	return VErrorBase::NativeErrorToVError( ::GetLastError());
}

VWinResFile::VWinResFile(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	fCloseFile = true;
	fModule = NULL;
	fUpdateHandle = NULL;
	fPath = NULL;
	fLastID = 0;

	fLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	_Open(inPath, inFileAccess, inCreate);
}


VWinResFile::VWinResFile(HMODULE inModule)
{
	fModule = inModule;
	fUpdateHandle = NULL;
	fCloseFile = false;
	fPath = NULL;
	fLastID = 0;
	fLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
}


VWinResFile::~VWinResFile()
{
	if (fModule != NULL)
	{
		if (fUpdateHandle != NULL)
		{
			testAssert(::EndUpdateResource(fUpdateHandle, TRUE));
			fUpdateHandle = NULL;
		}
		
		if (fCloseFile)
		{
			BOOL test = ::FreeLibrary(fModule);
			assert(test);
			fModule = NULL;
		}
	}
	if (fPath != NULL)
	{
		delete fPath;
		fPath = NULL;
	}
}


Boolean VWinResFile::IsValid() const
{
	return fModule != NULL;
}


VError VWinResFile::AddResource(const VString& inType, const void* inData, sLONG inDataLen, const VString* inName, sLONG* outID)
{
	// if empty name: find a unique ID
	// else: just use the given name, the resource must not already exists
	
	*outID = 0;
	VError err = VE_OK;
	
	if (inName != NULL && inName->GetLength() > 0)
	{
		// warning: this is not efficient for multiple add because one must load/close the file each time !
		if (!testAssert(!ResourceExists(inType, *inName)))
			err = VE_RES_ALREADY_EXISTS;
		else
			err = WriteResource(inType, 0, inData, inDataLen, inName);
	}
	else
	{
		// by id, one must find a free id
		Boolean found = false;
		sLONG max = 65536L;

		VStr31 uniType(inType);
		PWCHAR p = (PWCHAR) uniType.GetCPointer();
		for(; (--max >= 0) && !found ; ++fLastID)
			found = (::FindResourceExW(fModule, p, MAKEINTRESOURCEW(fLastID), fLanguage) != NULL);
		
		if (found)
			err = WriteResource(inType, fLastID-1, inData, inDataLen, NULL);
	}
	return err;
}


VError VWinResFile::DeleteResource(const VString& inType, const VString& inName)
{
	if (!testAssert(inType.GetLength() > 0))
		return vThrowError(VE_INVALID_PARAMETER);
	
	VError err = _BeginUpdate();
	if (err == VE_OK)
	{
		BOOL isOK = FALSE;
		VString uniType(inType);
		VString uniName(inName);
		isOK = ::UpdateResourceW(fUpdateHandle, (PWCHAR) uniType.GetCPointer(), (PWCHAR) uniName.GetCPointer(), fLanguage, NULL, 0);

		if (!testAssert(isOK))
			err = WIN_GetLastError();
	}
	return err;
}




VError VWinResFile::Close()
{
	VError err = VE_OK;
	if (testAssert(fModule != NULL))
	{
		if (fUpdateHandle != NULL)
		{
			testAssert(::EndUpdateResource(fUpdateHandle, TRUE));
			fUpdateHandle = NULL;
		}
	
		if (fCloseFile)
		{
			if (!testAssert(::FreeLibrary(fModule)))
				err = WIN_GetLastError();
		}
	} else
		err = vThrowError(VE_STREAM_NOT_OPENED);

	if (fPath != NULL)
	{
		delete fPath;
		fPath = NULL;
	}
	
	fModule = NULL;
	return err;
}

VError VWinResFile::_GetResource(const VString& inType, const VString& inName, HRSRC* outRsrc) const
{
	*outRsrc = NULL;

	_EndUpdate();
	
	if (!testAssert(fModule != NULL))
		return vThrowError(VE_STREAM_NOT_OPENED);

	VString uniType(inType);
	VString uniName(inName);
	*outRsrc = ::FindResourceExW(fModule, (PWCHAR) uniType.GetCPointer(), (PWCHAR) uniName.GetCPointer(), fLanguage);

	return (*outRsrc == NULL) ? VE_RES_NOT_FOUND : VE_OK;
}

void VWinResFile::Flush()
{
	_EndUpdate();
}


VError VWinResFile::_BeginUpdate()
{
	if (!testAssert(fModule != NULL))
		return vThrowError(VE_STREAM_NOT_OPENED);

	if (!testAssert(fPath != NULL))
		return vThrowError(VE_UNIMPLEMENTED); // means the VWinResFile has been given a module ID without a path

	VError err = VE_OK;
	if (fUpdateHandle == NULL)
	{

		testAssert(::FreeLibrary(fModule));
		fModule = NULL;

		fUpdateHandle = ::BeginUpdateResourceW(*fPath, FALSE);
	
		if (!testAssert(fUpdateHandle != NULL))
			err = WIN_GetLastError();
	}
	return err;
}


VError VWinResFile::_EndUpdate() const
{
	VError err = VE_OK;
	if (fUpdateHandle != NULL)
	{
		if (!testAssert(::EndUpdateResource(fUpdateHandle, FALSE)))
			err = WIN_GetLastError();
		fUpdateHandle = NULL;

		// let's reopen the dll
		assert((fPath != NULL) && (fModule == NULL));
		fModule = ::LoadLibraryExW(*fPath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		
		if (!testAssert(fModule != NULL) && (err == VE_OK))
			err = WIN_GetLastError();
	}
	return err;
}

Boolean VWinResFile::GetString(VString& outString, sLONG inID, sLONG inIndex) const
{
	outString.Clear();
	
	// there's no STR# on windows, so we remap to StringTables
	// we divide the 0->65535 range into 512 STR# of 128 strings each
	// the first 256 STR# are for XToolBox whose public IDs starts at 9000
	// the last 256 STR# are for the application whose public IDs should start at 128
	
	if (!testAssert((inID >= 9000L && inID < 9256L) || (inID >= 128L && inID <= 384L)))
	{
		vThrowError(VE_INVALID_PARAMETER);
		return false;
	}

	if (!testAssert(inIndex > 0L && inIndex <= 128L))
	{
		vThrowError(VE_INVALID_PARAMETER);
		return false;
	}
	
	sLONG id = inIndex - 1;
	if (inID >= 9000L)
		id += (inID - 9000) * 128;
	else
		id += 256 * 128 + (inID - 128) * 128;
	
	return GetString(outString, id);
}


VError VWinResFile::Open(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	if (!testAssert(fModule == NULL))
		return vThrowError(VE_STREAM_ALREADY_OPENED);
	return _Open(inPath, inFileAccess, inCreate);
}



VError VWinResFile::_GetResource(const VString& inType, sLONG inID, HRSRC* outRsrc) const
{
	*outRsrc = NULL;

	_EndUpdate();

	if (!testAssert(fModule != NULL))
		return vThrowError(VE_STREAM_NOT_OPENED);

	if (!testAssert(inID >= -32768 && inID <= 32767))
		return vThrowError(VE_RES_INDEX_OUT_OF_RANGE);

	VString uniType(inType);
	*outRsrc = ::FindResourceExW(fModule, (PWCHAR) uniType.GetCPointer(), MAKEINTRESOURCEW((WORD) inID), fLanguage);

	return (*outRsrc == NULL) ? VE_RES_NOT_FOUND : VE_OK;
}


VError VWinResFile::_Open(const VFilePath& inPath, FileAccess inFileAccess, Boolean inCreate)
{
	VError err = VE_OK;

	if (!testAssert(fModule == NULL))
		return vThrowError(VE_STREAM_ALREADY_OPENED);

	fCloseFile = true;

	if (fPath != NULL)
	{
		delete fPath;
		fPath = NULL;
	}
	
	fCloseFile = true;
	fPath = new XWinFullPath(inPath);
	fModule = ::LoadLibraryExW(*fPath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	
	if (!testAssert(fModule != NULL))
		err = WIN_GetLastError();
	
	return err;
}


VError VWinResFile::WriteResource(const VString& inType, sLONG inID, const void* inData, sLONG inDataLen, const VString* inName)
{
	if (!testAssert(inType.GetLength() > 0) || (inData == NULL))
		return vThrowError(VE_INVALID_PARAMETER);
	
	VError err = _BeginUpdate();
	if (err == VE_OK)
	{
		Boolean byID = ((inName == NULL) || (inName->GetLength() == 0));
		assert(byID || (inID == 0));
		BOOL isOK = FALSE;

		VString uniType(inType);
		if (byID)
		{
			isOK = ::UpdateResourceW(fUpdateHandle, (PWCHAR) uniType.GetCPointer(), MAKEINTRESOURCEW((WORD) inID), fLanguage, (char*)inData, inDataLen);
		}
		else
		{
			VString uniName(*inName);
			isOK = ::UpdateResourceW(fUpdateHandle, (PWCHAR) uniType.GetCPointer(), (PWCHAR) uniName.GetCPointer(), fLanguage, (char*)inData, inDataLen);
		}

		if (!testAssert(isOK))
			err = WIN_GetLastError();
	}
	return err;
}


VError VWinResFile::SetResourceName(const VString& inType, sLONG inID, const VString& inName)
{
	assert(false);
	return vThrowError(VE_UNIMPLEMENTED);
}


Boolean VWinResFile::ResourceExists(const VString& inType, sLONG inID) const
{
	HRSRC rsrc = NULL;
	return _GetResource(inType, inID, &rsrc) == VE_OK;
}


Boolean VWinResFile::ResourceExists(const VString& inType, const VString& inName) const
{
	HRSRC rsrc = NULL;
	return _GetResource(inType, inName, &rsrc) == VE_OK;
}


Boolean VWinResFile::GetString(VString& outString, sLONG inID) const
{
	outString.Clear();

	_EndUpdate();
	
	if (!testAssert(fModule != NULL))
	{
		vThrowError(VE_STREAM_NOT_OPENED);
		return false;
	}

	if (!testAssert(inID >= 0 && inID <= 65535L))
	{
		vThrowError(VE_RES_INDEX_OUT_OF_RANGE);
		return false;
	}
	
	char buff[4096L];
	
	UINT size;
	size = ::LoadStringW(fModule, (UINT) inID, (PWCHAR)&buff[0], sizeof(buff)/2);
	if (size > 0)
		outString.FromUniCString((UniChar*) buff);
	
	return (size > 0) ? true : (::GetLastError() == 0);
}


VError VWinResFile::GetResourceSize(const VString& inType, sLONG inID, sLONG* outSize) const
{
	*outSize = 0;
	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inID, &rsrc);
	if (err == VE_OK)
	{
		*outSize = ::SizeofResource(fModule, rsrc);
		if (*outSize == 0)
		{
			err = WIN_GetLastError();
			assert(err == VE_OK);
		}
	}
	return err;
}


VError VWinResFile::GetResourceSize(const VString& inType, const VString& inName, sLONG* outSize) const
{
	*outSize = 0;
	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inName, &rsrc);
	if (err == VE_OK)
	{
		*outSize = ::SizeofResource(fModule, rsrc);
		if (*outSize == 0)
		{
			err = WIN_GetLastError();
			assert(err == VE_OK);
		}
	}
	return err;
}


VError VWinResFile::GetResourceName(const VString& inType, sLONG inID, VString& outName) const
{
	outName.Clear();
	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inID, &rsrc);
	if (err == VE_OK)
	{
		outName.FromUniCString(L"#");
		outName.AppendLong(inID);
	}
	return err;
}


VError VWinResFile::GetResourceID(const VString& inType, const VString& inName, sLONG* outID) const
{
	*outID = 0;
	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inName, &rsrc);

	if (err == VE_OK)
	{
		if (testAssert(inName.GetLength() > 0 && inName[0] == CHAR_NUMBER_SIGN))
		{
			for(sLONG i = 1 ; i <= inName.GetLength() ; ++i)
			{
				UniChar c = inName[i-1];
				if (c >= '0' && c <= '9')
					*outID = 10L*(*outID) + (c - '0');
				else
				{
					err = vThrowError(VE_INVALID_PARAMETER);
					break;
				}
			}
		} else
			err = vThrowError(VE_INVALID_PARAMETER);
		assert(err == VE_OK);
	}
	return err;
}


bool VWinResFile::GetResource ( const VString& inType, sLONG inID, VBlob& outData) const
{
	bool ok = false;
	
	outData.Clear();

	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inID, &rsrc);
	if (err == VE_OK)
	{
		HGLOBAL hData = ::LoadResource(fModule, rsrc);
		if (!testAssert(hData != NULL))
			err = WIN_GetLastError();
		else
		{
			sLONG size = ::SizeofResource(fModule, rsrc);
			LPVOID p = ::LockResource(hData);
			if (testAssert(p != NULL))
			{
				ok = outData.PutData( (VPtr) p, size) == VE_OK;
			}
		}
	}
	return ok;
}


VHandle VWinResFile::GetResource(const VString& inType, sLONG inID) const
{
	VHandle h = NULL;

	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inID, &rsrc);
	if (err == VE_OK)
	{
		HGLOBAL hData = ::LoadResource(fModule, rsrc);
		if (!testAssert(hData != NULL))
			err = WIN_GetLastError();
		else
		{
			sLONG size = ::SizeofResource(fModule, rsrc);
			LPVOID p = ::LockResource(hData);
			if (testAssert(p != NULL))
				h =	VMemory::NewHandleFromData((VPtr) p, size);
		}
	}
	return h;
}


VHandle VWinResFile::GetResource(const VString& inType, const VString& inName) const
{
VHandle h = NULL;

	HRSRC rsrc = NULL;
	VError err = _GetResource(inType, inName, &rsrc);
	if (err == VE_OK)
	{
		HGLOBAL hData = ::LoadResource(fModule, rsrc);
		if (!testAssert(hData != NULL))
			err = WIN_GetLastError();
		else
		{
			sLONG size = ::SizeofResource(fModule, rsrc);
			LPVOID p = ::LockResource(hData);
			if (testAssert(p != NULL))
				h =	VMemory::NewHandleFromData((VPtr) p, size);
		}
	}
	return h;
}


VError VWinResFile::DeleteResource(const VString& inType, sLONG inID)
{
	if (!testAssert(inType.GetLength() > 0))
		return vThrowError(VE_INVALID_PARAMETER);
	
	VError err = _BeginUpdate();
	if (err == VE_OK)
	{
		BOOL isOK = FALSE;
		VString uniType(inType);
		isOK = ::UpdateResourceW(fUpdateHandle, (PWCHAR) uniType.GetCPointer(), MAKEINTRESOURCEW((WORD) inID), fLanguage, NULL, 0);
		if (!testAssert(isOK))
			err = WIN_GetLastError();
	}
	return err;
}


VResourceIterator* VWinResFile::NewIterator(const VString& inType) const
{
	return new VClassicResourceIterator(const_cast<VWinResFile*>(this), inType);
}
	


VError VWinResFile::GetResourceListNames(const VString& inType, VArrayString& outNames) const
{
	return vThrowError(VE_UNIMPLEMENTED);
}


BOOL CALLBACK EnumIdFunc(HMODULE pModule,LPCTSTR pType,LPCTSTR Id,LONG_PTR pParam)
{
	VArrayLong* outIDs = (VArrayLong*)pParam;
	outIDs->AppendLong( (USHORT) Id);
	return true;
}


VError VWinResFile::GetResourceListIDs(const VString& inType, VArrayLong& outIDs) const
{
	VError	err = VE_OK;
	BOOL isOK = FALSE;
	outIDs.SetCount(0);

	VString uniType(inType);
	isOK = EnumResourceNamesW(fModule,(PWCHAR) uniType.GetCPointer(),(ENUMRESNAMEPROCW)EnumIdFunc,(LONG_PTR)&outIDs); 

	if (!isOK)
	{
		if (::GetLastError() != ERROR_RESOURCE_TYPE_NOT_FOUND)	// no resource of specified type
			err = WIN_GetLastError();
	}
	return err;
}
	

VError VWinResFile::GetResourceListTypes(VArrayString& outTypes) const
{
	return vThrowError(VE_UNIMPLEMENTED);
}

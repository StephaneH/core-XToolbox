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
#include "V4DPictureIncludeBase.h"
#include "V4DPictureProxyCache.h"


VPictureCacheBlock::VPictureCacheBlock()
{
	fDataTimeStamp=0;
	fParentTimeStamp=0;
	fLastAccess=0;
}
VPictureCacheBlock::VPictureCacheBlock(class VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp)
{
	fPicture.FromVPicture_Retain(inPict,false);
	fDataTimeStamp=inDataTimeStamp;
	fParentTimeStamp=inParentTimeStamp;
	UpdateLastAccess();
}
VPictureCacheBlock::VPictureCacheBlock(const VPictureCacheBlock& inBlock)
{
	fPicture.FromVPicture_Retain(inBlock.fPicture,false);
	fDataTimeStamp=inBlock.fDataTimeStamp;
	fParentTimeStamp=inBlock.fParentTimeStamp;
	fLastAccess=inBlock.fLastAccess;
}
VPictureCacheBlock::~VPictureCacheBlock()
{
	
}
void  VPictureCacheBlock::GetPicture(VPicture& outPict,uLONG8 *outStamp)const
{
	outPict.FromVPicture_Retain(fPicture,false);
	if(outStamp)
		*outStamp=fDataTimeStamp;
	fLastAccess=VSystem::GetCurrentTime();
}
void VPictureCacheBlock::UpdatePicture(VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp)
{
	fPicture.FromVPicture_Retain(inPict,false);
	fDataTimeStamp=inDataTimeStamp;
	fParentTimeStamp=inParentTimeStamp;
	UpdateLastAccess();
}
void VPictureCacheBlock::UpdateParentTimeStamp(uLONG inParentTimeStamp)
{
	fParentTimeStamp=inParentTimeStamp;
}
/*****************************************************************************/

VPictureProxyCache::VPictureProxyCache(sWORD inKind)
{
	fTimeStamp=1;
	fKind=inKind;
}
VPictureProxyCache::~VPictureProxyCache()
{

}

VError VPictureProxyCache::GetPicture(const VString& inName,VPicture& outPicture,uLONG8 &outStamp)
{
	VError err;
	fCrit.Lock();
	err=_GetPicture(inName,outPicture,outStamp);
	fCrit.Unlock();
	return err;
}
VError VPictureProxyCache::GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp)
{
	VError err;
	fCrit.Lock();
	err=_GetPicture(inID,outPicture,outStamp);
	fCrit.Unlock();
	return err;
}

bool VPictureProxyCache::Synchronize(const VString& inName,VPicture& outPicture,uLONG8 &oiStamp)
{
	bool changed;
	fCrit.Lock();
	changed=_Synchronize(inName,outPicture,oiStamp);
	fCrit.Unlock();
	return changed;
}
bool VPictureProxyCache::Synchronize(sLONG inID,VPicture& outPicture,uLONG8& ioStamp)
{
	bool changed;
	fCrit.Lock();
	changed=_Synchronize(inID,outPicture,ioStamp);
	fCrit.Unlock();
	return  changed;
}

void VPictureProxyCache::Clear()
{
	fCrit.Lock();
	_Clear();
	fCrit.Unlock();
}
void VPictureProxyCache::AutoWash(bool inForce)
{
	fCrit.Lock();
	_AutoWash(inForce);
	fCrit.Unlock();
}
void VPictureProxyCache::Clear(sLONG inID)
{
	fCrit.Lock();
	_Clear(inID);
	fCrit.Unlock();
}
void VPictureProxyCache::Clear(const VString& inName)
{
	fCrit.Lock();
	_Clear(inName);
	fCrit.Unlock();
}

VError VPictureProxyCache::_GetPicture(const VString& inName,VPicture& outPicture,uLONG8 &outStamp)	
{
	VError err=VE_OK;
	outStamp=0;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inName);
	if(cacheblock)
	{
		cacheblock->GetPicture(outPicture,&outStamp);
		cacheblock->Release();
	}
	else
	{
		outPicture.Clear();
		err=VE_RES_NOT_FOUND;
	}
	return err;
}
VError VPictureProxyCache::_GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp)	
{
	VError err=VE_OK;
	outStamp=0;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inID);
	if(cacheblock)
	{
		cacheblock->GetPicture(outPicture,&outStamp);
		cacheblock->Release();
	}
	else
	{
		outPicture.Clear();
		err=VE_RES_NOT_FOUND;
	}
	return err;
}

bool VPictureProxyCache::GetTimeStamp(const VString& inName,uLONG8 &ioStamp,bool& ioParentStampDirty)
{
	bool res=false;
	ioStamp=0;
	ioParentStampDirty=false;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inName,false);
	if(cacheblock)
	{
		ioStamp=cacheblock->GetTimeStamp();
		ioParentStampDirty = (fTimeStamp!=cacheblock->GetParentTimeStamp());
		cacheblock->Release();
		res=true;
	}
	return res;
}
bool VPictureProxyCache::GetTimeStamp(sLONG inID,uLONG8& ioStamp,bool& ioParentStampDirty)
{
	bool res=false;
	ioStamp=0;
	ioParentStampDirty=false;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inID,false);
	if(cacheblock)
	{
		ioStamp=cacheblock->GetTimeStamp();
		ioParentStampDirty = (fTimeStamp!=cacheblock->GetParentTimeStamp());
		cacheblock->Release();
		res=true;
	}
	return res;
}

bool VPictureProxyCache::_Synchronize(const VString& inName,VPicture& outPicture,uLONG8& oiStamp)
{
	bool changed=false;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inName);
	if(cacheblock)
	{
		if(oiStamp != cacheblock->GetTimeStamp())
		{
			cacheblock->GetPicture(outPicture,&oiStamp);
			changed=true;
		}
		else
			cacheblock->UpdateLastAccess();
		cacheblock->Release();
	}
	else
	{
		oiStamp=0;
		outPicture.Clear();
		changed=true;
	}
	return changed;
}
bool VPictureProxyCache::_Synchronize(sLONG inID,VPicture& outPicture,uLONG8 &oiStamp)
{
	bool changed=false;
	VPictureCacheBlock* cacheblock=_RetainAndUpdateCacheBlock(inID);
	if(cacheblock)
	{
		if(cacheblock->GetTimeStamp()!=oiStamp)
		{
			cacheblock->GetPicture(outPicture,&oiStamp);
			changed=true;
		}
		else
			cacheblock->UpdateLastAccess();
		cacheblock->Release();
	}
	else
	{
		oiStamp=0;
		outPicture.Clear();
		changed=true;
	}
	return changed;
}

void VPictureProxyCache::SetDirty()
{
	VInterlocked::Increment((sLONG*)&fTimeStamp);
}

void VPictureProxyCache::SetDirty(uLONG inNewStamp)
{
	VInterlocked::Exchange((sLONG*)&fTimeStamp,inNewStamp);
}

VPictureCacheBlock* VPictureProxyCache::_RetainAndUpdateCacheBlock(const VString& inName,bool inTryLoad)
{
	return NULL;
}
VPictureCacheBlock* VPictureProxyCache::_RetainAndUpdateCacheBlock(sLONG inID,bool inTryLoad)
{
	return NULL;
}
void VPictureProxyCache::_Clear()
{
	
}

/**********************************************************/


VFilePictureCacheBlock::VFilePictureCacheBlock()
{
	fParentFolder=NULL;
}
VFilePictureCacheBlock::VFilePictureCacheBlock(const VFilePictureCacheBlock& inBlock)
:VPictureCacheBlock(inBlock)
{
	fParentFolder=RetainRefCountable(inBlock.fParentFolder);
}

VFilePictureCacheBlock::VFilePictureCacheBlock(class VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp,VFolder* inParentFolder)
:VPictureCacheBlock(inPict,inDataTimeStamp,inParentTimeStamp)
{
	fParentFolder = RetainRefCountable(inParentFolder);
}
VFilePictureCacheBlock::~VFilePictureCacheBlock()
{
	ReleaseRefCountable(&fParentFolder);
}

bool VFilePictureCacheBlock::IsParentFolder(const VFolder* inFolder)
{
	if(!fParentFolder)
	{
		return inFolder==NULL;
	}
	else 
	{
		#if 0//VERSIONDEBUG
		VString s1,s2;
		if(inFolder)
			inFolder->GetPath(s1);
		fParentFolder->GetPath(s2);
		DebugMsg("VFilePictureCacheBlock::IsParentFolder : %S  --- %S\r\n",&s2,&s1);
		#endif
		return fParentFolder->IsSameFolder(inFolder);
	}

}

void VFilePictureCacheBlock::SetParentFolder(VFolder* inFolder)
{
	CopyRefCountable(&fParentFolder, inFolder);
}

void VFilePictureCacheBlock::SetNotFound()
{
	fPicture.Clear();
	fDataTimeStamp = 0;
	ReleaseRefCountable(&fParentFolder);
}

/**********************************************************/

VPictureProxyCache_Folder::VPictureProxyCache_Folder(const VFolder& inFolder)
:VPictureProxyCache(PictureProxy_File)
{
	fFolders.push_back(new VFolder(inFolder));
}
VPictureProxyCache_Folder::~VPictureProxyCache_Folder()
{
	_Clear();
	
}
void VPictureProxyCache_Folder::_Clear()
{
	for(_VPictMap_Iterrator it=fMap.begin();it!=fMap.end();it++)
	{
		if((*it).second)
		{
			(*it).second->Release();
			(*it).second=0;
		}
	}
	fMap.clear();
}
void VPictureProxyCache_Folder::_Clear(const VString& inName)
{
	_VPictMap_Iterrator it=fMap.find(inName);
	if(it!=fMap.end())
	{
		if((*it).second)
			(*it).second->Release();
		fMap.erase(it);
	}
}

VPictureCacheBlock* VPictureProxyCache_Folder::_RetainAndUpdateCacheBlock(const VString& inName,bool inTryLoad)
{
	VString debugstr;
	VFilePictureCacheBlock* result=0;
	
	_VPictMap_Const_Iterrator it=fMap.find(inName);
	if(it!=fMap.end())
	{
		result=(*it).second;
	}
	
	if(inTryLoad)
	{
		if(!result || (result && result->GetParentTimeStamp() != fTimeStamp))
		{
			sLONG8 stamp;
			VTime ftime_mod,ftime_crea;
			VPictureCodecFactoryRef fact;
			VPicture thepict;
			VString nativepath=inName;
			if(nativepath.GetCPointer() && nativepath.GetCPointer()[0]=='/')
				nativepath.Remove(1,1);
			
			#if VERSIONMAC
			nativepath.Exchange(CVSTR("/"), CVSTR(":"), 1,inName.GetLength());
			#else
			nativepath.Exchange(CVSTR("/"), CVSTR("\\"), 1,inName.GetLength());
			#endif
			
			for(_FolderVector_Reverse_Iterator i = fFolders.rbegin() ; i != fFolders.rend() ; i++)
			{
				#if 0//VERSIONDEBUG
				(*i)->GetPath(debugstr);
				DebugMsg("Checking Folder : %S\r\n",&debugstr);
				#endif
				VFile* f=new VFile( **i ,nativepath);
				
				if(f)
				{
					if(f->Exists())
					{
						f->GetTimeAttributes(&ftime_mod,&ftime_crea);
						f->GetSize(&stamp);
						stamp+=ftime_mod.GetStamp();
						stamp+=ftime_crea.GetStamp();
						
						if(!result)
						{
							VPictureData* data=fact->CreatePictureData(*f);
							if(data)
							{
								thepict.FromVPictureData(data);
								data->Release();
							}
							else 
							{
								thepict.Clear();
							}
	
							result=new VFilePictureCacheBlock(thepict,stamp,fTimeStamp,*i);
							fMap[inName]=result;
						}
						else
						{
							
							if(result->GetTimeStamp()!=stamp || !result->IsParentFolder(*i))
							{
								VPictureData* data=fact->CreatePictureData(*f);
								if(data)
								{
									thepict.FromVPictureData(data);
									data->Release();
								}
								else
								{
									thepict.Clear();
								}
								
								result->UpdatePicture(thepict,stamp,fTimeStamp);
								if(!result->IsParentFolder(*i))
									result->SetParentFolder(*i);
							}
						}
						
						break;
					}
					else 
					{
						// file not found
						if(result && result->IsParentFolder(*i))
						{
							// reset block if already in cache
							result->SetNotFound();
						}
					}
					f->Release();
				}
			}
		}
	}
	if(result)
		result->Retain();
	return result;
}
void VPictureProxyCache_Folder::_AutoWash(bool inForce)
{
	uLONG curtime = VSystem::GetCurrentTime() - 600000;
	for(_VPictMap_Iterrator it=fMap.begin();it!=fMap.end();)
	{
		if((*it).second)
		{
			if(!(*it).second->IsVPictureUsed() && ((*it).second->GetLastAccess() < curtime || inForce))
			{
				_VPictMap_Iterrator todelete=it;
				it++;
				(*todelete).second->Release();
				fMap.erase(todelete);
			}
			else
			{
				it++;
			}
		}
		else
		{
			// pas de cache block, on l'efface de la liste
			_VPictMap_Iterrator todelete=it;
			it++;
			(*todelete).second->Release();
			fMap.erase(todelete);
		}
	}
}

bool VPictureProxyCache_Folder::IsPictureFileURL(const VFile &inFile,VString& outRel)
{
	bool result=false;
	VString inPath;
	VString cachepath;
	
	for(_FolderVector_Reverse_Iterator i=fFolders.rbegin();i!=fFolders.rend();++i)
	{
		inFile.GetPath(inPath,FPS_POSIX);
		(*i)->GetPath(cachepath,FPS_POSIX);
		outRel="";
		if(cachepath.GetLength()<=inPath.GetLength())
		{
			inPath.SubString(1,cachepath.GetLength());
			result = (inPath==cachepath);
			if(result)
			{
				inFile.GetPath(outRel,FPS_POSIX);
				outRel.SubString(cachepath.GetLength()+1,outRel.GetLength()-cachepath.GetLength());
				break;
			}
		}
	}
	return result;
}

VError VPictureProxyCache_Folder::_GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp)
{
	VString st;
	st.FromLong(inID);
	return inherited::_GetPicture(st,outPicture,outStamp);
}

void VPictureProxyCache_Folder::_AppendSearchPath(const VFolder& inPath)
{
	if(!_FindSearchPath(inPath))
		fFolders.push_back(new VFolder(inPath));
}

bool VPictureProxyCache_Folder::_FindSearchPath(const VFolder& inFolder,sWORD* outIndex)
{
	for(_FolderVector_Iterator i=fFolders.begin();i!=fFolders.end();++i)
	{
		if((*i)->IsSameFolder(&inFolder))
		{
			if(outIndex)
				*outIndex = distance(fFolders.begin(),i);;
			return true;
		}
	}
	return false;
}
void VPictureProxyCache_Folder::_RemoveSearchPath(const VFolder& inFolder)
{
	for(_FolderVector_Iterator i=fFolders.begin();i!=fFolders.end();++i)
	{
		if((*i)->IsSameFolder(&inFolder))
		{
			fFolders.erase(i);
			return;
		}
	}
}
	

VPictureProxyCache_LocalizedFolder::VPictureProxyCache_LocalizedFolder(const VFolder& inRoot,DialectCode inDialect,DialectCode inAlternateDialect)
:inherited(inRoot)
{
	fDefaultFolder=new VFolder(inRoot);
	fLocalizedFolder=NULL;
	fAlternateFolder=NULL;
	
	XBOX::VectorOfVFolder localizationFolders;
	if(XBOX::VIntlMgr::GetLocalizationFoldersWithDialect(inDialect, inRoot.GetPath(), localizationFolders))
	{
		assert(localizationFolders.size()==1);
		XBOX::VRefPtr<XBOX::VFolder> ref=localizationFolders[0];
		fLocalizedFolder=ref.Retain();
		_AppendSearchPath(*ref);
	}
	
	SetAlternateDialect(inAlternateDialect);
}
VPictureProxyCache_LocalizedFolder::~VPictureProxyCache_LocalizedFolder()
{
	_Clear();
	ReleaseRefCountable(&fDefaultFolder);
	ReleaseRefCountable(&fLocalizedFolder);
	ReleaseRefCountable(&fAlternateFolder);
}

void VPictureProxyCache_LocalizedFolder::SetAlternateDialect(DialectCode inAlternateDialect)
{
	if(inAlternateDialect != DC_NONE)
	{
		XBOX::VectorOfVFolder localizationFolders;
		
		if(XBOX::VIntlMgr::GetLocalizationFoldersWithDialect(inAlternateDialect, fDefaultFolder->GetPath(), localizationFolders))
		{
			assert(localizationFolders.size()==1);
			XBOX::VRefPtr<XBOX::VFolder> ref=localizationFolders[0];
			
			if(fLocalizedFolder->IsSameFolder(ref))
			{
				// application laguage : reset alternate cache
				if(fAlternateFolder)
				{
					_RemoveSearchPath(*fAlternateFolder);
					ReleaseRefCountable(&fAlternateFolder);
				}	
			}
			else
			{
				if(fAlternateFolder)
				{
					if(!fAlternateFolder->IsSameFolder(ref))
					{
						_RemoveSearchPath(*fAlternateFolder);
						ReleaseRefCountable(&fAlternateFolder);
						_AppendSearchPath(*ref);
						fAlternateFolder=ref.Retain();
					}
				}
				else
				{
					_AppendSearchPath(*ref);
					fAlternateFolder=ref.Retain();
				}
				SetDirty();
			}
		}
	}
	else
	{
		if(fAlternateFolder)
		{
			_RemoveSearchPath(*fAlternateFolder);
			ReleaseRefCountable(&fAlternateFolder);
			SetDirty();
		}
	}
}

void VPictureProxyCache_LocalizedFolder::_AutoWash(bool inForce)
{
	inherited::_AutoWash(inForce);
}

	
void VPictureProxyCache_LocalizedFolder::_Clear()
{
	inherited::_Clear();
}

void VPictureProxyCache_LocalizedFolder::_Clear(const VString& inName)
{
	inherited::_Clear(inName);
}

VPictureCacheBlock* VPictureProxyCache_LocalizedFolder::_RetainAndUpdateCacheBlock(const VString& inName,bool inTryLoad)
{
	VPictureCacheBlock* result=0;

	result=inherited::_RetainAndUpdateCacheBlock(inName,inTryLoad);
	
	return result;
}
bool VPictureProxyCache_LocalizedFolder::IsPictureFileURL(const VFile &inFile,VString& outRel)
{
	return inherited::IsPictureFileURL(inFile,outRel);
}


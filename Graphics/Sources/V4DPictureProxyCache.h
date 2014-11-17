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
#ifndef __VPictureProxyCache__
#define __VPictureProxyCache__

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VPictureCacheBlock :public VObject, public IRefCountable
{
	protected:
	VPictureCacheBlock();
	VPictureCacheBlock(const VPictureCacheBlock& inBlock);
	public:
	VPictureCacheBlock(class VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp);
	virtual ~VPictureCacheBlock();
	inline uLONG8 GetTimeStamp()const{return fDataTimeStamp;}
	inline uLONG GetParentTimeStamp()const{return fParentTimeStamp;}
	void	GetPicture(VPicture& outPict,uLONG8* outStamp)const;
	void	UpdatePicture(VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp);
	uLONG GetLastAccess(){return fLastAccess;};
	void UpdateLastAccess(){fLastAccess=VSystem::GetCurrentTime();};
	bool IsVPictureUsed()
	{
		sLONG use=fPicture._GetMaxPictDataRefcount();
		return use>1;
	}
	void UpdateParentTimeStamp(uLONG inParentTimeStamp);
	protected:
	VPicture							fPicture;
	uLONG8								fDataTimeStamp;
	uLONG								fParentTimeStamp;
	mutable uLONG						fLastAccess;
};
enum _PictureProxyKind
{
	PictureProxy_None=0,
	PictureProxy_File,
	PictureProxy_Last
};
class XTOOLBOX_API VPictureProxyCache :public VObject, public IRefCountable
{
	public:
	VPictureProxyCache(sWORD inKind=PictureProxy_None);
	virtual ~VPictureProxyCache();
	
	VError GetPicture(const VString& inName,VPicture& outPicture,uLONG8 &outStamp);
	VError GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp);
	
	bool Synchronize(const VString& inName,VPicture& outPicture,uLONG8 &oiStamp);
	bool Synchronize(sLONG inID,VPicture& outPicture,uLONG8& ioStamp);
	
	bool GetTimeStamp(const VString& inName,uLONG8 &ioStamp,bool& ioParentStampDirty);
	bool GetTimeStamp(sLONG inID,uLONG8& ioStamp,bool& ioParentStampDirty);	

	void Clear(sLONG inID);
	void Clear(const VString& inName);
	
	void AutoWash(bool inForce=false);
	
	void Clear();
	void Update();
	
	virtual void SetDirty();
	virtual void SetDirty(uLONG inNewStamp);	// BAD: hides inherited

	uLONG GetTimeStamp(){return fTimeStamp;}
	sWORD GetKind(){return fKind;}
	protected:
	
	virtual VError _GetPicture(const VString& inName,VPicture& outPicture,uLONG8 &outStamp);
	virtual VError _GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp);
	
	virtual bool _Synchronize(const VString& inName,VPicture& outPicture,uLONG8 &ioStamp);
	virtual bool _Synchronize(sLONG inID,VPicture& outPicture,uLONG8& ioStamp);

	virtual void _Clear();
	virtual void _Clear(sLONG inID){;}
	virtual void _Clear(const VString& inName){;}
	
	virtual void _AutoWash(bool inForce){;}
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(const VString& inName,bool inTryLoad=true);
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(sLONG inID,bool inTryLoad=true);

	sWORD fKind;
	VCriticalSection fCrit;
	uLONG fTimeStamp;
};

class XTOOLBOX_API VFilePictureCacheBlock :public VPictureCacheBlock
{
	protected:
	VFilePictureCacheBlock();
	VFilePictureCacheBlock(const VFilePictureCacheBlock& inBlock);
	public:
	VFilePictureCacheBlock(class VPicture& inPict,uLONG8 inDataTimeStamp,uLONG inParentTimeStamp,VFolder* inParentFolder);
	virtual ~VFilePictureCacheBlock();
	
	VFolder* GetParentFolder(){return fParentFolder;}
	void	SetParentFolder(VFolder* inFolder);
	bool	IsParentFolder(const VFolder* inFolder);
	void	SetNotFound();
	private:
	
	VFolder* fParentFolder;
};

class XTOOLBOX_API VPictureProxyCache_Folder :public VPictureProxyCache
{
	friend class VPictureProxyCache_LocalizedFolder;
	typedef VPictureProxyCache inherited;
	public:
	VPictureProxyCache_Folder(const VFolder &inFolder);
	virtual ~VPictureProxyCache_Folder();
	bool IsPictureFileURL(const VFile &inFile,VString& outRelPath);
	protected:
	
	virtual void	_Clear();
	virtual void	_Clear(sLONG inID){;}
	virtual void	_Clear(const VString& inName);
	virtual void	_AutoWash(bool inForce=false);
	virtual VError	_GetPicture(sLONG inID,VPicture& outPicture,uLONG8 &outStamp);
	virtual VError	_GetPicture(const VString& inName,VPicture& outPicture,uLONG8 &outStamp)
	{
		return inherited::_GetPicture(inName,outPicture,outStamp);
	}
	
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(const VString& inName,bool inTyLoad=true);
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(sLONG inID,bool inTryLoad=true)
	{
		return NULL;
	}
	//const VString& GetFolderPath(){return fFolder.GetPath().GetPath();}
	sLONG _IsEmpty(){return fMap.empty();}	
	
	protected:
	
	VectorOfVFolder& _GetSearchPath(){return fFolders;}
	void _AppendSearchPath(const VFolder& inPath);
	bool _FindSearchPath(const VFolder& inFolder,sWORD* outIndex=NULL);
	void _RemoveSearchPath(const VFolder& inFolder);
	sLONG _CountSearchPath(){return fFolders.size();}
	typedef std::vector < VFolder* > _FolderVector ;
	typedef VectorOfVFolder::reverse_iterator _FolderVector_Reverse_Iterator ;
	typedef VectorOfVFolder::const_reverse_iterator _FolderVector_Const_Reverse_Iterator ;
	typedef VectorOfVFolder::iterator _FolderVector_Iterator ;
	typedef VectorOfVFolder::const_iterator _FolderVector_Const_Iterator ;

	VectorOfVFolder fFolders;
	
	typedef std::map < VString , class VFilePictureCacheBlock*> _VPictMap ;
	typedef _VPictMap::iterator _VPictMap_Iterrator ;
	typedef _VPictMap::const_iterator _VPictMap_Const_Iterrator ;
	
	_VPictMap fMap;
};

class XTOOLBOX_API VPictureProxyCache_LocalizedFolder : public VPictureProxyCache_Folder
 { 
	typedef VPictureProxyCache_Folder inherited;
	public:
	VPictureProxyCache_LocalizedFolder(const VFolder& inRoot,DialectCode inDialect,DialectCode inAlternateDialect=DC_NONE);
	virtual ~VPictureProxyCache_LocalizedFolder();

	virtual bool IsPictureFileURL(const VFile &inFile,VString& outRelPath);
	void	SetAlternateDialect(DialectCode inAlternateDialect);
	protected:
	virtual void	_AutoWash(bool inForce=false);
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(const VString& inName,bool inTryLoad=true);
	virtual VPictureCacheBlock* _RetainAndUpdateCacheBlock(sLONG inID,bool inTryLoad=true)
	{
		return NULL;
	}
	virtual void	_Clear();
	virtual void	_Clear(sLONG inID){;}
	virtual void	_Clear(const VString& inName);
	private:
	
	VFolder*					fDefaultFolder;
	VFolder*					fLocalizedFolder;
	VFolder*					fAlternateFolder;
};

END_TOOLBOX_NAMESPACE

#endif
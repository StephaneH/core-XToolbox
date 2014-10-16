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
#ifndef __4DPictureDataSource__
#define __4DPictureDataSource__

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VPictureDataProvider :public VObject, public IRefCountable
{
	private:
	VPictureDataProvider& operator=(const VPictureDataProvider&){assert(false);return *this;}
	VPictureDataProvider(const VPictureDataProvider&){assert(false);}
	public:
	enum xV4DPictureDataSourceKind
	{
		kInvalid=-1,
		kVPtr=0,
		kVHandle,
		kMacHandle,
		kVBlob,
		kVBlobWithPtr,
		kVStream,
		kVFile,
		kVDataSource
	};
	protected:
	VPictureDataProvider(sWORD inKind)
	{
		fKind=inKind;
		fLastError=VE_UNKNOWN_ERROR;
		fLockCount=0;
		fDataSize=0;
		fDecoder=0;
		fDataOffset=0;
		fIsValid=false;
		fLastVError=0;
	}
	
	virtual VPtr DoBeginDirectAccess()=0;
	virtual VError DoEndDirectAccess()=0;
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0)=0;
	
	public:
	
	// pp le passage par reference pour les sources VBlob, VStream, duplique le bloc de data
	// je ne peut pas travaillé directement sur les blob4d car la datasource doit etre const
	
	static VPictureDataProvider* Create(VStream& inStream,VSize inNbBytes=0, VIndex inOffset=0);
	static VPictureDataProvider* Create(VStream* inStream,uBOOL inOwnData,VSize inNbBytes=0, VIndex inOffset=0);
	
	static VPictureDataProvider* Create(const VBlob* inBlob,uBOOL inOwnData,VSize inNbBytes=0, VIndex inOffset=0);
	static VPictureDataProvider* Create(const VBlob& inBlob,VSize inNbBytes=0, VIndex inOffset=0);
	
	static VPictureDataProvider* Create(const VBlobWithPtr* inBlob,uBOOL inOwnData,VSize inNbBytes=0, VIndex inOffset=0);
	
	static VPictureDataProvider* Create(const VPtr inPtr,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset=0);

#if WITH_VMemoryMgr
	static VPictureDataProvider* Create(const VHandle inHandle,uBOOL inOwnData,VSize inNbBytes=0, VIndex inOffset=0);
#endif

#if !VERSION_LINUX
//TODO - jmo :  Verifier si pon garde ou pas
	static VPictureDataProvider* Create(const xMacHandle inHandle,uBOOL inOwnData,VSize inNbBytes=0, VIndex inOffset=0);
#endif

	static VPictureDataProvider* Create(VPictureDataProvider* inDs,VSize inNbBytes=0, VIndex inOffset=0);
	
	static VPictureDataProvider* Create(const VFile& inFile,uBOOL inKeepInMem=true);
	
	static VPictureDataProvider* Create(const VFilePath& inFilePath,const VBlob& inBlob);
	
	static void InitMemoryAllocator(VMacHandleAllocatorBase *inAlloc);
	static void DeinitStatic();
	
	virtual ~VPictureDataProvider()
	{
		//JQ 10/03/2010: fixed ACI0065236
		XBOX::ReleaseRefCountable(&fDecoder);
			
		if(fLastVError)
		{
			fLastVError->Release();
		}	
	}
	
	VPtr BeginDirectAccess()
	{
		VPtr result=0;
		if(fIsValid)
			result= DoBeginDirectAccess();
		return result;
	}
	VError EndDirectAccess()
	{
		VError err=VE_UNKNOWN_ERROR;
		if(fIsValid)
			err = DoEndDirectAccess();
		return SetLastError(err);
	}
	VError GetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0)
	{
		VError err=VE_UNKNOWN_ERROR;
		if(fIsValid)
		{
			err= DoGetData(inBuffer,inOffset,inSize,outSize);
		}
		return SetLastError(err);
	}
	
	virtual VError WriteToBlob(VBlob& inBlob,VIndex inOffset)=0;
	
	VString GetPictureKind();
	void SetDecoder(const class VPictureCodec* inDecoder);
	const VPictureCodec* RetainDecoder();
	
	VError GetLastError()
	{
		if(fLastVError)
			return fLastVError->GetError();
		else
			return fLastError;
	}
	VError SetLastError(VError inErr)
	{
		if(fLastVError)
		{
			fLastVError->Release();
			fLastVError=0;
		}
		return fLastError=inErr;
	}
	VError SetLastError(VErrorBase* inErr)
	{
		if(fLastVError)
		{
			fLastVError->Release();
			fLastVError=0;
		}
		fLastVError=inErr;
		if(fLastVError)
			return fLastVError->GetError();
		else
			return VE_OK;
	}
	VSize GetDataSize() const
	{
		if(!fIsValid)
			return 0;
		else
			return fDataSize;
	}
	uLONG8 GetDataSize64() const
	{
		if(!fIsValid)
			return 0;
		else
			return (uLONG8)fDataSize;
	}
	inline uBOOL IsValid(){return fIsValid;}
	
	void ThrowLastError()
	{
		if(GetLastError()!=VE_OK)
		{
			if(fLastVError)
			{
				VTask::PushError( fLastVError);
			}
			else
				vThrowError(fLastError)	;
		}
	}
	
	uBOOL IsFile(){return fKind==kVFile;}
	virtual bool GetFilePath(VFilePath& outPath){outPath.Clear();return false;}
	protected:
	
	static VMacHandleAllocatorBase* sMacAllocator;
	
	inline sLONG _Lock()
	{
		return VInterlocked::Increment( &fLockCount);
	}
	inline sLONG _Unlock()
	{
		return VInterlocked::Decrement( &fLockCount);
	}
	inline sLONG	_GetLockCount(){return fLockCount;}
	inline uBOOL _EnterCritical()
	{
		return fAccessCrit.Lock();
	}
	inline uBOOL _LeaveCritical()
	{
		return fAccessCrit.Unlock();
	}
	void _SetValid(VError inErr=VE_OK)
	{
		if(fLastVError)
			fLastVError->Release();
		
		fLastVError=0;
			
		fIsValid=inErr==VE_OK;
		fLastError=inErr;
	}
	void _SetValid(VErrorBase* inRetainedErrBase=0)
	{
		SetLastError(inRetainedErrBase);
		fIsValid=GetLastError()==VE_OK;
	}
	
	VIndex						fDataOffset;
	VSize						fDataSize;
	
	private:

	sWORD						fKind;
	sLONG						fLockCount;
	VCriticalSection			fAccessCrit;
	uBOOL						fIsValid;
	VError						fLastError;
	VErrorBase*					fLastVError;
	
	const VPictureCodec*		fDecoder;
};

class XTOOLBOX_API VPictureDataProvider_DS : public VPictureDataProvider
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_DS& operator=(const VPictureDataProvider_DS&){assert(false);return *this;}
	VPictureDataProvider_DS(const VPictureDataProvider_DS&):VPictureDataProvider(kInvalid){assert(false);}

	public:
	
	VPictureDataProvider_DS(VPictureDataProvider* inParent,VSize inNbBytes=0,VIndex inOffset=0);
	
	virtual ~VPictureDataProvider_DS();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0);
	virtual VError WriteToBlob(VBlob& inBlob,VIndex inOffset);
	private:
	VPictureDataProvider* fDs;
};

class XTOOLBOX_API VPictureDataProvider_DirectAccess : public VPictureDataProvider
{
	protected:
	VPictureDataProvider_DirectAccess(sWORD inKind);
	public:
	virtual ~VPictureDataProvider_DirectAccess();
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0);
	virtual VError WriteToBlob(VBlob& inBlob,VIndex inOffset);
};

class XTOOLBOX_API VPictureDataProvider_SequentialAccess : public VPictureDataProvider
{
	private:
	VPictureDataProvider_SequentialAccess& operator=(const VPictureDataProvider_SequentialAccess&){assert(false);return *this;}
	VPictureDataProvider_SequentialAccess(const VPictureDataProvider_SequentialAccess&):VPictureDataProvider(kInvalid){assert(false);}
	protected:
	VPictureDataProvider_SequentialAccess(sWORD inKind);
	public:
	virtual ~VPictureDataProvider_SequentialAccess();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	virtual VError WriteToBlob(VBlob& inBlob,VIndex inOffset);
	private:
	VPtr fDirectAccesCache;
};

#if WITH_VMemoryMgr
class XTOOLBOX_API VPictureDataProvider_VHandle : public VPictureDataProvider_DirectAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VHandle& operator=(const VPictureDataProvider_VHandle&){assert(false);return *this;}
	VPictureDataProvider_VHandle(const VPictureDataProvider_VHandle&):VPictureDataProvider_DirectAccess(kInvalid){assert(false);}
	protected:
	VPictureDataProvider_VHandle(sWORD inKind);
	virtual void _InitFromVHandle(const VHandle inHandle,bool inOwnedHandle,VSize inNbBytes=0,VIndex inOffset=0);
	virtual void _InitFromVBlob(const VBlob& inBlob,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	VPictureDataProvider_VHandle(const VHandle inHandle,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_VHandle(VStream& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_VHandle(const VBlob& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_VHandle(void* inDataPtr,VSize inNbBytes,VIndex inOffset=0);
	virtual ~VPictureDataProvider_VHandle();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	protected:
	VHandle	fHandle;
	VPtr	fPtr;
	uBOOL	fOwned;
};
#endif

#if !VERSION_LINUX
class XTOOLBOX_API VPictureDataProvider_MacHandle : public VPictureDataProvider_DirectAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_MacHandle& operator=(const VPictureDataProvider_MacHandle&){assert(false);return *this;}
	VPictureDataProvider_MacHandle(const VPictureDataProvider_MacHandle&):VPictureDataProvider_DirectAccess(kInvalid){assert(false);}

	void _InitFromMacHandle(const xMacHandle inHandle,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	VPictureDataProvider_MacHandle(const xMacHandle inHandle,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_MacHandle(VStream& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_MacHandle(const VBlob& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_MacHandle(const VHandle inStream,VSize inNbBytes=0,VIndex inOffset=0);
	virtual ~VPictureDataProvider_MacHandle();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	protected:
	xMacHandle	fHandle;
	VPtr		fPtr;
};
#endif

class XTOOLBOX_API VPictureDataProvider_VPtr : public VPictureDataProvider_DirectAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VPtr& operator=(const VPictureDataProvider_VPtr&){assert(false);return *this;}
	VPictureDataProvider_VPtr(const VPictureDataProvider_VPtr&):VPictureDataProvider_DirectAccess(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VPtr(sWORD inKind);
	virtual void _InitFromVPtr(const VPtr inPtr,bool inOnwedPtr,VSize inNbBytes,VIndex inOffset=0);
	virtual void _InitFromVBlob(const VBlob& inBlob,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	VPictureDataProvider_VPtr(const VPtr inPtr,VSize inNbBytes,VIndex inOffset=0);
	VPictureDataProvider_VPtr(VStream& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_VPtr(const VBlob& inStream,VSize inNbBytes=0,VIndex inOffset=0);
	virtual ~VPictureDataProvider_VPtr();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	protected:
	VPtr	fPtr;
	VPtr	fDataPtr;
	uBOOL	fOwned;
};

class XTOOLBOX_API VPictureDataProvider_VBlobWithPtr : public VPictureDataProvider_VPtr
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VBlobWithPtr& operator=(const VPictureDataProvider_VBlobWithPtr&){assert(false);return *this;}
	VPictureDataProvider_VBlobWithPtr(const VPictureDataProvider_VBlobWithPtr&):VPictureDataProvider_VPtr(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VBlobWithPtr(const VBlobWithPtr* inBlob,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	virtual ~VPictureDataProvider_VBlobWithPtr();
	virtual VPtr DoBeginDirectAccess();
	virtual VError DoEndDirectAccess();
	protected:
	mutable const VBlobWithPtr*	fBlob;
};

class XTOOLBOX_API VPictureDataProvider_VStream : public VPictureDataProvider_SequentialAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VStream& operator=(const VPictureDataProvider_VStream&){assert(false);return *this;}
	VPictureDataProvider_VStream(const VPictureDataProvider_VStream&):VPictureDataProvider_SequentialAccess(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VStream(VStream* inBlob,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	virtual ~VPictureDataProvider_VStream();
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0);
	protected:
	VStream*		fStream;
	
};

class XTOOLBOX_API VPictureDataProvider_VFile : public VPictureDataProvider_SequentialAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VFile& operator=(const VPictureDataProvider_VFile&){assert(false);return *this;}
	VPictureDataProvider_VFile(const VPictureDataProvider_VFile&):VPictureDataProvider_SequentialAccess(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VFile(const VFile& inFile,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	virtual ~VPictureDataProvider_VFile();
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0);
	virtual bool GetFilePath(VFilePath& outPath);
	private:
	VFileDesc*		fDesc;
};

class XTOOLBOX_API VPictureDataProvider_VFileInMem : public VPictureDataProvider_VPtr
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VFileInMem& operator=(const VPictureDataProvider_VFileInMem&){assert(false);return *this;}
	VPictureDataProvider_VFileInMem(const VPictureDataProvider_VFileInMem&):VPictureDataProvider_VPtr(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VFileInMem(const VFile& inFile,VSize inNbBytes=0,VIndex inOffset=0);
	VPictureDataProvider_VFileInMem(const VFilePath& inPath,const VBlob& inBlob);
	public:
	virtual ~VPictureDataProvider_VFileInMem();
	virtual bool GetFilePath(VFilePath& outPath)
	{
		outPath.FromFilePath(fPath);
		return true;
	}
	private:
	VFilePath		fPath;
};

class XTOOLBOX_API VPictureDataProvider_VBlob : public VPictureDataProvider_SequentialAccess
{
	friend class VPictureDataProvider;
	private:
	VPictureDataProvider_VBlob& operator=(const VPictureDataProvider_VBlob&){assert(false);return *this;}
	VPictureDataProvider_VBlob(const VPictureDataProvider_VBlob&):VPictureDataProvider_SequentialAccess(kInvalid){assert(false);}

	protected:
	VPictureDataProvider_VBlob(const VBlob* inBlob,VSize inNbBytes=0,VIndex inOffset=0);
	public:
	virtual ~VPictureDataProvider_VBlob();
	virtual VError DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize=0);
	protected:
	mutable const VBlob*	fBlob;
};

class XTOOLBOX_API VPictureDataStream : public VStream
{
public:
	VPictureDataStream( VPictureDataStream* inHandleStream );
	VPictureDataStream (VPictureDataProvider* inDS);
	virtual	~VPictureDataStream ();
	
	// Specific acceessors
	VPictureDataProvider*	RetainDataSource()
	{
		if(fDataSource)
			fDataSource->Retain();
		return fDataSource;
	}
	
protected:
	VPictureDataProvider* fDataSource;
	sLONG8	fLogSize;
	// Inherited from VStream
	virtual VError	DoOpenReading ();
	virtual VError	DoCloseReading ();
	virtual VError	DoOpenWriting () ;
	virtual VError	DoCloseWriting (Boolean inSetSize);
	virtual VError	DoGetData (void* inBuffer, VSize* ioCount);
	virtual VError	DoPutData (const void* inBuffer, VSize inNbBytes);
	virtual VError	DoSetSize (sLONG8 inNewSize);
	virtual VError	DoSetPos (sLONG8 inNewPos);
	virtual sLONG8	DoGetSize ();
};


class XTOOLBOX_API xV4DPictureBlobRef  :public VObject , public IRefCountable
{
public:
	xV4DPictureBlobRef(VBlob* inBlob,uBOOL inReadOnly=false);
	xV4DPictureBlobRef( const xV4DPictureBlobRef& inBlob);
	xV4DPictureBlobRef* ExchangeBlob();
	virtual ~xV4DPictureBlobRef();
	VSize	GetSize () const;
	VError	SetSize (VSize inNewSize);
	VError	GetData (void* inBuffer, VSize inNbBytes, VIndex inOffset = 0, VSize* outNbBytesCopied = NULL) const;
	VError	PutData (const void* inBuffer, VSize inNbBytes, VIndex inOffset = 0);
	VError	Flush( void* inDataPtr, void* inContext) const;
	void SetReservedBlobNumber(sLONG inBlobNum);
	void*	LoadFromPtr (const void* inDataPtr, Boolean inRefOnly = false);
	void*	WriteToPtr (void* inDataPtr, Boolean inRefOnly = false) const;
	VSize	GetSpace () const;
	bool	IsNull() const;
	VValue* FullyClone(bool ForAPush = false) const;
	void Detach(Boolean inMayEmpty = false);
	void RestoreFromPop(void* context);

	VBlob* GetVBlobPtr(){return fBlob;}
	VBlob& GetVBlobRef(){return *fBlob;}

	void SetReadOnly(uBOOL inReadOnly){fReadOnly=inReadOnly;}
	uBOOL IsReadOnly(){return fReadOnly;}

private:
	uBOOL fReadOnly;
	VBlob* fBlob;
};

#if VERSIONWIN
 
class VPictureDataProvider_Stream : public IStream
{
    LONG m_cRef; // QI refcount
    ULONG m_nPos; // the current position in the stream
    ULONG m_nSize; // the current size of the stream
    ULONG m_nData; // the size of the allocated data storage, can be < m_nSize
    BYTE* m_pData; // the data storage
	VPictureDataProvider *fDataSource;
 private:
     HRESULT Ensure(ULONG nNewData);
 
 public:
     VPictureDataProvider_Stream(VPictureDataProvider *inDataSource);
      virtual ~VPictureDataProvider_Stream();
 
     HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,void **ppvObject);
     ULONG STDMETHODCALLTYPE AddRef();
     ULONG STDMETHODCALLTYPE Release();
     HRESULT STDMETHODCALLTYPE Read(void *pv,ULONG cb,ULONG *pcbRead);
     HRESULT STDMETHODCALLTYPE Write(const void *pv,ULONG cb,ULONG *pcbWritten);
     HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
     HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
     HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten);
     HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
     HRESULT STDMETHODCALLTYPE Revert();
     HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
     HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
     HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg,DWORD grfStatFlag);
     HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);
     
 };
#elif VERSIONMAC

class xV4DPicture_MemoryDataProvider : public VObject
{
	public:
	static CGDataProviderRef CGDataProviderCreate(VPictureDataProvider* inDataSource);
	static CGDataProviderRef CGDataProviderCreate(char* inBuff,size_t inBuffSize,uBOOL inCopyData=false);
	
	static size_t _GetBytesCallback_seq(void *info,void *buffer, size_t count);
	static off_t _SkipBytesCallback_seq(void *info,off_t count);	
	static void _RewindCallback_seq (void *info);
	static void _ReleaseInfoCallback_seq (void *info);
	
	static const void* _GetBytesPointerCallback_direct(void *info);
	static size_t _GetBytesAtOffsetCallback_direct(void *info,void *buffer,size_t offset,size_t count);	
	static void _ReleasePointerCallback_direct (void *info,const void *pointer);
	
	
	xV4DPicture_MemoryDataProvider(char* inBuff,size_t inBuffSize,uBOOL inCopyData=false);
	
	operator CGDataProviderRef() {return fDataRef;}

	protected:
	
	xV4DPicture_MemoryDataProvider();
	xV4DPicture_MemoryDataProvider& operator=(const xV4DPicture_MemoryDataProvider&){assert(false);return *this;}
	virtual ~xV4DPicture_MemoryDataProvider();
	virtual size_t GetBytesCallback_seq(void *buffer, size_t count);
	virtual off_t SkipBytesCallback_seq(off_t count);
	virtual void RewindCallback_seq ();
	virtual void ReleaseInfoCallback_seq();
	
	virtual const void* GetBytesPointerCallback_direct();
	virtual size_t GetBytesAtOffsetCallback_direct(void *buffer,size_t offset,size_t count);	
	virtual void ReleasePointerCallback_direct (const void *pointer);
	
	
	
	
	virtual void _Init(char* inBuff,size_t inBuffSize,uBOOL inCopyData=false);
	
				
	CGDataProviderRef fDataRef;
	
	uBOOL fOwnBuffer;
	
	char* fDataPtr;
	char* fMaxDataPtr;
	char* fCurDataPtr;
};

class xV4DPicture_DataProvider_DataSource : public xV4DPicture_MemoryDataProvider
{
	public:
	xV4DPicture_DataProvider_DataSource(VPictureDataProvider *inDataSource);
	protected:
	virtual ~xV4DPicture_DataProvider_DataSource();
	private:
	VPictureDataProvider *fDataSource;
};

#endif


END_TOOLBOX_NAMESPACE

#endif

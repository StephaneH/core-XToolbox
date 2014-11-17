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
#include "Kernel/VKernel.h"

#include "V4DPictureIncludeBase.h"


#if VERSIONMAC


CGDataProviderRef xV4DPicture_MemoryDataProvider::CGDataProviderCreate(VPictureDataProvider* inDataSource)
{
	xV4DPicture_MemoryDataProvider* data=new xV4DPicture_DataProvider_DataSource(inDataSource);
	return *data;
}

CGDataProviderRef xV4DPicture_MemoryDataProvider::CGDataProviderCreate(char* inBuff,size_t inBuffSize,uBOOL inCopyData)
{
	xV4DPicture_MemoryDataProvider* data=new xV4DPicture_MemoryDataProvider(inBuff,inBuffSize,inCopyData);
	return *data;
}
	
size_t xV4DPicture_MemoryDataProvider::_GetBytesCallback_seq(void *info,void *buffer, size_t count)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		return mthis->GetBytesCallback_seq(buffer, count);
	}
	else
	{
		return 0;
	}
}
off_t xV4DPicture_MemoryDataProvider::_SkipBytesCallback_seq(void *info,off_t count)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		return mthis->SkipBytesCallback_seq(count);
	}
	return 0;
}
void xV4DPicture_MemoryDataProvider::_RewindCallback_seq (void *info)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		mthis->RewindCallback_seq();
	}
}
void xV4DPicture_MemoryDataProvider::_ReleaseInfoCallback_seq (void *info)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		mthis->ReleaseInfoCallback_seq();
	}
			
}

const void* xV4DPicture_MemoryDataProvider::_GetBytesPointerCallback_direct(void *info)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		return mthis->GetBytesPointerCallback_direct();
	}
	return 0;
}
size_t xV4DPicture_MemoryDataProvider::_GetBytesAtOffsetCallback_direct(void *info,void *buffer,size_t offset,size_t count)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		return mthis->GetBytesAtOffsetCallback_direct(buffer,offset,count);
	}
	return 0;
}	
void xV4DPicture_MemoryDataProvider::_ReleasePointerCallback_direct (void *info,const void *pointer)
{
	xV4DPicture_MemoryDataProvider *mthis=reinterpret_cast<xV4DPicture_MemoryDataProvider*>(info);
	if(mthis)
	{
		mthis->ReleasePointerCallback_direct(pointer);
	}
}


xV4DPicture_MemoryDataProvider::xV4DPicture_MemoryDataProvider()
{
			
	fOwnBuffer=false;
	fDataRef=0;
	fDataPtr=0;
	fMaxDataPtr=0;
	fCurDataPtr=0;
}

xV4DPicture_MemoryDataProvider::xV4DPicture_MemoryDataProvider(char* inBuff,size_t inBuffSize,uBOOL inCopyData)
{
	_Init(inBuff,inBuffSize,inCopyData);
}

xV4DPicture_MemoryDataProvider::~xV4DPicture_MemoryDataProvider()
{
	if(fOwnBuffer && fDataPtr)
	{
		free((void*)fDataPtr);
	}
}

void xV4DPicture_MemoryDataProvider::_Init(char* inBuff,size_t inBuffSize,uBOOL inCopyData)
{
	
	if(inCopyData)
	{
		fOwnBuffer=TRUE;
		fDataPtr=(char*)malloc(inBuffSize);
		if(fDataPtr)
		{
			memcpy(fDataPtr,inBuff,inBuffSize);
			fCurDataPtr=fDataPtr;
			fMaxDataPtr= fCurDataPtr + inBuffSize;
			/*
			CGDataProviderDirectAccessCallbacks cb;
			
			cb.getBytePointer=xV4DPicture_MemoryDataProvider::_GetBytesPointerCallback_direct;
			cb.releaseBytePointer=xV4DPicture_MemoryDataProvider::_ReleasePointerCallback_direct;
			cb.getBytes=xV4DPicture_MemoryDataProvider::_GetBytesAtOffsetCallback_direct;
			cb.releaseProvider=xV4DPicture_MemoryDataProvider::_ReleaseInfoCallback_seq;
			
			fDataRef = ::CGDataProviderCreateDirectAccess((void*)this,inBuffSize,&cb);
			*/
			CGDataProviderSequentialCallbacks cb;
			cb.version=0;
			cb.getBytes=xV4DPicture_MemoryDataProvider::_GetBytesCallback_seq;
			cb.skipForward=xV4DPicture_MemoryDataProvider::_SkipBytesCallback_seq;
			cb.rewind=xV4DPicture_MemoryDataProvider::_RewindCallback_seq;
			cb.releaseInfo=XBOX::xV4DPicture_MemoryDataProvider::_ReleaseInfoCallback_seq;
			
			fDataRef=::CGDataProviderCreateSequential((void*)this,&cb);

		}
	}
	else
	{
		fOwnBuffer=FALSE;
		fDataPtr=inBuff;
		fCurDataPtr=fDataPtr;
		fMaxDataPtr= fCurDataPtr + inBuffSize;
		
		CGDataProviderSequentialCallbacks cb;
		cb.version=0;
		cb.getBytes=xV4DPicture_MemoryDataProvider::_GetBytesCallback_seq;
		cb.skipForward=xV4DPicture_MemoryDataProvider::_SkipBytesCallback_seq;
		cb.rewind=xV4DPicture_MemoryDataProvider::_RewindCallback_seq;
		cb.releaseInfo=XBOX::xV4DPicture_MemoryDataProvider::_ReleaseInfoCallback_seq;
		
		fDataRef=::CGDataProviderCreateSequential((void*)this,&cb);

	}
}
size_t xV4DPicture_MemoryDataProvider::GetBytesCallback_seq(void *buffer, size_t count)
{
	if(fCurDataPtr + count> fMaxDataPtr)
		count=std::max((uLONG)(fMaxDataPtr-fCurDataPtr),(uLONG)0);
	
	memcpy(buffer,fCurDataPtr,count);
	fCurDataPtr+=count;
	
	return count;
}
	
off_t xV4DPicture_MemoryDataProvider::SkipBytesCallback_seq(off_t count)
{
	fCurDataPtr+=count;
	return count;
}
void xV4DPicture_MemoryDataProvider::RewindCallback_seq ()
{
	fCurDataPtr=fDataPtr;
}
void xV4DPicture_MemoryDataProvider::ReleaseInfoCallback_seq()
{
	delete this;
}

const void* xV4DPicture_MemoryDataProvider::GetBytesPointerCallback_direct()
{
	return fDataPtr;
}
size_t xV4DPicture_MemoryDataProvider::GetBytesAtOffsetCallback_direct(void *buffer,size_t offset,size_t count)
{
	if(fDataPtr + offset + count> fMaxDataPtr)
		count=std::max((uLONG)(fMaxDataPtr-fCurDataPtr),(uLONG)0);
	
	memcpy(buffer,fDataPtr+offset,count);
	
	return count;
}	
void xV4DPicture_MemoryDataProvider::ReleasePointerCallback_direct (const void *pointer)
{
	if(fOwnBuffer && fDataPtr)
	{
		free((void*)fDataPtr);
		fDataPtr=0;
	}
}
	
	
	
xV4DPicture_DataProvider_DataSource::xV4DPicture_DataProvider_DataSource(VPictureDataProvider *inDataSource)
{
		fDataSource=inDataSource;
		fDataSource->Retain();
		
		_Init(fDataSource->BeginDirectAccess(),inDataSource->GetDataSize());
}

	
xV4DPicture_DataProvider_DataSource::~xV4DPicture_DataProvider_DataSource()
{
	fDataSource->EndDirectAccess();
	fDataSource->Release();
}


	
#endif
// static ctor for datasource
VMacHandleAllocatorBase* VPictureDataProvider::sMacAllocator=0;

void VPictureDataProvider::InitMemoryAllocator(VMacHandleAllocatorBase *inAlloc)
{
	sMacAllocator=inAlloc;
	if(sMacAllocator)
		sMacAllocator->Retain();
}
void VPictureDataProvider::DeinitStatic()
{
	if(sMacAllocator)
		sMacAllocator->Release();
	sMacAllocator=0;
}

VPictureDataProvider* VPictureDataProvider::Create(VStream& inStream,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result= new VPictureDataProvider_VPtr(inStream,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}
VPictureDataProvider* VPictureDataProvider::Create(VStream* inStream,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result;
	if(!inOwnData)
		result = new VPictureDataProvider_VPtr(*inStream,inNbBytes,inOffset);
	else
		result = new VPictureDataProvider_VStream(inStream,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}
	
VPictureDataProvider* VPictureDataProvider::Create(const VBlob* inBlob,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result;
	if(!inOwnData)
		result= new VPictureDataProvider_VPtr(*inBlob,inNbBytes,inOffset);
	else
		result= new VPictureDataProvider_VBlob(inBlob,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}
VPictureDataProvider* VPictureDataProvider::Create(const VBlob& inBlob,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result= new VPictureDataProvider_VPtr(inBlob,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}

VPictureDataProvider* VPictureDataProvider::Create(const VBlobWithPtr* inBlob,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result;
	if(!inOwnData)
		result= new VPictureDataProvider_VPtr(*inBlob,inNbBytes,inOffset);
	else
		result= new VPictureDataProvider_VBlobWithPtr(inBlob,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}



VPictureDataProvider* VPictureDataProvider::Create(const VPtr inPtr,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result=NULL;
	assert(inNbBytes);
	if(!inOwnData)
	{
		if(inNbBytes>0)
		{
			VPtr copie = VMemory::NewPtr(inNbBytes,'pict');
			if(copie!=NULL)
			{
				memcpy(copie,inPtr + inOffset , inNbBytes);
				result = new VPictureDataProvider_VPtr(copie,inNbBytes,0);
			}
		}
	}
	else
	{
		result = new VPictureDataProvider_VPtr(inPtr,inNbBytes,inOffset);
	}
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}

#if WITH_VMemoryMgr
VPictureDataProvider* VPictureDataProvider::Create(const VHandle inHandle,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result;
	if(!inOwnData)
	{
		void* datapre=VMemory::LockHandle(inHandle);
		result= new VPictureDataProvider_VHandle(datapre,inNbBytes,inOffset);
		VMemory::UnlockHandle(inHandle);
	}
	else
		result= new VPictureDataProvider_VHandle(inHandle,inNbBytes,inOffset);
	
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}
#endif

#if !VERSION_LINUX
//TODO - jmo :  Verifier si pon garde ou pas
VPictureDataProvider* VPictureDataProvider::Create(const xMacHandle inHandle,uBOOL inOwnData,VSize inNbBytes, VIndex inOffset)
{
	VPictureDataProvider* result;
	if(!inOwnData)
	{
		VPtr data=0;
		
		sMacAllocator->Lock(inHandle);
		if(inNbBytes==0)
			inNbBytes=sMacAllocator->GetSize(inHandle);
		
		data=VMemory::NewPtr(inNbBytes,'pict');
		if(data)
		{
			VMemory::CopyBlock(*inHandle,data,inNbBytes);
			result= new VPictureDataProvider_VPtr(data,inNbBytes,0);
		}
		sMacAllocator->Unlock(inHandle);
	}
	else
	{
		result= new VPictureDataProvider_MacHandle(inHandle,inNbBytes,inOffset);
	}
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}
#endif
	
VPictureDataProvider* VPictureDataProvider::Create(VPictureDataProvider* inDs,VSize inNbBytes, VIndex inOffset)
{

	VPictureDataProvider* result= new VPictureDataProvider_DS(inDs,inNbBytes,inOffset);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;

}

VPictureDataProvider* VPictureDataProvider::Create(const VFile& inFile,uBOOL inCreateDirectAcces)
{
	VPictureDataProvider* result=0;
	if(!inCreateDirectAcces)
	{
		result= new VPictureDataProvider_VFile(inFile);
		if(result && !result->IsValid())
		{
			result->ThrowLastError();
			result->Release();
			result=0;
		}
	}
	else
	{
		result= new VPictureDataProvider_VFileInMem(inFile);
		if(result && !result->IsValid())
		{
			result->ThrowLastError();
			result->Release();
			result=0;
		}
	}
	return result;
}

VPictureDataProvider* VPictureDataProvider::Create(const VFilePath& inFilePath,const VBlob& inBlob)
{
	VPictureDataProvider* result=0;
	result= new VPictureDataProvider_VFileInMem(inFilePath,inBlob);
	if(result && !result->IsValid())
	{
		result->ThrowLastError();
		result->Release();
		result=0;
	}
	return result;
}

VString VPictureDataProvider::GetPictureKind()
{
	VString res;
	if(fDecoder)
		res=fDecoder->GetDefaultIdentifier();
	else
		res= sCodec_none;
	return res;
}
void VPictureDataProvider::SetDecoder(const VPictureCodec* inDecoder)
{
	if(fDecoder)
	{
		fDecoder->Release();
	}
	fDecoder=inDecoder;
	fDecoder->Retain();
}
const VPictureCodec* VPictureDataProvider::RetainDecoder()
{
	if(fDecoder)
		fDecoder->Retain();
	return fDecoder;
}

// direct access base

VPictureDataProvider_DirectAccess::VPictureDataProvider_DirectAccess(sWORD inKind)
:VPictureDataProvider(inKind)
{
}
VPictureDataProvider_DirectAccess::~VPictureDataProvider_DirectAccess()
{
	
}
VError VPictureDataProvider_DirectAccess::WriteToBlob(VBlob& inBlob,VIndex inOffset)
{
	VError err=VE_UNKNOWN_ERROR;
	VPtr dataptr=BeginDirectAccess();
	if(dataptr)
	{
		err=inBlob.PutData(dataptr,GetDataSize(),inOffset);
		EndDirectAccess();
	}
	return err;
}	
VError VPictureDataProvider_DirectAccess::DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize)
{
	VError err=VE_UNKNOWN_ERROR;
	VPtr dataptr=BeginDirectAccess();
	if(outSize)
		*outSize=0;
	if(dataptr)
	{
		VSize tocopy=inSize;
		if(inOffset+inSize>fDataSize)
			tocopy=fDataSize-inOffset;
		CopyBlock(dataptr+inOffset,inBuffer,tocopy);
		if(outSize)
			*outSize=tocopy;
		err=EndDirectAccess();
	}
	else
		err=GetLastError();	
	return err;
}
// sequential acces base
VPictureDataProvider_SequentialAccess::VPictureDataProvider_SequentialAccess(sWORD inKind)
:VPictureDataProvider(inKind)
{
	fDirectAccesCache=0;
}	
VPictureDataProvider_SequentialAccess::~VPictureDataProvider_SequentialAccess()
{
	
}

VPtr VPictureDataProvider_SequentialAccess::DoBeginDirectAccess()
{
	VError err=VE_UNKNOWN_ERROR;
	if(_EnterCritical())
	{
		if(!fDirectAccesCache)
		{
			fDirectAccesCache=VMemory::NewPtr(fDataSize, 'pict');
			err=VMemory::GetLastError();
			if(err==VE_OK && fDirectAccesCache)
			{
				
				err=GetData(fDirectAccesCache,0,GetDataSize());
				if(err==VE_OK)
				{
					_Lock();
				}
				else
				{
					VMemory::DisposePtr(fDirectAccesCache);
					fDirectAccesCache=0;
				}
			}
		}
		else
		{
			_Lock();
			err=VE_OK;
		}
		_LeaveCritical();
	}
	SetLastError(err);
	return fDirectAccesCache;
}
VError VPictureDataProvider_SequentialAccess::DoEndDirectAccess()
{
	VError err=VE_UNKNOWN_ERROR;
	if(_EnterCritical())
	{
		if(fDirectAccesCache)
		{
			if(_Unlock()==0)
			{
				VMemory::DisposePtr(fDirectAccesCache);
				fDirectAccesCache=0;
			}
			err=VE_OK;
		}
		_LeaveCritical();
	}
	return SetLastError(err);
}
VError VPictureDataProvider_SequentialAccess::WriteToBlob(VBlob& inBlob,VIndex inOffset)
{
	VError err=VE_UNKNOWN_ERROR;
	VPtr dataptr=BeginDirectAccess();
	if(dataptr)
	{
		err=inBlob.PutData(dataptr,GetDataSize(),inOffset);
		EndDirectAccess();
	}
	return err;
}
// DATASOURCE
VPictureDataProvider_DS::VPictureDataProvider_DS(VPictureDataProvider* inParent,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider(kVDataSource)
{
	VError err=VE_UNKNOWN_ERROR;
	fDs=inParent;
	if(fDs && fDs->IsValid())
	{
		fDataOffset=inOffset;
		if(inNbBytes==0)
			inNbBytes=fDs->GetDataSize();
		fDataSize=inNbBytes;
		if(fDataSize+fDataOffset>fDs->GetDataSize())
		{
			fDataSize=0;
			fDataOffset=0;
			fDs=0;
		}
		else
		{
			fDs->Retain();
			err=VE_OK;
		}
	}
	_SetValid(err);
}
VPictureDataProvider_DS::~VPictureDataProvider_DS()
{
	if(fDs)
	{
		fDs->Release();
	}
}
VPtr VPictureDataProvider_DS::DoBeginDirectAccess()
{
	VPtr res=fDs->BeginDirectAccess();
	if(res)
	{
		res+=fDataOffset;
	}
	SetLastError(fDs->GetLastError());
	return res;
}
VError VPictureDataProvider_DS::DoEndDirectAccess()
{
	return SetLastError(fDs->EndDirectAccess());
}
VError VPictureDataProvider_DS::DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize)
{
	VError err;
	err=fDs->GetData(inBuffer, fDataOffset+inOffset, inSize,outSize);
	return SetLastError(err);
}
VError VPictureDataProvider_DS::WriteToBlob(VBlob& inBlob,VIndex inOffset)
{
	VError err;
	VPtr p=BeginDirectAccess();
	if(p)
	{
		p+=fDataOffset;
		err=inBlob.PutData(p,GetDataSize(),inOffset);
		EndDirectAccess();
	}
	else
		err=SetLastError(fDs->GetLastError());
	return err;
}

// VHANDLE
#if WITH_VMemoryMgr
VPictureDataProvider_VHandle::VPictureDataProvider_VHandle(VHandle inHandle,VSize inNbBytes, VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVHandle)
{
	_InitFromVHandle(inHandle,true,inNbBytes,inOffset);
}
VPictureDataProvider_VHandle::VPictureDataProvider_VHandle(sWORD inKind)
:VPictureDataProvider_DirectAccess(inKind)
{
	fOwned=true;
	fPtr=0;
	fHandle=0;
	
	_SetValid(VE_UNKNOWN_ERROR);
}

VPictureDataProvider_VHandle::VPictureDataProvider_VHandle(VStream& inStream,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVHandle)
{
	VHandle data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	
	fPtr=0;
	fHandle=0;
	
	if(inNbBytes==0)
		inNbBytes=inStream.GetSize();
	
	data=VMemory::NewHandle(inNbBytes);
	if(data)
	{
		VPtr p=VMemory::LockHandle(data);
		if(p)
		{
			err=inStream.SetPos(inOffset);
			if(err==VE_OK)
			{
				err=inStream.GetData(p,inNbBytes,&outsize);
				if(err==VE_OK && outsize==inNbBytes)
				{
					VMemory::UnlockHandle(data);
					_InitFromVHandle(data,true,inNbBytes,0);
				}
				else
				{
					VMemory::UnlockHandle(data);
					VMemory::DisposeHandle(data);
				}
			}
			else
			{
				VMemory::UnlockHandle(data);
				VMemory::DisposeHandle(data);
			}
		}
		else
			err=VMemory::GetLastError();
	}
	else
		err=VMemory::GetLastError();
	_SetValid(err);
	
}

void VPictureDataProvider_VHandle::_InitFromVBlob(const VBlob& inBlob,VSize inNbBytes,VIndex inOffset)
{
	VHandle data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	fOwned=true;
	fPtr=0;
	fHandle=0;
	
	if(inNbBytes==0)
		inNbBytes=inBlob.GetSize();
	
	data=VMemory::NewHandle(inNbBytes);
	
	if(data)
	{
		VPtr p=VMemory::LockHandle(data);
		if(p)
		{
			err=inBlob.GetData(p,inNbBytes,inOffset,&outsize);
			if(err==VE_OK && outsize==inNbBytes)
			{
				VMemory::UnlockHandle(data);
				_InitFromVHandle(data,true,inNbBytes,0);
			}
			else
			{
				VMemory::UnlockHandle(data);
				VMemory::DisposeHandle(data);
			}
		}
		else
			err=VMemory::GetLastError();
	}
	else
	{
		err=VMemory::GetLastError();
	}
	_SetValid(err);
}

VPictureDataProvider_VHandle::VPictureDataProvider_VHandle(const VBlob& inBlob,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVHandle)
{
	_InitFromVBlob(inBlob,inNbBytes,inOffset);
}

VPictureDataProvider_VHandle::VPictureDataProvider_VHandle(void* inDataPtr,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVHandle)
{
	VHandle data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	fOwned=true;
	fPtr=0;
	fHandle=0;
	
	data=VMemory::NewHandle(inNbBytes);
	
	if(data)
	{
		VPtr p=VMemory::LockHandle(data);
		if(p)
		{
			VMemory::CopyBlock(((char*)inDataPtr)+inOffset,p,inNbBytes);
			VMemory::UnlockHandle(data);
			_InitFromVHandle(data,true,inNbBytes,0);
			err=VE_OK;
		}
		else
			err=VMemory::GetLastError();
	}
	else
	{
		err=VMemory::GetLastError();
	}
	_SetValid(err);
}

VPictureDataProvider_VHandle::~VPictureDataProvider_VHandle()
{
	assert(_GetLockCount()==0);
	
	if(fOwned && !VMemory::IsLocked(fHandle))
	{
		VMemory::DisposeHandle(fHandle);
	}
	else
	{
		vThrowError(VE_HANDLE_LOCKED);
	}
}
void VPictureDataProvider_VHandle::_InitFromVHandle(VHandle inHandle,bool inOwnedHandle,VSize inNbBytes,VIndex inOffset)
{
	assert(inHandle);
	VSize hsize=VMemory::GetHandleSize(inHandle);
	
	fOwned=inOwnedHandle;
	fPtr=0;
	fHandle=inHandle;
	fDataSize=inNbBytes;
	fDataOffset=inOffset;
	if(fDataSize==0)
		fDataSize=hsize;
	if(fDataSize+fDataOffset>hsize)
	{
		_SetValid(VE_UNKNOWN_ERROR);
	}
	else
	{
		_SetValid(VE_OK);
	}
}
VPtr VPictureDataProvider_VHandle::DoBeginDirectAccess()
{
	VError err=VE_UNKNOWN_ERROR;
	_EnterCritical();
	if(!fPtr)
	{
		fPtr=VMemory::LockHandle(fHandle);
		err=VMemory::GetLastError();
		if(err==VE_OK)
		{
			fPtr+=fDataOffset;
			_Lock();
		}
	}
	else
	{
		_Lock();
		err=VE_OK;
	}
	SetLastError(err);
	_LeaveCritical();	
	return fPtr;
}	
VError VPictureDataProvider_VHandle::DoEndDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	_EnterCritical();
	if(!fPtr)
	{
		assert(false);
		result=VE_HANDLE_NOT_LOCKED;
	}
	else
	{
		if(_Unlock()==0)
		{
			VMemory::UnlockHandle(fHandle);
			fPtr=0;
		}
		result=VE_OK;	
	}
	_LeaveCritical();	
	return SetLastError(result);
}
#endif //WITH_VMemoryMgr


// MACHANDLE
#if !VERSION_LINUX
VPictureDataProvider_MacHandle::VPictureDataProvider_MacHandle(xMacHandle inHandle,VSize inNbBytes, VIndex inOffset)
:VPictureDataProvider_DirectAccess(kMacHandle)
{
	_InitFromMacHandle(inHandle,inNbBytes,inOffset);
}
VPictureDataProvider_MacHandle::VPictureDataProvider_MacHandle(const VHandle inHandle,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kMacHandle)
{
	xMacHandle data;
	VError err=VE_UNKNOWN_ERROR;

	fPtr=0;
	fHandle=0;
	
	if(inHandle)
	{
		VPtr p=VMemory::LockHandle(inHandle);
		if(p)
		{
			p+=inOffset;
			if(inNbBytes==0)
				inNbBytes=VMemory::GetHandleSize(inHandle);
			data=(xMacHandle)sMacAllocator->AllocateFromBuffer(p,inNbBytes);
			if(data)
			{
				_InitFromMacHandle(data,inNbBytes,0);
			}
			else
				err=sMacAllocator->MemError();
			
			VMemory::UnlockHandle(inHandle);
		}
		else
			err=VMemory::GetLastError();
	}

	_SetValid(err);
}
VPictureDataProvider_MacHandle::VPictureDataProvider_MacHandle(VStream& inStream,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kMacHandle)
{
	xMacHandle data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	
	fPtr=0;
	fHandle=0;
	
	if(inNbBytes==0)
		inNbBytes=inStream.GetSize();
	
	data=(xMacHandle)sMacAllocator->Allocate(inNbBytes);
	if(data)
	{
		sMacAllocator->Lock(data);
		if(sMacAllocator->MemError()==0)
		{
			VPtr p=*data;
			err=inStream.SetPos(inOffset);
			if(err==VE_OK)
			{
				err=inStream.GetData(p,inNbBytes,&outsize);
				if(err==VE_OK && outsize==inNbBytes)
				{
					sMacAllocator->Unlock(data);
					_InitFromMacHandle(data,inNbBytes,0);
				}
				else
				{
					sMacAllocator->Unlock(data);
					sMacAllocator->Free(data);
				}
			}
			else
			{
				sMacAllocator->Unlock(data);
				sMacAllocator->Free(data);
			}
		}
		else
			err=sMacAllocator->MemError();
	}
	else
		err=sMacAllocator->MemError();
	_SetValid(err);
	
}
VPictureDataProvider_MacHandle::VPictureDataProvider_MacHandle(const VBlob& inBlob,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kMacHandle)
{
	xMacHandle data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	
	fPtr=0;
	fHandle=0;
	
	if(inNbBytes==0)
		inNbBytes=inBlob.GetSize();
	
	data=(xMacHandle)sMacAllocator->Allocate(inNbBytes);
	
	if(data)
	{
		sMacAllocator->Lock(data);
		err=sMacAllocator->MemError();
		if(err==VE_OK)
		{
			VPtr p=*data;
			
			err=inBlob.GetData(p,inNbBytes,inOffset,&outsize);
			if(err==VE_OK && outsize==inNbBytes)
			{
				sMacAllocator->Unlock(data);
				_InitFromMacHandle(data,inNbBytes,0);
			}
			else
			{
				sMacAllocator->Unlock(data);
				sMacAllocator->Free(data);
			}
		}
		else
			err=sMacAllocator->MemError();
	}
	else
	{
		err=sMacAllocator->MemError();
	}
	_SetValid(err);
}
VPictureDataProvider_MacHandle::~VPictureDataProvider_MacHandle()
{
	assert(_GetLockCount()==0);
	if(fHandle)
	{
		sMacAllocator->Free(fHandle);
	}
}
void VPictureDataProvider_MacHandle::_InitFromMacHandle(xMacHandle inHandle,VSize inNbBytes,VIndex inOffset)
{
	VError err=VE_UNKNOWN_ERROR;
	assert(inHandle);
	VSize hsize=sMacAllocator->GetSize(inHandle);
	
	fHandle=inHandle;
	fPtr=0;
	fDataSize=inNbBytes;
	fDataOffset=inOffset;
	if(fDataSize==0)
		fDataSize=hsize;
	if(fDataSize+fDataOffset>hsize)
	{
		err=VE_UNKNOWN_ERROR;
	}
	else
	{
		err=VE_OK;
	}
	_SetValid(err);
}
VPtr VPictureDataProvider_MacHandle::DoBeginDirectAccess()
{
	VError err=VE_UNKNOWN_ERROR;
	_EnterCritical();
	if(!fPtr)
	{
		sMacAllocator->Lock(fHandle);
		fPtr=*fHandle;
		if(fPtr)
		{
			fPtr+=fDataOffset;
			_Lock();
		}
	}
	else
	{
		_Lock();
		err=VE_OK;
	}
	SetLastError(err);
	_LeaveCritical();	
	return fPtr;
}	
VError VPictureDataProvider_MacHandle::DoEndDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	_EnterCritical();
	if(!fPtr)
	{
		assert(false);
		result=VE_HANDLE_NOT_LOCKED;
	}
	else
	{
		if(_Unlock()==0)
		{
			sMacAllocator->Unlock(fHandle);
			fPtr=0;
		}
		result=VE_OK;	
	}
	_LeaveCritical();	
	return SetLastError(result);
}
#endif //VERSION_LINUX


// VPTR

VPictureDataProvider_VPtr::VPictureDataProvider_VPtr(VPtr inPtr,VSize inNbBytes, VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVPtr)
{
	fOwned=true;
	fPtr=0;
	fDataPtr=0;
	_InitFromVPtr(inPtr,true,inNbBytes,inOffset);
}
VPictureDataProvider_VPtr::VPictureDataProvider_VPtr(sWORD inKind)
:VPictureDataProvider_DirectAccess(inKind)
{
	fOwned=true;
	fPtr=0;
	fDataPtr=0;
	_SetValid(VE_UNKNOWN_ERROR);
}
VPictureDataProvider_VPtr::~VPictureDataProvider_VPtr()
{
	assert(_GetLockCount()==0);
	if(fOwned && fPtr)
	{
		VMemory::DisposePtr(fPtr);
	}
}
VPictureDataProvider_VPtr::VPictureDataProvider_VPtr(VStream& inStream,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVPtr)
{
	VPtr data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;
	
	fOwned=true;
	fPtr=0;
	fDataPtr=0;
	
	if(inNbBytes==0)
		inNbBytes=inStream.GetSize();
	
	data=VMemory::NewPtr(inNbBytes,'pict');
	if(data)
	{
		err=inStream.SetPos(inOffset);
		if(err==VE_OK)
		{
			err=inStream.GetData(data,inNbBytes,&outsize);
			if(err==VE_OK && outsize==inNbBytes)
			{
				_InitFromVPtr(data,true,inNbBytes,0);
			}
			else
			{
				VMemory::DisposePtr(data);
			}
		}
	}
	else
		err=VMemory::GetLastError();
	_SetValid(err);
	
}

VPictureDataProvider_VPtr::VPictureDataProvider_VPtr(const VBlob& inBlob,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_DirectAccess(kVPtr)
{
	_InitFromVBlob(inBlob,inNbBytes,inOffset);
}

void VPictureDataProvider_VPtr::_InitFromVBlob(const VBlob& inBlob,VSize inNbBytes,VIndex inOffset)
{
	VPtr data;
	VSize outsize;
	VError err=VE_UNKNOWN_ERROR;

	if(inNbBytes==0)
		inNbBytes=inBlob.GetSize();
	
	data=VMemory::NewPtr(inNbBytes,'pict');
	
	if(data)
	{
		err=inBlob.GetData(data,inNbBytes,inOffset,&outsize);
		if(err==VE_OK && outsize==inNbBytes)
		{
			_InitFromVPtr(data,true,inNbBytes,0);
		}
		else
		{
			VMemory::DisposePtr(data);
		}
	}
	else
	{
		fOwned=false;
		fPtr=0;
		fDataSize=0;
		fDataOffset=0;
		fDataPtr=0;
		err=VMemory::GetLastError();
	}
	_SetValid(err);
}

void VPictureDataProvider_VPtr::_InitFromVPtr(VPtr inPtr,bool inOwnedPtr,VSize inNbBytes,VIndex inOffset)
{
	assert(inPtr);
	assert(inNbBytes);
	
	// inNbBytes is a VSize so it is positive
	if(inPtr)
	{
		fOwned=inOwnedPtr;
		fPtr=inPtr;
		fDataSize=inNbBytes;
		fDataOffset=inOffset;
		
		fDataPtr=fPtr+fDataOffset;
		
		_SetValid(VE_OK);
	}
	else
	{
		fOwned=false;
		fPtr=0;
		fDataSize=0;
		fDataOffset=0;
		fDataPtr=0;
		_SetValid(VE_UNKNOWN_ERROR);
	}
}
VPtr VPictureDataProvider_VPtr::DoBeginDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataPtr)
	{
		_Lock();
		result=VE_OK;
	}
	SetLastError(result);
	return fDataPtr;
}	
VError VPictureDataProvider_VPtr::DoEndDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	if(fPtr)
	{
		_Unlock();
		result=VE_OK;
	}
	return SetLastError(result);
}


// VBLOBWITHPTR

VPictureDataProvider_VBlobWithPtr::VPictureDataProvider_VBlobWithPtr(const VBlobWithPtr* inBlob,VSize inNbBytes, VIndex inOffset)
:VPictureDataProvider_VPtr(kVBlobWithPtr)
{
	assert(inBlob);
	if(inBlob)
	{
		fBlob=inBlob;
		
		VSize hsize=fBlob->GetSize();
		VPtr data=(VPtr) fBlob->GetDataPtr();
		
		if(inNbBytes==0)
			inNbBytes=hsize;
		if(inNbBytes+inOffset>hsize)
		{
			_SetValid(VE_UNKNOWN_ERROR);
		}
		else
		{
			_InitFromVPtr(data,false,inNbBytes,inOffset);
		}
	}
	else
		_SetValid(VE_UNKNOWN_ERROR);
}
VPictureDataProvider_VBlobWithPtr::~VPictureDataProvider_VBlobWithPtr()
{
	assert(_GetLockCount()==0);	
	delete fBlob;
}
VPtr VPictureDataProvider_VBlobWithPtr::DoBeginDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataPtr)
	{
		_Lock();
		result=VE_OK;
	}
	SetLastError(result);
	return fDataPtr;
}	
VError VPictureDataProvider_VBlobWithPtr::DoEndDirectAccess()
{
	VError result=VE_UNKNOWN_ERROR;
	
	if(fDataPtr)
	{
		_Unlock();
		result=VE_OK;
	}
	return SetLastError(result);
}

// STREAM

VPictureDataProvider_VStream::VPictureDataProvider_VStream(VStream* inStream,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_SequentialAccess(kVStream)
{
	VError err=VE_UNKNOWN_ERROR;
	assert(inStream);

	fStream=inStream;

	fDataSize=inNbBytes;
	fDataOffset=inOffset;
	
	err=fStream->OpenReading();
	if(err==VE_OK)
	{
		VSize stsize=fStream->GetSize();
		if(fDataSize==0)
			fDataSize=stsize;
		if(fDataSize+fDataOffset>stsize)
		{
			err=VE_STREAM_EOF;
		}
		else
		{
			err=VE_OK;
		}
	}
	_SetValid(err);
}
VPictureDataProvider_VStream::~VPictureDataProvider_VStream()
{
	assert(!_GetLockCount());
	if(fStream)
		fStream->CloseReading();
	if(fStream)
	{
		delete fStream;
	}
}
	
VError VPictureDataProvider_VStream::DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize)
{
	VError err=VE_UNKNOWN_ERROR;
	if(_EnterCritical())
	{
		if(outSize)
			*outSize=0;
		sLONG8	oldpos=fStream->GetPos();
		err=fStream->SetPos(fDataOffset+inOffset);
		if(err==VE_OK)
		{
			VSize s=inSize;
			err=fStream->GetData(inBuffer,&s);
			if(outSize)
				*outSize=inSize;
		}
		fStream->SetPos(oldpos);
		_LeaveCritical();
	}
	return err;
}


// VFILE seq access

VPictureDataProvider_VFile::VPictureDataProvider_VFile(const VFile& inFile,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_SequentialAccess(kVFile)
{
	VError err;
	fDesc=0;
	if(inFile.Exists())
	{
		err=inFile.Open(FA_READ,&fDesc);
		if(err==VE_OK)
		{
			VSize stsize=fDesc->GetSize();
			fDataSize=inNbBytes;
			fDataOffset=inOffset;
			if(fDataSize==0)
				fDataSize=stsize;
			if(fDataSize+fDataOffset>stsize)
			{
				_SetValid(VE_STREAM_EOF);
			}
			else
				_SetValid(VE_OK);
		}
		else
		{
			_SetValid(err);
			fDesc=0;
		}
	}
	else
	{
		_SetValid(new VFileError(&inFile,VE_FILE_NOT_FOUND,VNE_OK));
	}
}
	
VPictureDataProvider_VFile::~VPictureDataProvider_VFile()
{
	assert(!_GetLockCount());
	
	if(fDesc)
		delete fDesc;

}
	
VError VPictureDataProvider_VFile::DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize)
{
	VError err=VE_UNKNOWN_ERROR;
	if(_EnterCritical())
	{
		err=fDesc->GetData(inBuffer,inSize,fDataOffset+inOffset,outSize);
		_LeaveCritical();
	}
	return err;
}

bool VPictureDataProvider_VFile::GetFilePath(VFilePath& outPath)
{
	outPath.Clear();
	if(fDesc)
	{
		const VFile*	f=fDesc->GetParentVFile();
		if(f)
		{
			outPath.FromFilePath(f->GetPath());
			return true;
		}
	}
	return false;
}

// VFILE in mem

VPictureDataProvider_VFileInMem::VPictureDataProvider_VFileInMem(const VFile& inFile,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_VPtr(kVFile)
{
	fPath.FromFilePath(inFile.GetPath());
	if(inFile.Exists())
	{
		VFileDesc*		Desc;
		VError err=inFile.Open(FA_READ,&Desc);
		if(err==VE_OK)
		{
			VSize stsize=Desc->GetSize();
			if(stsize>0)
			{
				VPtr p=VMemory::NewPtr(stsize,'pict');
				if(p)
				{
					
					VSize outSize;
					err=Desc->GetData(p,stsize,0,&outSize);
					if(err==VE_OK)
					{
						_InitFromVPtr(p,true,outSize,inOffset);
					}
					else
					{
						VMemory::DisposePtr(p);
					}
				}
				else
				{
					err=VMemory::GetLastError();
				}
			}
			delete Desc;
		}
		_SetValid(err);
	}
	else
	{
		_SetValid(new VFileError(&inFile,VE_FILE_NOT_FOUND,VNE_OK));
	}
}

VPictureDataProvider_VFileInMem::VPictureDataProvider_VFileInMem(const VFilePath& inPath,const VBlob& inBlob)
:VPictureDataProvider_VPtr(kVFile)
{
	_InitFromVBlob(inBlob,0,0);
	fPath=inPath;
}
	
VPictureDataProvider_VFileInMem::~VPictureDataProvider_VFileInMem()
{
	
}

// BLOB

VPictureDataProvider_VBlob::VPictureDataProvider_VBlob(const VBlob* inBlob,VSize inNbBytes,VIndex inOffset)
:VPictureDataProvider_SequentialAccess(kVBlob)
{
	VError err=VE_UNKNOWN_ERROR;
	assert(inBlob);
	
	fBlob=inBlob;
	
	fDataSize=inNbBytes;
	fDataOffset=inOffset;
	
	if(fBlob)
	{
		VSize stsize=fBlob->GetSize();
		if(fDataSize==0)
			fDataSize=stsize;
		if(fDataSize+fDataOffset>stsize)
		{
			err=VE_STREAM_EOF;
		}
		else
		{
			err=VE_OK;
		}
	}
	
	_SetValid(err);
	
}
VPictureDataProvider_VBlob::~VPictureDataProvider_VBlob()
{
	assert(!_GetLockCount());
	
	if(fBlob)
	{
		delete fBlob;
	}
}
	
VError VPictureDataProvider_VBlob::DoGetData(void* inBuffer, VIndex inOffset, VSize inSize,VSize* outSize)
{
	VError err=VE_UNKNOWN_ERROR;
	if(_EnterCritical())
	{
		err=fBlob->GetData(inBuffer,inSize,fDataOffset+inOffset,outSize);
		_LeaveCritical();
	}
	return err;
}


/***********************************************************************************/

VPictureDataStream::VPictureDataStream( VPictureDataStream* inStream )
{
	assert(inStream);
	if(inStream->fDataSource)
	{
		fDataSource=inStream->fDataSource;
		fDataSource->Retain();
		fLogSize=fDataSource->GetDataSize64();
	}
	else
	{
		fDataSource=0;
		fLogSize=0;
	}
}

VPictureDataStream::VPictureDataStream(VPictureDataProvider* inDS)
{
	assert(inDS);
	if(inDS)
	{
		fDataSource=inDS;
		fDataSource->Retain();
		fLogSize=fDataSource->GetDataSize64();
	}
	else
	{
		fDataSource=0;
		fLogSize=0;
	}
}


VPictureDataStream::~VPictureDataStream()
{
	if (fDataSource) 
	{
		fDataSource->Release();
	}
}


VError VPictureDataStream::DoOpenWriting()
{
	return VE_STREAM_CANNOT_WRITE;
}

VError VPictureDataStream::DoOpenReading()
{
	if (fDataSource == NULL)
		return vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);
	return VE_OK;
}


VError VPictureDataStream::DoCloseReading()
{
	return VE_OK;
}


VError VPictureDataStream::DoCloseWriting(Boolean /*inSetSize*/)
{
	return VE_OK;
}


VError VPictureDataStream::DoSetSize(sLONG8 /*inNewSize*/)
{
	return VE_STREAM_CANNOT_SET_SIZE;
}


VError VPictureDataStream::DoPutData(const void* /*inBuffer*/, VSize /*inNbBytes*/)
{
	return VE_STREAM_CANNOT_PUT_DATA;
}


VError VPictureDataStream::DoGetData(void* inBuffer, VSize* ioCount)
{
	VError err = VE_OK;
	if(fDataSource)
	{
		sLONG8 pos = GetPos();
		
		if (testAssert(pos <= fLogSize)) 
		{	// caller should have checked this already
			if (pos + *ioCount > fLogSize) 
			{
				err = vThrowError(VE_STREAM_EOF);
				*ioCount = (VSize) (fLogSize - pos);
			}
			err=fDataSource->GetData(inBuffer,pos,*ioCount,ioCount);
		}
	}
	else
		err=vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);
	return err;
}


sLONG8 VPictureDataStream::DoGetSize()
{
	sLONG8 result=0;
	if(fDataSource)
	{
		result= fDataSource->GetDataSize();
	}
	else
	{
		vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);
	}
	return result;
}


VError VPictureDataStream::DoSetPos(sLONG8 inNewPos)
{
	VError err = VE_OK;
	if(fDataSource)
	{
		if (fLogSize < inNewPos)
			err=VE_STREAM_EOF;
	}
	else
	{
		vThrowError(VE_STREAM_CANNOT_FIND_SOURCE);
	}
	return err;
}

/***************************************************************/
// debug
/**************************************************************/


/***************************************************************/
/**************************************************************/

xV4DPictureBlobRef::xV4DPictureBlobRef(VBlob* inBlob,uBOOL inReadOnly)
{
	fReadOnly=inReadOnly;
	fBlob=inBlob;
}
xV4DPictureBlobRef::xV4DPictureBlobRef( const xV4DPictureBlobRef& inBlob)
{
	fBlob=(VBlob*)inBlob.fBlob->Clone();
}
xV4DPictureBlobRef* xV4DPictureBlobRef::ExchangeBlob()
{
	xV4DPictureBlobRef* result=new xV4DPictureBlobRef(fBlob);
	fBlob=(VBlob*)fBlob->Clone();
	return result;
};
VValue* xV4DPictureBlobRef::FullyClone(bool ForAPush) const
{
	if(fBlob)
		return fBlob->FullyClone(ForAPush);
	else
		return NULL;;
}
void xV4DPictureBlobRef::RestoreFromPop(void* context)
{
	if(fBlob)
		fBlob->RestoreFromPop(context);
}
void xV4DPictureBlobRef::Detach(Boolean inMayEmpty)
{
	if(fBlob)
		fBlob->Detach(inMayEmpty);
}
xV4DPictureBlobRef::~xV4DPictureBlobRef()
{
	if(fBlob)
		delete fBlob;
}
VSize	xV4DPictureBlobRef::GetSize () const
{
	assert(fBlob);
	return fBlob->GetSize();
}
VError	xV4DPictureBlobRef::SetSize (VSize inNewSize)
{
	assert(fBlob);
	if(!fReadOnly)
		return fBlob->SetSize(inNewSize);
	else
		return VE_STREAM_CANNOT_WRITE;
}
VError	xV4DPictureBlobRef::GetData (void* inBuffer, VSize inNbBytes, VIndex inOffset, VSize* outNbBytesCopied) const
{
	assert(fBlob);
	return fBlob->GetData ( inBuffer, inNbBytes, inOffset, outNbBytesCopied);
}
VError	xV4DPictureBlobRef::PutData (const void* inBuffer, VSize inNbBytes, VIndex inOffset)
{
	VError result;
	assert(fBlob);
	
	if(!fReadOnly)
	{
		result=fBlob->PutData (inBuffer, inNbBytes, inOffset);
		return result;
	}
	else
		return VE_STREAM_CANNOT_WRITE;
}
VError	xV4DPictureBlobRef::Flush( void* inDataPtr, void* inContext) const
{
	assert(fBlob);
	if(!fReadOnly)
		return fBlob->Flush (inDataPtr, inContext);
	else
		return VE_STREAM_CANNOT_WRITE;
}
void xV4DPictureBlobRef::SetReservedBlobNumber(sLONG inBlobNum)
{
	assert(fBlob);
	fBlob->SetReservedBlobNumber(inBlobNum);

}
void*	xV4DPictureBlobRef::LoadFromPtr (const void* inDataPtr, Boolean inRefOnly)
{
	assert(fBlob);
	if(!fReadOnly)
		return fBlob->LoadFromPtr (inDataPtr, inRefOnly );
	else
		return NULL;
}
void*	xV4DPictureBlobRef::WriteToPtr (void* inDataPtr, Boolean inRefOnly) const
{
	assert(fBlob);
	return fBlob->WriteToPtr (inDataPtr, inRefOnly);
}
VSize	xV4DPictureBlobRef::GetSpace () const
{
	assert(fBlob);
	return fBlob->GetSpace ();
}
bool	xV4DPictureBlobRef::IsNull() const	
{
	assert(fBlob);
	return fBlob->IsNull();
}

#if VERSIONWIN

HRESULT VPictureDataProvider_Stream::Ensure(ULONG nNewData)
{
    if (nNewData > m_nData)
    {
        // apply some heurestic for growing
        ULONG n = m_nData;

        // grow 2x for smaller sizes, 1.25x for bigger sizes
        n = min(2 * n, n + n / 4 + 0x100000);

        // don't allocate tiny chunks
        n = max(n, 0x100);

        // compare with the hard limit
        nNewData = max(n, nNewData);
    }
    else
    if (nNewData > m_nData / 4)
    {
        // shrinking but it is not worth it
        return S_OK;
    }

    BYTE * pNewData = (BYTE*)realloc(m_pData, nNewData);
    if (pNewData == NULL && nNewData != 0)
        return E_OUTOFMEMORY;

    m_nData = nNewData;
    m_pData = pNewData;
    return S_OK;
}

VPictureDataProvider_Stream::VPictureDataProvider_Stream(VPictureDataProvider *inDataSource)
{
    m_cRef = 1;
    m_nPos = 0;
    m_nData = 0;
    m_pData = NULL;
    fDataSource=inDataSource;
    fDataSource->Retain();
    m_nSize = (ULONG) inDataSource->GetDataSize();
    m_nData = m_nSize;
}

VPictureDataProvider_Stream::~VPictureDataProvider_Stream()
{
    fDataSource->Release();
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::QueryInterface( REFIID riid,void **ppvObject)
{
    if (riid == IID_IStream || 
        riid == IID_ISequentialStream ||
        riid == IID_IUnknown)
    {
        InterlockedIncrement(&m_cRef);
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}
    
ULONG STDMETHODCALLTYPE VPictureDataProvider_Stream::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}
    
ULONG STDMETHODCALLTYPE VPictureDataProvider_Stream::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    ULONG nData;
    ULONG nNewPos = m_nPos + cb;

    // check for overflow
    if (nNewPos < cb)
        return STG_E_INVALIDFUNCTION;

    // compare with the actual size
    nNewPos = min(nNewPos, m_nSize);

    // compare with the data available
    nData = min(nNewPos, m_nData);

    // copy the data over
   
    if (nData > m_nPos)
		fDataSource->GetData(pv,m_nPos,nData - m_nPos);
		
    // fill the rest with zeros
    if (nNewPos > nData)
        memset((BYTE*)pv + (nData - m_nPos), 0, nNewPos - nData);


    cb = nNewPos - m_nPos;
    m_nPos = nNewPos;

    if (pcbRead)
        *pcbRead = cb;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Write(const void * /*pv*/,ULONG /*cb*/,ULONG* /*pcbWritten*/)
{
   
   /* ULONG nNewPos = m_nPos + cb;

    // check for overflow
    if (nNewPos < cb)
        return STG_E_INVALIDFUNCTION;

    // ensure the space
    if (nNewPos > m_nData)
    {
        HRESULT hr = Ensure(nNewPos);
        if (FAILED(hr)) return hr;
    }

    // copy the data over
    memcpy(m_pData + m_nPos, pv, cb);

    m_nPos = nNewPos;
    if (m_nPos > m_nSize)
        m_nSize = m_nPos;

    if (pcbWritten)
        *pcbWritten = cb;

    return S_OK;
    */
    xbox_assert(false);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{       
	ULONG           lStartPos;
    LONGLONG        lNewPos;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        lStartPos = 0;
        break;
    case STREAM_SEEK_CUR:
        lStartPos = m_nPos;
        break;
    case STREAM_SEEK_END:
        lStartPos = m_nSize;
        break;
    default:
        return STG_E_INVALIDFUNCTION;
    }

    lNewPos = lStartPos + dlibMove.QuadPart;

    // it is an error to seek before the beginning of the stream
	// we are in read only mode, so it is not possoble to seek past the end of the stream
	if (lNewPos < 0 || lNewPos > m_nSize)
        return STG_E_INVALIDFUNCTION;

    // It is not, however, an error to seek past the end of the stream
#if 0 // pp read only
	if (lNewPos > m_nSize)
    {
		/* ULARGE_INTEGER NewSize;
        NewSize.QuadPart = lNewPos;

        HRESULT hr = SetSize(NewSize);
        if (FAILED(hr)) return hr;*/
    }
#endif
    m_nPos = (ULONG)lNewPos;

    if (plibNewPosition != NULL)
        plibNewPosition->QuadPart = m_nPos;

    return S_OK;
    
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::SetSize(ULARGE_INTEGER /*libNewSize*/)
{
    /*
    if (libNewSize.u.HighPart != 0)
        return STG_E_INVALIDFUNCTION;

    m_nSize = libNewSize.u.LowPart;

    // free the space if we are shrinking
    if (m_nSize < m_nData)
        Ensure(m_nSize);

    return S_OK;
    */
	
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
    HRESULT res;
    uLONG read=0,write=0;
    char* buff=new char[cb.QuadPart];
    res=Read(buff,cb.QuadPart,&read);
    if(res==S_OK)
    {
	res = pstm->Write(buff,read,&write);
    }
    if(pcbRead)
	pcbRead->QuadPart=read;
	if(pcbWritten)
		pcbWritten->QuadPart=write;
	delete[] buff;
   return res;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Commit(DWORD /*grfCommitFlags*/)
{
	xbox_assert(false);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Revert()
{
	xbox_assert(false);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::LockRegion(ULARGE_INTEGER /*libOffset*/,ULARGE_INTEGER /*cb*/,DWORD /*dwLockType*/)
{
	xbox_assert(false);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::UnlockRegion(ULARGE_INTEGER /*libOffset*/,ULARGE_INTEGER /*cb*/,DWORD /*dwLockType*/)
{
	xbox_assert(false);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Stat(STATSTG *pstatstg,DWORD /*grfStatFlag*/)
{
    memset(pstatstg, 0, sizeof(STATSTG));
    pstatstg->cbSize.QuadPart = m_nSize;
    return S_OK;
}
 
HRESULT STDMETHODCALLTYPE VPictureDataProvider_Stream::Clone(IStream** /*ppstm*/)
{
	xbox_assert(false);
	return E_NOTIMPL;
}
 

#endif

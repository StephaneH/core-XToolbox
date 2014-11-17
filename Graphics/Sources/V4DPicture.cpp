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
#include "Kernel/Sources/VBlob.h"
#include "V4DPictureIncludeBase.h"

#if VERSIONDEBUG
	#define debug_pict_mem 0
#else
	#define debug_pict_mem 0
#endif

namespace VPicturesBagKeys
{
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( vers,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peX,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peY,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( peMode,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m0,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m1,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m2,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m3,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m4,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m5,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m6,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m7,VReal,GReal,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( m8,VReal,GReal,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frSplit,VLong,sLONG,0);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frH,VLong,sLONG,1);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( frV,VLong,sLONG,1);
	CREATE_BAGKEY(settings);
};

const VPicture_info	VPicture::sInfo;

void VPicture::Init(VMacHandleAllocatorBase* inMacAllocator)
{
	VPictureCodecFactory::Init();
	XBOX::VPictureData::InitMemoryAllocator(inMacAllocator);
	XBOX::VPictureDataProvider::InitMemoryAllocator(inMacAllocator);
}
void VPicture::Deinit()
{
	VPictureCodecFactory::Deinit();
	XBOX::VPictureData::DeinitStatic();
	XBOX::VPictureDataProvider::DeinitStatic();
}

void VPicture::InitQDBridge(VPictureQDBridgeBase* inQDBridge)
{
	XBOX::VPictureData::InitQDBridge(inQDBridge);
}


/**********************************************************/
/************************** value info *****************/

VValue *VPicture_info::Generate() const
{
	return new VPicture();
}

VValue*	VPicture_info::Generate(VBlob* inBlob) const 
{
	return new VPicture(inBlob);
}
VValue *VPicture_info::LoadFromPtr( const void *inBackStore, bool /*inRefOnly*/) const
{
	VPtr databuff=(VPtr)(reinterpret_cast<const uLONG*>(inBackStore))+1;
	VSize len=*((uLONG*)inBackStore);
	VPicture *pict=new VPicture();

	pict->FromPtr((void*)databuff,len);
	
	return pict;
}


void* VPicture_info::ByteSwapPtr( void *inBackStore, bool inFromNative) const
{
	sLONG lenblob = *(sLONG*)inBackStore;
	if (!inFromNative)
		ByteSwap(&lenblob);

	ByteSwap((sLONG*)inBackStore);
	if (lenblob <= -10)
	{
		lenblob = -lenblob - 10;
		return ((uBYTE*)inBackStore) + 4 + lenblob;
	}
	else
		return ((sLONG*)inBackStore)+1;
}


CompareResult VPicture_info::CompareTwoPtrToData( const void* /*inPtrToValueData1*/, const void* /*inPtrToValueData2*/, Boolean /*inDiacritical*/) const
{
	return CR_UNRELEVANT;
}


CompareResult VPicture_info::CompareTwoPtrToDataBegining( const void* /*inPtrToValueData1*/, const void* /*inPtrToValueData2*/, Boolean /*inDiacritical*/) const
{
	return CR_UNRELEVANT;
}


VSize VPicture_info::GetSizeOfValueDataPtr( const void* inPtrToValueData) const
{
	return sizeof(sLONG) + (*((sLONG*)inPtrToValueData));
}

Boolean	VPicture::EqualToSameKind( const VValue* inValue, Boolean inDiacritical) const
{
	Boolean result=false;
	
	if(inValue && inValue->GetValueKind()==VK_IMAGE)
	{
		const VPicture* comparewith=dynamic_cast<const VPicture*>(inValue);
		if(comparewith)
		{
			if ( IsPictEmpty ( ) && comparewith-> IsPictEmpty ( ) )
			{
				result = true;
			}
			else if(fVValueSource && comparewith->fVValueSource)
			{
				if(fVValueSource->GetSize() == comparewith->fVValueSource->GetSize())
				{
					
					VSize bufferSize=1024*1024;
					XBOX::VBlob *b1,*b2;
					void *buff1 = XBOX::vMalloc(bufferSize,'BUFF');
					
					
					b1=fVValueSource;
					b2=comparewith->fVValueSource;
					
					if(buff1)
					{
						void *buff2 = XBOX::vMalloc(bufferSize,'BUFF');
						if(buff2)
						{
							VError err =  VE_OK;
							VSize tot=b1->GetSize();
							VIndex pos=0;
							int equal=0;
							while( (tot > 0) && (err == VE_OK) && (equal==0))
							{
								VSize chunkSize = (tot > bufferSize) ? bufferSize : tot;
								
								err = b1->GetData(buff1,chunkSize,pos);
								if(err==VE_OK)
								{
									err = b2->GetData(buff2,chunkSize,pos);
									if(err==VE_OK)
									{
										equal = memcmp(buff1,buff2,chunkSize);
										pos+=chunkSize;
										tot-=chunkSize;
									}
								}
							}
							result = (equal==0);
							XBOX::vFree(buff1);
							XBOX::vFree(buff2);
						}
						else
						{
							XBOX::vFree(buff1);
							vThrowError(VE_MEMORY_FULL);
						}
					}
					else
					{
						vThrowError(VE_MEMORY_FULL);
					}
				}
			}
		}
	}
	return result;
}

VError VPicture::ReloadFromOutsidePath()
{
	VError err=VE_OK;
	if(fVValueSource)
	{
		err=fVValueSource->ReloadFromOutsidePath();
		if(err==VE_OK)
			err= _LoadFromVValueSource();
	}
	return err;
}

void VPicture::SetHintRecNum(sLONG TableNum, sLONG FieldNum, sLONG inHint)
{
	if (fVValueSource != NULL)
		fVValueSource->SetHintRecNum(TableNum, FieldNum, inHint);
}


/*******************************************************/

#if VERSIONDEBUG
sLONG VPicture::sCountPictsInMem = 0;
#endif

void VPicture::_ResetExtraInfo()
{
	fDrawingSettings.Reset();
}
void VPicture::_CopyExtraInfo(const VPicture& inPict)
{
	fDrawingSettings.FromVPictureDrawSettings(inPict.GetSettings());	
}
void VPicture::_Init(bool inSetNULL)
{
#if debug_pict_mem
	++sCountPictsInMem;
	DebugMsg("new pict , Nb Picts in Mem : "+ToString(sCountPictsInMem)+"\n");
#endif
	fIs4DBlob=false;
	
	fVValueSource=NULL;
	fVValueSourceLoaded=false;
	fVValueSourceDirty=false;
		
	fExtraData=NULL;
	fInCVCache=false;
	fBestForDisplay=0;
	fBestForPrinting=0;
	_ResetExtraInfo();
	if(inSetNULL)
		SetNull(true);
}
VPicture::VPicture()
{
	_Init(true);
}
VPicture::VPicture(VStream* inStream,_VPictureAccumulator* inRecorder)
{
	_Init();
	_FromStream(inStream,inRecorder);
}
VPicture::VPicture(VBlob* inBlob)
{
	_Init();
	
	_AttachVValueSource(inBlob);

	_LoadFromVValueSource();
}

VPicture::VPicture(const VPictureData* inData)
{
	_Init();
	
	FromVPictureData(inData);
}

VPicture::VPicture(const VPicture& inData)
{
	_Init();
	
	FromVPicture_Retain(inData,false);
}

VPicture::VPicture( const VFile& inFile,bool inAllowUnknownData)
{
	_Init();
	
	FromVFile(inFile,inAllowUnknownData);
}

#if !VERSION_LINUX
VPicture::VPicture(PortRef inPortRef,VRect inBounds)
{
	_Init(true);
	VPictureData* data=VPictureData::CreatePictureDataFromPortRef(inPortRef,inBounds);
	if(data)
	{
		AppendPictData(data);
		fBestForDisplay=data;
		fBestForPrinting=data;
		data->Release();
	}
}
#endif

VPicture::~VPicture()
{
#if debug_pict_mem
	--sCountPictsInMem;
	DebugMsg("del pict , Nb Picts in Mem : "+ToString(sCountPictsInMem)+"\n");
#endif
	_DetachVValueSource(false);
	_ClearMap();
	_ReleaseExtraData();
}

VPicture& VPicture::operator=(const VPicture& inPict)
{
	FromVPicture_Retain(inPict,false);
	return *this;
}

void VPicture::FromVPictureData(const VPictureData* inData)
{
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	if(inData)
	{
		if(inData->IsKind(sCodec_4pct))
		{
			const VPictureData_VPicture* container=static_cast<const VPictureData_VPicture*>(inData);
			const VPicture* pict=container->GetVPicture();
			if(pict)
			{
				FromVPicture_Retain(*pict,false);
			}
		}
		else
		{
			fBestForDisplay=inData;
			fBestForPrinting=inData;
	
			fPictDataMap[inData->GetEncoderID()]=inData;
			inData->Retain();
			_SelectDefault();
		}
	}

	_SetValueSourceDirty(true);
	_UpdateNullState();
}


void VPicture::SetEmpty(bool inKeepSettings)
{
	_DetachVValueSource();

	_ReleaseExtraData();
	_ClearMap();
	fIs4DBlob=false;
	fBestForDisplay=0;
	fBestForPrinting=0;
	if(!inKeepSettings)
	{
		_ResetExtraInfo();
		fSourceFileName.Clear();
	}
}

VPicture*	VPicture::EncapsulateBlob(const VBlob* inBlob,bool inOwnData)
{
	VPicture* result=NULL;
	VPictureDataProvider *dataprovider=VPictureDataProvider::Create(inBlob,inOwnData);
	if(dataprovider)
	{
		VPictureCodecFactoryRef fact;
		const VPictureCodec* codec=fact->RetainEncoderByIdentifier(sCodec_blob);
		if(codec)
		{	
			VPictureData* pdata;
			pdata = codec->CreatePictDataFromDataProvider(*dataprovider);
			if(pdata)
			{
				result=new VPicture(pdata);
				pdata->Release();
			}
		}
		dataprovider->Release();
	}
	return result;
}
VError VPicture::ExtractBlob(VBlob& outBlob)
{
	VError result=VE_UNKNOWN_ERROR;
	
	outBlob.SetSize(0);
	const VPictureData* pdata=RetainPictDataByIdentifier(sCodec_blob);
	if(pdata)
	{
		VSize outSize;
		result = pdata->Save(&outBlob,0,outSize);
		pdata->Release();
	}
	return result;
}


void VPicture::GetKeywords( VString& outKeywords)
{
	outKeywords.Clear();
	const VPictureData* picdata = RetainNthPictData(1);
	if (picdata != NULL)
	{
		const VValueBag* metadata = picdata->RetainMetadatas();
		if (metadata != NULL)
		{
			const VValueBag* iptcbag = metadata->GetUniqueElement( CVSTR("IPTC"));
			if (iptcbag != NULL)
			{
				iptcbag->GetString( CVSTR("Keywords"), outKeywords);
			}
		}
		ReleaseRefCountable( &metadata);
	}
	ReleaseRefCountable( &picdata);
}

VError VPicture::ImportFromStream (VStream& ioStream) 
{
	VError err=VE_INVALID_PARAMETER;
	
	Clear();
	
	VPictureDataProvider* dataprovider=VPictureDataProvider::Create(ioStream);
	
	if(!dataprovider)
	{
		err=VTask::GetCurrent()->GetLastError();
	}
	else
	{
		VPictureCodecFactoryRef fact;
		VPictureData* data=fact->CreatePictureData(*dataprovider,NULL);
		if(data)
		{
			FromVPictureData(data);
			data->Release();
			err=VE_OK;
		}
		else
		{
			err=VE_INVALID_PARAMETER;
		}
		dataprovider->Release();	
	}
	return err;
}

void VPicture::FromVFile( const VFile& inFile,bool inAllowUnknownData)
{
	VPictureCodecFactoryRef fact;
	VPictureData* data;
	VString filename;
	
	inFile.GetPath().GetFileName(filename);
	
	const VPictureCodec* decoder=fact->RetainDecoder(inFile,true);
	
	if(decoder)
	{
		data=decoder->CreatePictDataFromVFile(inFile);
		
		FromVPictureData(data);
		
		SetSourceFileName(filename);
		
		if(data)
			data->Release();
		decoder->Release();
	}
	else
	{
		if(inAllowUnknownData)
		{
			VString ext;
			inFile.GetExtension(ext);
			if(!ext.IsEmpty())
			{
				ext="."+ext;
				decoder=fact->RetainDecoderByIdentifier(ext);
				if(!decoder)
				{
					decoder=fact->RegisterAndRetainUnknownDataCodec(ext);
					if(decoder)
					{
						data=decoder->CreatePictDataFromVFile(inFile);
						FromVPictureData(data);
						if(data)
							data->Release();
						decoder->Release();
						SetSourceFileName(filename);
					}
					else
					{
						SetEmpty();
					}
				}
				else
				{
					// pp le decoder exist, mais il n'a pas valider le data. Donc image vide
					SetEmpty();
				}
			}
		}
		else
			SetEmpty();
	}
}

void VPicture::SetExtraData(const VBlob& inBlob)
{
	if(fExtraData)
		fExtraData->Release();
	
	fExtraData=VPictureDataProvider::Create(inBlob);
	_UpdateNullState();
	_SetValueSourceDirty();
}
VSize VPicture::GetExtraData(VBlob& outBlob)const
{
	outBlob.SetSize(0);
	if(fExtraData)
	{
		fExtraData->WriteToBlob(outBlob,0);
	}
	return outBlob.GetSize();
}
VSize VPicture::GetExtraDataSize()const
{
	VSize result=0;
	if(fExtraData)
	{
		result = fExtraData->GetDataSize();
	}
	return result;
}
void VPicture::ClearExtraData()
{
	_ReleaseExtraData();
	_SetValueSourceDirty();
}
void VPicture::_ReleaseExtraData()
{
	if(fExtraData)
	{
		fExtraData->Release();
		fExtraData=0;
	}	
}
void VPicture::_CopyExtraData_Copy(const VPicture& inPict)
{
	_ReleaseExtraData();
	if(inPict.fExtraData)
	{
		VPtr data=inPict.fExtraData->BeginDirectAccess();
		if(data)
		{
			VPtr copy = VMemory::NewPtr(inPict.fExtraData->GetDataSize(),'pict');
			if(copy)
			{
				memcpy(copy,data,inPict.fExtraData->GetDataSize());
				fExtraData=new VPictureDataProvider_VPtr(copy,inPict.fExtraData->GetDataSize(),0); 
			}
			else
				vThrowError(VMemory::GetLastError());
				
			inPict.fExtraData->EndDirectAccess();
		}
	}
}
void VPicture::_CopyExtraData_Retain(const VPicture& inPict)
{
	_ReleaseExtraData();
	if(inPict.fExtraData)
	{
		fExtraData=inPict.fExtraData;
		fExtraData->Retain();
	}
}
void VPicture::_ReadHeaderV1(xVPictureBlockHeaderV1& /*inHeader*/,bool /*inSwap*/)
{
}
/*
void VPicture::_ReadHeaderV2(xVPictureBlockHeaderV2& inHeader,bool inSwap)
{
	if(inSwap)
	{
		ByteSwapWordArray(&inHeader.fPicEnd_PosX,3);
	}
					
	fPicEnd_PosX=inHeader.fPicEnd_PosX; // old picend
	fPicEnd_PosY=inHeader.fPicEnd_PosY;
	fPicEnd_TransfertMode=inHeader.fPicEnd_TransfertMode;
}
*/

VError VPicture::_SaveExtensions(VPtrStream& outStream) const
{
	VError err;
	if(!fPictDataMap.size())
	{
		outStream.Clear();
		return VE_OK;
	}

	outStream.SetLittleEndian();
	err=outStream.OpenWriting();
	if(vThrowError(err)==VE_OK)
	{
		for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end() && err==VE_OK;it++)
		{
			if((*it).second)
			{
				VString ext=(*it).second->GetEncoderID();
				err=ext.WriteToStream(&outStream);
				vThrowError(err);
			}
		}
		err=vThrowError(outStream.CloseWriting(true));
	}
	return err;
}

void	VPicture::SetSourceFileName(const VString& inFileName)
{
	_SetValueSourceDirty();
	fSourceFileName=inFileName;	
}
const VString&	VPicture::GetSourceFileName() const
{
	return fSourceFileName;
}

VError VPicture::_SaveExtraInfoBag(VPtrStream& outStream) const
{
	VError err = VE_OK;
	VString outname;
	
	VValueBag bag;
	
	fDrawingSettings.SaveToBag(bag,outname);
	
	if(!fSourceFileName.IsEmpty())
		bag.SetString(CVSTR("SourceFineName"),fSourceFileName);
	
	if(!bag.IsEmpty())
	{
		outStream.SetLittleEndian();
		err=outStream.OpenWriting();
		if(vThrowError(err)==VE_OK)
		{
			err=bag.WriteToStream(&outStream);
			if(vThrowError(err)==VE_OK)
				err=vThrowError(outStream.CloseWriting(true));
		}
	}
	return err;	
}

void VPicture::_ReadExtraInfoBag(VValueBag& inBag)
{
	GReal mat[9];
	sLONG	l;
	GReal	r;
	
	
	if(inBag.GetLong(VPicturesBagKeys::vers,l))
	{
		fDrawingSettings.LoadFromBag(inBag);
		inBag.GetString(CVSTR("SourceFineName"),fSourceFileName);
	}
	else
	{	
		sWORD PicEnd_PosX=VPicturesBagKeys::peX.Get(&inBag);
		sWORD PicEnd_PosY=VPicturesBagKeys::peY.Get(&inBag);
		sWORD PicEnd_TransfertMode=VPicturesBagKeys::peMode.Get(&inBag);
		
		fDrawingSettings.SetPicEndPos(PicEnd_PosX,PicEnd_PosY);
		fDrawingSettings.SetPicEnd_TransfertMode(PicEnd_TransfertMode);

		fDrawingSettings.SetSplitInfo(VPicturesBagKeys::frSplit.Get(&inBag)==1,VPicturesBagKeys::frH.Get(&inBag),VPicturesBagKeys::frV.Get(&inBag));
	
		if(inBag.GetReal(VPicturesBagKeys::m0,r))
		{
			mat[0]=r;	
			mat[1]=VPicturesBagKeys::m1.Get(&inBag);
			mat[2]=0;//VPicturesBagKeys::m2.Get(&inBag);
			mat[3]=VPicturesBagKeys::m3.Get(&inBag);
			mat[4]=VPicturesBagKeys::m4.Get(&inBag);
			mat[5]=0;//VPicturesBagKeys::m5.Get(&inBag);
			mat[6]=VPicturesBagKeys::m6.Get(&inBag);
			mat[7]=VPicturesBagKeys::m7.Get(&inBag);
			mat[8]=1;//VPicturesBagKeys::m8.Get(&inBag);
			VAffineTransform af(mat[0],mat[1],mat[3],mat[4],mat[5],mat[7]);
			fDrawingSettings.SetPictureMatrix(af);
		}
	}	
}
bool VPicture::_Is4DMetaSign(const VString& instr)
{
	return (instr==sCodec_4DMeta_sign1 || instr==sCodec_4DMeta_sign2 || instr==sCodec_4dmeta);
}
VError VPicture::_FromStream(VStream* inStream,_VPictureAccumulator* inRecorder)
{
	VError err;
	
	bool byteswap=false,closestream;
	
	_ResetExtraInfo();
		
	if(inRecorder)
		inRecorder->Retain();
	if(!inStream->IsReading())
	{
		err=inStream->OpenReading()	;
		closestream=true;
	}
	else
	{
		err=VE_OK;
		closestream=false;
	}
	if(err==VE_OK)
	{
		if(inStream->GetSize())
		{
			xVPictureBlockHeaderV1 tmpheader={-1,0,0,0};
			uLONG8 startpos=inStream->GetPos();
			VSize outread;
			bool okreaddata=false;
			
			err=inStream->GetData(&tmpheader.fSign,sizeof(sLONG));
			
			if(err==VE_OK)
			{
				if(tmpheader.fSign==0)
				{
					// image vide
					_ClearMap();
				}
				else
				{
				
					//inStream->SetPos(startpos);
						
					err=inStream->GetData(&tmpheader.fVersion,sizeof(xVPictureBlockHeaderV1) - sizeof(sLONG));
					if(err==VE_OK)
					{
						if(tmpheader.fSign=='4PCT' || tmpheader.fSign=='TCP4')
						{
							byteswap= tmpheader.fSign=='TCP4';
							if(byteswap)
							{
								ByteSwapLongArray((uLONG*)&tmpheader,4);
							}
						
							std::vector<VString> ExtensionArray;
							VSize mapoffset=0; //SH Modif pour eviter le C4267
							VSize mapsize=0; //SH Modif pour eviter le C4267
							short mapversion=1;
							
							switch(tmpheader.fVersion)
							{

								case 6:
								case 7:
								{
									xVPictureBlockHeaderV6* headerv6Ptr;
									headerv6Ptr = new xVPictureBlockHeaderV6();
									if(headerv6Ptr)
									{
										xVPictureBlockHeaderV6& headerv6=*headerv6Ptr;
										
										memcpy(headerv6Ptr,&tmpheader,sizeof(xVPictureBlockHeaderV1));
										
										err=inStream->GetData(((char*)headerv6Ptr)+sizeof(xVPictureBlockHeaderV1),sizeof(xVPictureBlockHeaderV6) - sizeof(xVPictureBlockHeaderV1));
										if(err==VE_OK)
										{
											if(byteswap)
											{
												ByteSwapLong((uLONG*)&headerv6.fExtensionsBufferSize);
												ByteSwapLong((uLONG*)&headerv6.fBagSize);
											}
											if(headerv6.fExtensionsBufferSize)
											{
												
												VString tmpstr;
												for(sLONG i=0;i<tmpheader.fNbPict;i++)
												{
													tmpstr.ReadFromStream(inStream);
													if(!inRecorder && _Is4DMetaSign(tmpstr))
													{
														inRecorder=new _VPictureAccumulator();
													}
													ExtensionArray.push_back(tmpstr);
												}
											}
											if(headerv6.fBagSize)
											{
												VValueBag bag;
												err=bag.ReadFromStream(inStream);
												if(err==VE_OK)
													_ReadExtraInfoBag(bag);
											}
											
											mapoffset=sizeof(xVPictureBlockHeaderV6) + headerv6.fBagSize + headerv6.fExtensionsBufferSize;
											mapsize=sizeof(xVPictureBlock)*tmpheader.fNbPict;
											if(tmpheader.fVersion==6)
												mapversion=2;
											else
												mapversion=3;
										}
										delete headerv6Ptr;
									}
									else
										err=VE_MEMORY_FULL;
									
									break;
								}
								case 8:
								{
									xVPictureBlockHeaderV7* headerv7Ptr;
									headerv7Ptr = new xVPictureBlockHeaderV7();
									if(headerv7Ptr)
									{
										xVPictureBlockHeaderV7& headerv7=*headerv7Ptr;
										
										memcpy(headerv7Ptr,&tmpheader,sizeof(xVPictureBlockHeaderV1));

										err=inStream->GetData(((char*)headerv7Ptr)+ sizeof(xVPictureBlockHeaderV1) ,sizeof(xVPictureBlockHeaderV7) - sizeof(xVPictureBlockHeaderV1));
										if(err==VE_OK)
										{
											if(byteswap)
											{
												ByteSwapLong((uLONG*)&headerv7.fExtensionsBufferSize);
												ByteSwapLong((uLONG*)&headerv7.fBagSize);
												ByteSwapLong((uLONG*)&headerv7.fExtraDataSize);
											}
											if(headerv7.fExtensionsBufferSize)
											{
												VPtr tmpptr=VMemory::NewPtr(headerv7.fExtensionsBufferSize,'pict');
												VString tmpstr;
												if(tmpptr)
												{
													err=inStream->GetData(tmpptr,headerv7.fExtensionsBufferSize);
													if(err==VE_OK)
													{
														VConstPtrStream tmpstream(tmpptr,headerv7.fExtensionsBufferSize);
														err=tmpstream.OpenReading();
														if(err==VE_OK)
														{
															
															tmpstream.SetLittleEndian();
															for(sLONG i=0;i<tmpheader.fNbPict;i++)
															{
																tmpstr.ReadFromStream(&tmpstream);
																if(!inRecorder && _Is4DMetaSign(tmpstr))
																{
																	inRecorder=new _VPictureAccumulator();
																}
																ExtensionArray.push_back(tmpstr);
															}
															tmpstream.CloseReading();
														}
													}
													VMemory::DisposePtr(tmpptr);
												}
												else
													err=VMemory::GetLastError();
											}
											if(headerv7.fBagSize)
											{
												VPtr tmpptr=VMemory::NewPtr(headerv7.fBagSize,'pict');
												if(tmpptr)
												{
													err=inStream->GetData(tmpptr,headerv7.fBagSize);
													if(err==VE_OK)
													{
														VConstPtrStream tmpstream(tmpptr,headerv7.fBagSize);
														err=tmpstream.OpenReading();
														if(err==VE_OK)
														{
															VValueBag bag;
															err=bag.ReadFromStream(&tmpstream);
															if(err==VE_OK)
																_ReadExtraInfoBag(bag);
															tmpstream.CloseReading();
														}
													}
													VMemory::DisposePtr(tmpptr);
													
												}
												else
													err=VMemory::GetLastError();
												
											}
											if(headerv7.fExtraDataSize)
											{
												fExtraData = VPictureDataProvider::Create(*inStream,headerv7.fExtraDataSize,inStream->GetPos());
												
											}
											mapoffset=sizeof(xVPictureBlockHeaderV7) + headerv7.fBagSize + headerv7.fExtensionsBufferSize + headerv7.fExtraDataSize;
											mapsize=sizeof(xVPictureBlock)*tmpheader.fNbPict;
											mapversion=3;
										}
										delete headerv7Ptr;
									}
									else
										err=VE_MEMORY_FULL;
									break;
								}
								default:
								{
									err=VE_UNKNOWN_ERROR;
									okreaddata=false;
									break;
								}
							}
							okreaddata=mapsize>0 && err==VE_OK;
							if(okreaddata)
							{
									
								xVPictureBlock* dataptr=NULL; 
								dataptr=(xVPictureBlock*)VMemory::NewPtr(mapsize, 'pict');
										
								if(dataptr)
								{
									err=inStream->SetPos(startpos+mapoffset);
									if(err==VE_OK)
									{
										err=inStream->GetData(dataptr,mapsize);
										if(err==VE_OK)
										{
											if(byteswap)
											{
												for(sLONG i=0;i<tmpheader.fNbPict;i++)	
												{
													ByteSwapLongArray((uLONG*)&dataptr[i],8);
												}
											}
											if(mapversion>=2)
												err=_BuildData(dataptr,mapversion,ExtensionArray,tmpheader.fNbPict,*inStream,startpos,inRecorder);
											else
												err=VE_UNKNOWN_ERROR;
										}
									}
									VMemory::DisposePtr((VPtr)dataptr);
								}
								else
								{
									err=VMemory::GetLastError();
								}
							}	
						}
					}
				}
			}
		}
		if(closestream)
			err=inStream->CloseReading();
	}
	if(vThrowError(err)!=VE_OK)
		_ClearMap();
	_SelectDefault();
	_UpdateNullState();
	if(inRecorder)
		inRecorder->Release();
	return err;
}

bool VPicture::IsVPictureData(uCHAR* inBuff,VSize inBuffSize)
{
	return inBuffSize>sizeof(xVPictureBlockHeaderV1) && (*(sLONG*)inBuff=='4PCT' || *(sLONG*)inBuff=='TCP4');
}

VError VPicture::_BuildData(xVPictureBlock* inData,sLONG version,std::vector<VString>& inExtList, sLONG nbPict,VStream& inStream,VIndex inStartpos,_VPictureAccumulator* inRecorder)
{
	VError err=VE_OK;
	VPictureCodecFactoryRef fact;
	
	uLONG8 maxsize=inStream.GetSize();
	for(sLONG i=0;i<nbPict && err==VE_OK;i++)	
	{
		const VPictureData* tmp;
		if(inData[i].fDataOffset==-6 && inRecorder)
		{
			tmp=inRecorder->GetPictureData(inData[i].fDataSize);
			if(tmp)
			{
				tmp->Retain();
				fPictDataMap[tmp->GetEncoderID()]=tmp;
			}
		}
		else
		{
			if(inData[i].fDataSize + inStartpos + inData[i].fDataOffset <= maxsize)
			{
				
				VPictureDataProvider* ds=VPictureDataProvider::Create(inStream,inData[i].fDataSize,inStartpos+inData[i].fDataOffset);
				if(ds)
				{
					if(version==2)
						tmp=fact->CreatePictureDataFromExtension(inExtList[i],ds,inRecorder);
					else
					{
						tmp=fact->CreatePictureDataFromIdentifier(inExtList[i],ds,inRecorder);
						if(!tmp)
						{
							const XBOX::VPictureCodec* codec=fact->RetainDecoderByIdentifier(inExtList[i]);
							// pas de pictdata, on regarde si le codec existe
							if(!codec)
							{
								// pas de codec, on en creer un nouveau
								codec=fact->RegisterAndRetainUnknownDataCodec(inExtList[i]);
								if(codec)
								{
									tmp=codec->CreatePictDataFromDataProvider(*ds,inRecorder);
									codec->Release();
								}
							}
							else
							{
								// le pb n'est pas la
								codec->Release();
							}
							
						}
					}
					if(tmp)
					{
						fPictDataMap[tmp->GetEncoderID()]=tmp;
					}
					ds->Release();
				}
			}
		}
	}
	return err;
}

#if !VERSION_LINUX
VPicture* VPicture::CreateGrayPicture()const
{
	VPicture* result=0;
	const XBOX::VPictureData* data=RetainPictDataForDisplay();
	XBOX::VPictureData* gray;
	if(data)
	{
		gray=data->ConvertToGray(NULL);
		if(gray)
		{
			result=new VPicture(gray);
			result->SetDrawingSettings(GetSettings());
			gray->Release();
		}
		data->Release();
	}
	return result;
}
#endif

const VPictureData* VPicture::_GetBestForDisplay()
{
	const VPictureData* data=0;
	
	if(CountPictureData()==0) 
		return data;
	
	if(CountPictureData()==1)
		return _GetNthPictData(1);
	
	data=_GetPictDataByExtension(sCodec_png);
	if(data)
	{
		return data;
	}
	else
	{
		data=_GetPictDataByExtension(sCodec_jpg);
		if(data)
		{
			return data;
		}
		else
		{
			data=_GetPictDataByExtension(sCodec_tiff);
			if(data)
			{
				return data;
			}
			else
			{
				data=_GetPictDataByExtension(sCodec_bmp);
				if(data)
				{
					return data;
				}
				else
				{
					#if VERSIONWIN
					data=_GetPictDataByExtension(sCodec_emf);
					#else
					data=_GetPictDataByExtension(sCodec_pdf);
					#endif
					if(!data)
					{
						data=_GetPictDataByExtension(sCodec_pict);
						if(data)
						{
							return data;
						}
						else
						{
							// on a rien trouv, peut etre une image quicktime
							// dans ce cas le nb de pictdata doit etre egale a 1
							assert(CountPictureData()==1);
							return _GetNthPictData(1);
							
						}
					}
					else
						return data;
				}
			}
		}
	}
	return 0;
}
const VPictureData* VPicture::_GetBestForPrinting()
{
	if(CountPictureData()==1)
		return _GetNthPictData(1);
	
	const VPictureData *data=0;
	
	if(CountPictureData()==0)
		return data;
	
	#if VERSIONWIN
	data=_GetPictDataByExtension(sCodec_emf);
	#else
	data=_GetPictDataByExtension(sCodec_pdf);
	#endif
	if(!data)
	{
		data=_GetPictDataByExtension(sCodec_pict);
		if(!data)
		{
			data=_GetPictDataByExtension(sCodec_tiff); //priorit au tiff qui est utilis pour des doc haute def 
			if(!data)
				data=_GetBestForDisplay();
		}
	}
	return data;
}
void VPicture::_SelectDefault()
{
	const VPictureData* data;
	
	if(CountPictureData()==1)
	{
		fBestForDisplay = _GetNthPictData(1);
		fBestForPrinting = fBestForDisplay;
		return;
	}
	
	if(CountPictureData()==0)
	{
		fBestForDisplay=0;
		fBestForPrinting=0;
		return;
	}
	
	data=_GetPictDataByExtension(sCodec_gif);
	if(data)
	{
		if(data->GetFrameCount()>1)
		{
			// c'est une annimation on prend le gif en priorit
			fBestForDisplay=data;
		}
		else
		{
			// c'est pas une annimation, on essaye de trouver un type plus sympa pour l'affichage
			fBestForDisplay=_GetBestForDisplay();
		}
	}
	else
	{
		fBestForDisplay=_GetBestForDisplay();
	}
	fBestForPrinting=_GetBestForPrinting();
}
const VPictureData* VPicture::_GetDefault()const
{
	const VPictureData* result=0;
	if(fBestForDisplay)
		result=fBestForDisplay;
	else if(fBestForPrinting)
		result=fBestForPrinting;
	return result;
}

void VPicture::FromValue( const VValueSingle& inValue)
{
	if(inValue.GetValueKind()==VK_IMAGE)
	{
		FromVPicture_Copy(dynamic_cast<const VPicture&>(inValue),false);
	}
	else if (inValue.GetValueKind()==VK_BLOB)
	{
		VBlob*		vblb = dynamic_cast<VBlob*> ( ( VValueSingle* ) &inValue );
		if ( vblb )
		{
			VPictureCodecFactoryRef fact;
			VPictureDataProvider* ds= VPictureDataProvider::Create(*vblb);
			if(ds)
			{
				VPictureData* data=fact->CreatePictureData(*ds);
				FromVPictureData(data);
				if(data)
					data->Release();
				ds->Release();
			}
		}
	}
}
bool VPicture::AppendPictData(const VPictureData* inData,bool inReplaceIfExist,bool inRecalcDefaultPictData)
{
	bool result=FALSE;
	if(inData)
	{
		if(!inData->IsKind(sCodec_4pct))
		{
			result=TRUE;
			const VPictureData* exist=fPictDataMap[inData->GetEncoderID()];
			if(exist)
			{
				if(inReplaceIfExist)
				{
					exist->Release();
				}
				else
					result=FALSE;
			}
			if(result)
			{
				fPictDataMap[inData->GetEncoderID()]=inData;
				inData->Retain();
				if(inRecalcDefaultPictData)
					_SelectDefault();
			}
		}
		else
		{
			result=false;
		}	
	}
	_UpdateNullState();
	return result;
}

sLONG VPicture::_GetMaxPictDataRefcount()
{
	sLONG result=0,refcount;
	for(VPictDataMap_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			refcount=(*it).second->GetRefCount();
			if(refcount>result)
			{
				result=refcount;
			}
		}
	}
	return result;
}

void VPicture::RemovePastablePicData()
{
	VPictDataMap tmpmap;
	
	for(VPictDataMap_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			const VPictureCodec* inf=(*it).second->RetainDecoder();
			if(inf)
			{
				VString compat,priv;
				inf->GetScrapKind(compat,priv);
				if(priv.IsEmpty())
				{
					tmpmap[(*it).first]=(*it).second;
					(*it).second->Retain();
				}
				inf->Release();
			}
		}
	}
	_ClearMap();
	fPictDataMap=tmpmap;
	_SelectDefault();
	_SetValueSourceDirty();
}
const VPictureData* VPicture::_GetNthPictData(VIndex inIndex) const
{
	const VPictureData* result=0;
	if(inIndex>=1 && inIndex<=fPictDataMap.size())
	{
		VPictDataMap_Const_Iterrator it=fPictDataMap.begin();
		advance(it,inIndex-1);
		if(it!=fPictDataMap.end())
		{
			result=(*it).second;
		}
	}
	return result;
}
const VPictureData* VPicture::RetainNthPictData(VIndex inIndex) const
{
	const VPictureData* result=_GetNthPictData(inIndex);
	if(result)
		result->Retain();
	return result;
}

bool VPicture::IsRenderableForDisplay() const
{
	bool result=false;
	if (fBestForDisplay)
	{
		result = fBestForDisplay->IsRenderable();
	}
	else if (fBestForPrinting)
	{
		result = fBestForPrinting->IsRenderable();
	}
	return result;
}
bool VPicture::IsRenderableForPrinting() const
{
	bool result = false;
	if (fBestForPrinting)
	{
		result = fBestForPrinting->IsRenderable();
	}
	else if (fBestForDisplay)
	{
		result = fBestForDisplay->IsRenderable();
	}
	return result;
}

const VPictureData* VPicture::RetainPictDataForDisplay() const
{
	const VPictureData* result=0;
	if(fBestForDisplay)
	{
		result=fBestForDisplay;
	}
	else if(fBestForPrinting)
	{
		result=fBestForPrinting;
	}
	if(result)
		result->Retain();
	return result;
}
const VPictureData* VPicture::RetainPictDataForPrinting() const
{
	const VPictureData* result=0;
	if(fBestForPrinting)
	{
		result=fBestForPrinting;
	}
	else if(fBestForDisplay)
	{
		result=fBestForDisplay;
	}
	if(result)
		result->Retain();
	return result;
}

void VPicture::SelectPictureDataByExtension(const VString& inExt,bool inForDisplay,bool inForPrinting)const
{
	const VPictureData* data=RetainPictDataByExtension(inExt);
	if(data)
	{
		data->Release();
		if(inForDisplay)
			fBestForDisplay=data;
		if(inForPrinting)
			fBestForPrinting=data;
	}
}
void VPicture::SelectPictureDataByMimeType(const VString& inMime,bool inForDisplay,bool inForPrinting)const
{
	const VPictureData* data=RetainPictDataByMimeType(inMime);
	if(data)
	{
		data->Release();
		if(inForDisplay)
			fBestForDisplay=data;
		if(inForPrinting)
			fBestForPrinting=data;
	}
}

void VPicture::GetMimeList(VString& outMimes) const
{
	VString s;
	
	outMimes="";
	
	for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			const VPictureCodec* codec=(*it).second->RetainDecoder();
			if(codec)
			{
				codec->GetNthMimeType(0,s);
				if(!s.IsEmpty())
				{
					if(!outMimes.IsEmpty())
						outMimes+=";";
					outMimes += s;
				}
			}
		}
	}
}

bool VPicture::IsKeyWordInMetaData(const VString& keyword, VIntlMgr* inIntlMgr)
{
	bool result = false;
	const VPictureData* picdata =RetainNthPictData(1);
	if (picdata != nil)
	{
		const VValueBag* metadata = picdata->RetainMetadatas();
		if (metadata != nil)
		{
			const VValueBag* iptcbag = metadata->GetUniqueElement(L"IPTC");
			if (iptcbag != nil)
			{
				VString Keywords;
				iptcbag->GetString(L"Keywords",Keywords);
				std::vector<std::pair<VIndex,VIndex> > boundaries;
				if (inIntlMgr->GetWordBoundaries( Keywords, boundaries))
				{
				}
			}
			metadata->Release();
		}

		picdata->Release();
	}

	return result;
}


const VPictureData* VPicture::_GetPictDataByMimeType(const VString& inMime) const
{
	VPictureCodecFactoryRef fact;
	const VPictureData* result=0;
	for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			const VPictureCodec* inf=fact->RetainDecoderForExtension((*it).first);
			if(inf)
			{
				if(inf->CheckMimeType(inMime))
				{
					result=(*it).second;
				}
				inf->Release();
				if(result)
					break;
			}
		}
	}
	return result;
}
const VPictureData* VPicture::_GetPictDataByExtension(const VString& inExt) const
{
	
	VPictureCodecFactoryRef fact;
	const VPictureData* result=0;
	for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			const VPictureCodec* inf=fact->RetainDecoderForExtension((*it).first);
			if(inf)
			{
				if(inf->CheckExtension(inExt))
				{
					result=(*it).second;
				}
				inf->Release();
				if(result)
					break;
			}
		}
	}
	return result;
}

const VPictureData* VPicture::_GetPictDataByIdentifier(const VString& inID) const
{
	VPictureCodecFactoryRef fact;
	const VPictureData* result=0;

	const VPictureCodec* refdeco=fact->RetainDecoderByIdentifier(inID);
	if(refdeco)
	{
		for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
		{
			if((*it).second)
			{
				const VPictureCodec* inf=fact->RetainDecoderByIdentifier((*it).first);
				if(inf)
				{
					if(inf==refdeco)
						result=(*it).second;
					inf->Release();
					if(result)
						break;
				}
			}
		}
	}
	return result;
}

const VPictureData* VPicture::RetainPictDataByMimeType(const VString& inMime) const
{
	const VPictureData* result=_GetPictDataByMimeType(inMime);
	if(result)
		result->Retain();
	return result;
}

const VPictureData* VPicture::RetainPictDataByExtension(const VString& inExtension)const
{
	const VPictureData* result=_GetPictDataByExtension(inExtension);
	if(result)
		result->Retain();
	return result;
}
const VPictureData* VPicture::RetainPictDataByIdentifier(const VString& inID) const
{
	const VPictureData* result=_GetPictDataByIdentifier(inID);
	if(result)
		result->Retain();
	return result;
}

void VPicture::_UpdateNullState()
{
	if(fVValueSource && !_IsValueSourceDirty())
	{
		SetNull(fVValueSource->IsNull());
	}
	else
	{
		SetNull(_CountPictData()==0 && GetExtraDataSize()==0);
	}
}

void VPicture::DoNullChanged()
{
	if (IsNull())
	{
		_ClearMap();
		_ResetExtraInfo();
		if (fVValueSource)
			fVValueSource->SetSize(0);
	}
}

VError VPicture::ReadFromBlob(VBlob& inBlob)
{
	_SetValueSourceDirty();
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	VBlobStream st(&inBlob);
	return _FromStream(&st);
}

#if !VERSION_LINUX
//TODO - jmo : Voir avec Patrick pour virer le VHandle ?
void VPicture::FromMacHandle(void* inMacPict,bool trydecode,bool inWithPicEnd)
{
	_SetValueSourceDirty();
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	
	if(inMacPict)
	{
		char** mach=(char**)inMacPict;
		VSize machsize=VPictureData::GetMacAllocator()->GetSize(mach);
		VPictureData::GetMacAllocator()->Lock(mach);
		if(IsVPictureData((uCHAR*)*mach,(sLONG)machsize))
		{
			VConstPtrStream st(*mach,machsize);
			_FromStream(&st);
			VPictureData::GetMacAllocator()->Unlock(mach);
		}	
		else
		{
			if(trydecode)
			{
				VSize ptrsize;
				VPtr vp=VPictureData::GetMacAllocator()->ToVPtr(inMacPict,ptrsize);
				if(vp)
				{
					XBOX::VPictureDataProvider* datasource= XBOX::VPictureDataProvider::Create(vp,true,ptrsize);
					if(datasource)
					{
						VPictureCodecFactoryRef fact;
						VPictureData* data=fact->CreatePictureData(*datasource);
						if(data)
						{
							FromVPictureData(data);
							data->Release();
						}
						datasource->Release();
					}
					else
						VMemory::DisposePtr(vp);
				}
			}
			else
			{
				FromMacPicHandle(inMacPict,inWithPicEnd);
				/*VPictureData::GetMacAllocator()->Unlock(mach);
				VPictureData_MacPicture* tmppict=new VPictureData_MacPicture((xMacPictureHandle)mach);
				AppendPictData(tmppict);
				fBestForDisplay=tmppict;
				fBestForPrinting=tmppict;
				tmppict->Release();*/
			}
		}
	}
	_UpdateNullState();
}
#endif

void VPicture::FromPtr(void* inPtr,VSize inSize)
{
	_SetValueSourceDirty();
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	
	if(inPtr && inSize)
	{
		if(IsVPictureData((uCHAR*)inPtr,inSize))
		{
			VConstPtrStream st(inPtr,inSize);
			_FromStream(&st);
		}	
		else
		{
			XBOX::VPictureDataProvider* datasource= XBOX::VPictureDataProvider::Create((const VPtr)inPtr,false,inSize);
			if(datasource)
			{
				VPictureCodecFactoryRef fact;
				VPictureData* data=fact->CreatePictureData(*datasource);
				if(data)
				{
					FromVPictureData(data);
					data->Release();
				}
				datasource->Release();
			}
		}
	}
	_UpdateNullState();
}

void VPicture::FromMacPicHandle(void* inMacPict,bool inWithPicEnd)
{
	char* dataptr=0;
	VSize datasize=0;
	
	_SetValueSourceDirty();
	
	if(inMacPict)
	{
		datasize=VPictureData::GetMacAllocator()->GetSize(inMacPict);
		
		VPictureData::GetMacAllocator()->Lock(inMacPict);
		dataptr=*((char**)inMacPict);
		FromMacPictPtr(dataptr,datasize,inWithPicEnd);
		VPictureData::GetMacAllocator()->Unlock(inMacPict);
	}
	else
	{
		_ClearMap();
		_ResetExtraInfo();
		_ReleaseExtraData();
	}
	_UpdateNullState();
	
}
void VPicture::FromMacPictPtr(void* inMacPtr,VSize inSize,bool inWithPicEnd)
{
	_SetValueSourceDirty();
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	
#if (VERSIONMAC && ARCH_32) || VERSIONWIN
	if(inMacPtr && inSize)
	{
		void* mach=VPictureData::GetMacAllocator()->AllocateFromBuffer(inMacPtr,inWithPicEnd ? inSize-6 : inSize);
		
		if (mach != NULL)
		{
			VPictureData_MacPicture* tmppict=new VPictureData_MacPicture((xMacPictureHandle)mach);
			AppendPictData(tmppict);
			fBestForDisplay=tmppict;
			fBestForPrinting=tmppict;
			tmppict->Release();
			if(inWithPicEnd && inSize>6)
			{
				xPictEndBlock* picend= (xPictEndBlock*)(((char*)inMacPtr)+(inSize-6));
				SetPicEndPos(picend->fOriginX,picend->fOriginY);
				SetPicEnd_TransfertMode(picend->fTransfer);
			}
			VPictureData::GetMacAllocator()->Free(mach);
		}
	}
#else
	if(inMacPtr && inSize)
	{
		VPictureCodecFactoryRef fact;
		VPictureDataProvider* ds= VPictureDataProvider::Create((VPtr)inMacPtr,false,inSize);
		if(ds)
		{
			VPictureData* data=fact->CreatePictureData(*ds);
			if(data)
			{
				AppendPictData(data);
				fBestForDisplay=data;
				fBestForPrinting=data;
				data->Release();
			}
			ds->Release();
		}
	}
#endif
	_UpdateNullState();
}


#if VERSIONWIN
void VPicture::Draw(Gdiplus::Graphics* inPortRef,const VRect& r,class VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForDisplay)
		data=fBestForDisplay;
	else if(fBestForPrinting)
		data=fBestForPrinting;
	
	if(data)
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInGDIPlusGraphics(inPortRef,r,&set);
		}
		else
			data->DrawInGDIPlusGraphics(inPortRef,r,inSet);
	}
}
#elif VERSIONMAC
void VPicture::Draw(CGContextRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForDisplay)
		data=fBestForDisplay;
	else if(fBestForPrinting)
		data=fBestForPrinting;
	
	if (data != NULL) // sc 08/08/2006, ACI0045805
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInCGContext(inPortRef,r,&set);
		}
		else
			data->DrawInCGContext(inPortRef,r,inSet);
	}
}
#endif

#if !VERSION_LINUX
void VPicture::Draw(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForDisplay)
		data=fBestForDisplay;
	else if(fBestForPrinting)
		data=fBestForPrinting;
	
	if (data != NULL) // sc 08/08/2006, ACI0045805
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInPortRef(inPortRef,r,&set);
		}
		else
			data->DrawInPortRef(inPortRef,r,inSet);
	}
}

void VPicture::Print(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForPrinting)
		data=fBestForPrinting;
	else if(fBestForDisplay)
		data=fBestForDisplay;
	
	if(data!=NULL)
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInPortRef(inPortRef,r,&set);
		}
		else
			data->DrawInPortRef(inPortRef,r,inSet);
	}
}

void VPicture::Draw(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForDisplay)
		data=fBestForDisplay;
	else if(fBestForPrinting)
		data=fBestForPrinting;
	
	if (data != NULL) // sc 08/08/2006, ACI0045805
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInVGraphicContext(*inGraphicContext,r,&set);
		}
		else
			data->DrawInVGraphicContext(*inGraphicContext,r,inSet);
	}
}
void VPicture::Print(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet)const
{
	const VPictureData* data=0;
	if(fBestForPrinting)
		data=fBestForPrinting;
	else if(fBestForDisplay)
		data=fBestForDisplay;
	
	if(data!=NULL)
	{
		if(!inSet)
		{
			VPictureDrawSettings set(GetSettings());
			data->DrawInVGraphicContext(*inGraphicContext,r,&set);
		}
		else
			data->DrawInVGraphicContext(*inGraphicContext,r,inSet);
	}
}
#endif

void VPicture::GetValue( VValueSingle& outValue) const
{
	if(outValue.GetValueKind()==VK_IMAGE)
	{
		VPicture& outPicture=static_cast<VPicture&>(outValue);
		outPicture.FromVPicture_Retain(*this,false);
	}
	else if ( outValue.GetValueKind()==VK_BLOB )
	{
		/* Sergiy 5 March 2007 - This 'else if' is implemented for the SQL server. */
		const VPictureData*		vPictData = RetainPictDataForDisplay ( );
		if ( vPictData )
		{
			VBlob&				outBlob=static_cast<VBlob&>(outValue);
			XBOX::VSize			nSize = 0;
			vPictData-> Save ( &outBlob, 0, nSize, 0 );
			vPictData-> Release ( );
		}
	}
	else
		outValue.Clear();	
}
void VPicture::Clear()
{
	_SetValueSourceDirty();
	_ResetExtraInfo();
	_ClearMap();
	_ReleaseExtraData();
	fBestForDisplay=0;
	fBestForPrinting=0;
	SetNull(true);
}
void VPicture::FromVPicture_Retain(const VPicture& inData,bool inKeepSettings,bool inCopyOutsidePath)
{
	bool done=false;
	_SetValueSourceDirty();
	if(inData.fVValueSource)
	{
		// image li a la base de donne
		if(!inData._IsValueSourceDirty())
		{
			if(!inData.fVValueSourceLoaded)
			{
				// la pict n'est pas encore charg.
				VBlobStream st(inData.fVValueSource);
				_FromStream(&st);
				done=true;
			}
			//else
			//{
			//	// L'image est deja charge, on utilise les pictdata en memoire afin de beneficier du refconting.
			//}
		}
		//else
		//{
		//	// il y a eu des modif sur l'image, on prend la version deja en memoire
		//}
	}
	if(!done)
	{
		if(!inKeepSettings)
			_CopyExtraInfo(inData);
		_CopyMap_Retain(inData.fPictDataMap);
		_CopyExtraData_Retain(inData);
		
		fBestForDisplay=inData.fBestForDisplay;
		fBestForPrinting=inData.fBestForPrinting;
		fSourceFileName=inData.fSourceFileName;
	}
	if(inCopyOutsidePath)
	{
		if (inData.fVValueSource != NULL)
			inData.fVValueSource->GetOutsidePath(fOutsidePath);
		else
			fOutsidePath = inData.fOutsidePath;
		if (fVValueSource != NULL)
			fVValueSource->SetOutsidePath(fOutsidePath);
	}
	_UpdateNullState();
}

void VPicture::_FullyClone(const VPicture& inData,bool ForAPush)
{
	_CopyExtraInfo(inData);
	_CopyMap_Retain(inData.fPictDataMap);
	_CopyExtraData_Retain(inData);
		
	fBestForDisplay=inData.fBestForDisplay;
	fBestForPrinting=inData.fBestForPrinting;
	
	fVValueSourceLoaded=inData.fVValueSourceLoaded;
	if(inData.fVValueSource)
		fVValueSource = (VBlob*)inData.fVValueSource->FullyClone(ForAPush);
	fVValueSourceDirty=inData.fVValueSourceDirty;
	_UpdateNullState();
}
void VPicture::FromVPicture_Copy(const VPicture& inData,bool inKeepSettings,bool inCopyOutsidePath)
{
	bool done=false;
	if(inData.fVValueSource)
	{
		// image li a la base de donne
		if(!inData._IsValueSourceDirty())
		{
			if(!inData.fVValueSourceLoaded)
			{
				// la pict n'est pas encore charg.
				VBlobStream st(inData.fVValueSource);
				_FromStream(&st);
				done=true;
			}
		}
	}
	if(!done)
	{
		if(!inKeepSettings)
			_CopyExtraInfo(inData);
		
		_CopyMap_Copy(inData.fPictDataMap);
		_CopyExtraData_Copy(inData);
		
		_SelectDefault();
		
		fSourceFileName=inData.fSourceFileName;
	}
	if(inCopyOutsidePath)
	{
		if (inData.fVValueSource != NULL)
			inData.fVValueSource->GetOutsidePath(fOutsidePath);
		else
			fOutsidePath = inData.fOutsidePath;
		if (fVValueSource != NULL)
			fVValueSource->SetOutsidePath(fOutsidePath);
	}
	_SetValueSourceDirty();
	_UpdateNullState();
}

void VPicture::FromVPicture_Retain(const VPicture* inData,bool inKeepSettings,bool inCopyOutsidePath)
{
	_SetValueSourceDirty();
	if(inData)
	{
		FromVPicture_Retain(*inData,inKeepSettings,inCopyOutsidePath);
	}
	else
	{
		Clear();
	}
}

void VPicture::FromVPicture_Copy(const VPicture* inData,bool inKeepSettings,bool inCopyOutsidePath)
{
	_SetValueSourceDirty();
	if(inData)
	{
		FromVPicture_Copy(*inData,inKeepSettings,inCopyOutsidePath);
	}
	else
	{
		Clear();
	}
	
}

#if WITH_VMemoryMgr
void* VPicture::GetPicHandle(bool inAppendPicEnd) const
{
	void* result=0;
	const VPictureData* data=_GetDefault();
	if(data)
	{
		VPictureDrawSettings set(fDrawingSettings);
		bool outCanAddPicEnd=true;
		result= data->CreatePicHandle(&set,outCanAddPicEnd);
		if(result && (inAppendPicEnd && outCanAddPicEnd))
		{
			if(VPictureData::GetMacAllocator()->SetSize(result,VPictureData::GetMacAllocator()->GetSize(result)+sizeof(xPictEndBlock)))
			{
				xPictEndBlock* endblock;
				endblock=(xPictEndBlock*)(((*((char**)result))+VPictureData::GetMacAllocator()->GetSize(result))-sizeof(xPictEndBlock));
				endblock->fOriginX=GetPicEnd_PosX();
				endblock->fOriginY=GetPicEnd_PosY();
				endblock->fTransfer=GetPicEnd_TransfertMode();
			}
			else
			{
				// mem error, on peut pas ajout le picend
				VPictureData::GetMacAllocator()->Free(result);
			}
		}
	}

	return result;
}
#endif

VSize VPicture::GetDataSize(_VPictureAccumulator* inRecorder) const
{
	VSize result=0;
	if(!inRecorder)
	{
		if(_FindMetaPicture())
			inRecorder=new _VPictureAccumulator();
	}
	else
	{
		inRecorder->Retain();
	}
	result=_GetDataSize(inRecorder);
	
	if(inRecorder)
		inRecorder->Release();
	return result;
}
VPicture*	VPicture::Clone() const
{
	return new VPicture(*this);
}
VValue* VPicture::FullyClone(bool ForAPush) const // no intelligent cloning (for example cloning by refcounting for Blobs)
{
	VPicture* result=new VPicture();
	result->_FullyClone(*this,ForAPush);
	return result;
}
void VPicture::RestoreFromPop(void* context)
{
	if(fVValueSource)
		fVValueSource->RestoreFromPop(context);
}
void VPicture::Detach(Boolean inMayEmpty) // used by blobs in DB4D
{
	if(fVValueSource)
		fVValueSource->Detach(inMayEmpty);
}

VValueSingle* VPicture::GetDataBlob() const
{
	return fVValueSource;
}


CompareResult VPicture::CompareTo( const VValueSingle& /*inValue*/, Boolean /*inDiacritic*/) const
{
	return CR_UNRELEVANT;
}

VPoint VPicture::GetWidthHeight(bool inIgnoreTransform) const
{
	const VPictureData* data=_GetDefault();
	if(data)
	{
		if(!inIgnoreTransform)
		{
			VPoint p(data->GetWidth(),data->GetHeight());
			p=GetPictureMatrix()*p;
			return p;
		}
		else
		{
			return VPoint(data->GetWidth(),data->GetHeight());
		}
	}
	else
		return VPoint(0,0);
}
VRect VPicture::GetCoords(bool inIgnoreTransform)const
{
	const VPictureData* data=_GetDefault();
	if(data)
	{
		if(!inIgnoreTransform)
		{
			const VAffineTransform& trans=GetPictureMatrix();
			VRect r;
			VPoint p0(0,0);
			VPoint p1(data->GetWidth(),data->GetHeight());
			p0=trans*p0;
			p1=trans*p1;
			
			r=VRect(p0.GetX(),p0.GetY(),p1.GetX()-p0.GetX(),p1.GetY()-p0.GetY());
			if(r.GetWidth()<0)
			{
				r.SetPosBy(r.GetWidth(),0);
				r.SetSizeBy(abs(r.GetWidth())*2,0);
			}
			if(r.GetHeight()<0)
			{
				r.SetPosBy(0,r.GetHeight());
				r.SetSizeBy(0,abs(r.GetHeight())*2);
			}
			return r;
		}
		else
		{
			return VRect(0,0,data->GetWidth(),data->GetHeight());
		}
	}
	else
		return VRect(0,0,0,0);
}

sLONG VPicture::GetWidth(bool inIgnoreTransform)const
{
	const VPictureData* data=_GetDefault();
	if(data)
	{
#if !VERSION_LINUX
		if(!inIgnoreTransform)
		{
			VRect pictrect(0,0,data->GetWidth(),data->GetHeight());
			VPolygon pictpoly(pictrect);
			pictpoly=GetPictureMatrix().TransformVector(pictpoly);
			return pictpoly.GetWidth();
		}
		else
#endif
		{
			return data->GetWidth(); 
		}
	}
	else
		return 0;
}
sLONG VPicture::GetHeight(bool inIgnoreTransform)const
{
	const VPictureData* data=_GetDefault();
	if(data)
	{
#if !VERSION_LINUX
		if(!inIgnoreTransform)
		{
			VRect pictrect(0,0,data->GetWidth(),data->GetHeight());
			VPolygon pictpoly(pictrect);
			pictpoly=GetPictureMatrix().TransformVector(pictpoly);
			return pictpoly.GetHeight();
		}
		else
#endif
		{
			return data->GetHeight();
		}
	}
	else
		return 0;
}

bool VPicture::_FindPictData(const VPictureData* inData) const
{
	if(!inData)
		return false;
	for(VIndex i=1;i<=(VIndex)_CountPictData();i++)
	{
		const VPictureData *data=_GetNthPictData(i);
		if(inData==data)
		{
			return true;
		}
	}
	return false;
}

bool VPicture::IsSamePictData(const VPicture* inPicture) const
{
	if(inPicture)
	{
		if(inPicture==this)
			return true;
		if(inPicture->_CountPictData()==_CountPictData() && _CountPictData()!=0)
		{
			VSize found=0;
			for(VIndex i=1;i<=(VIndex)_CountPictData();i++)
			{
				const VPictureData *data=_GetNthPictData(i);
				if(inPicture->_FindPictData(data))
				{
					found++;
				}
			}
			return found==_CountPictData() && fExtraData==inPicture->fExtraData;
		}
		return false;
	}
	else
		return false;
}

Boolean VPicture::FromValueSameKind(const VValue* inValue)
{
	XBOX_ASSERT_KINDOF(VPicture, inValue);
	
	const VPicture* theValue = reinterpret_cast<const VPicture*>(inValue);
	FromVPicture_Retain(theValue,false);
	return true;
}

VError	VPicture::ReadFromStream (VStream* ioStream, sLONG /*inParam*/ )
{
	_SetValueSourceDirty();
	_ClearMap();
	
	return _FromStream(ioStream);
}

VError	VPicture::ReadRawFromStream( VStream* ioStream, sLONG inParam ) //used by db4d server to read data without interpretation (for example : pictures)
{
	bool closestream,byteswap=false,newblob=false;
	VError err=VE_OK;
	
	Clear();
	fVValueSourceLoaded=false;

	if(ioStream==NULL)
		return VE_INVALID_PARAMETER;

	if(!ioStream->IsReading())
	{
		err=ioStream->OpenReading();
		closestream=true;
	}
	else
	{
		err=VE_OK;
		closestream=false;
	}
	if(err==VE_OK)
	{
		xVPictureBlockHeaderV7 header,unswapedheader; // xVPictureBlockHeaderV7 is the largest header
		xVPictureBlockHeaderV7 *buffheader;
		
		buffheader = &header;
		err=ioStream->GetData(&header,sizeof(xVPictureBlockHeaderV7));
		if(err==VE_OK)
		{
			if(header.fBase.fSign=='4PCT' || header.fBase.fSign=='TCP4')
			{
				sLONG headersize=0;
				VBlob* blob;
				VSize datasize,outdatasize=0;
				
				byteswap= header.fBase.fSign=='TCP4';
				if(byteswap)
				{
					memcpy(&unswapedheader,&header,sizeof(xVPictureBlockHeaderV7));
					buffheader = &unswapedheader;
					ByteSwapLongArray((uLONG*)&header.fBase,4);
				}
				
				switch(header.fBase.fVersion)
				{
					case 6:	
					case 7:
					{
						// smaller than headerv7 can cast
						xVPictureBlockHeaderV6* headerptr=(xVPictureBlockHeaderV6*)&header;
						
						if(byteswap)
						{
							ByteSwapLong((uLONG*)&headerptr->fExtensionsBufferSize);
							ByteSwapLong((uLONG*)&headerptr->fBagSize);
						}

						datasize=sizeof(xVPictureBlockHeaderV6) 
								+ headerptr->fExtensionsBufferSize 
								+ headerptr->fBagSize 
								+ (sizeof(xVPictureBlock) * headerptr->fBase.fNbPict)
								+ headerptr->fBase.fDataSize;
						break;
					}
					case 8:
					{
						if(byteswap)
						{
							ByteSwapLong((uLONG*)&header.fExtensionsBufferSize);
							ByteSwapLong((uLONG*)&header.fBagSize);
							ByteSwapLong((uLONG*)&header.fExtraDataSize);
						}

						datasize=sizeof(xVPictureBlockHeaderV7) 
								+ header.fExtensionsBufferSize 
								+ header.fBagSize 
								+ header.fExtraDataSize
								+ (sizeof(xVPictureBlock) * header.fBase.fNbPict)
								+ header.fBase.fDataSize;

						break;
					}
					default:
					{
						datasize=0;
						break;
					}
				}
				if(datasize>0)
				{
					newblob= fVValueSource==NULL;
					if(newblob)
						blob = new VBlobWithPtr();
					else
						blob= fVValueSource;
					
					err=blob->SetSize(datasize);
					
					if(err==VE_OK)
					{
						datasize -= sizeof(xVPictureBlockHeaderV7);
						
						VPtr buff=VMemory::NewPtr(datasize,'pict');
						
						if(buff)
						{
							
							err=ioStream->GetData(buff,datasize,&outdatasize);
							if(err==VE_OK && outdatasize==datasize)
							{
								// c'est tout bon;
								// ecriture du header
								err=blob->PutData(buffheader,sizeof(xVPictureBlockHeaderV7),0);
								// ecriture du data
								err=blob->PutData(buff,datasize,sizeof(xVPictureBlockHeaderV7));
								VMemory::DisposePtr(buff);
								if(newblob)
									_AttachVValueSource(blob);
								else
									_SetValueSourceDirty(false);
									
							}
							else
							{
								if(newblob)
									delete blob;
								else
									blob->SetSize(0);
							}
						}
						else
						{
							err=VMemory::GetLastError();
						}
					}
					else
					{
						if(newblob)
							delete blob ;
						else
							blob->SetSize(0);
					}
				}
				else
				{
					err=VE_INVALID_PARAMETER;
				}
			}
		}
		if(closestream)
			err=ioStream->CloseReading();
	}
	_UpdateNullState();
	return err;
}

VError	VPicture::WriteToStream (VStream* ioStream, sLONG inParam ) const
{
	VError err=VE_OK;
	VBlobWithPtr b;
	
	err= SaveToBlob(&b,true,0);
	if(err==VE_OK)
	{
		err = ioStream->PutData( b.GetDataPtr(), b.GetSize());
	}
	return err;
}



VError	VPicture::Flush( void* inDataPtr, void* inContext) const
{
	VError err;
	if(fVValueSource && !_IsValueSourceDirty())
	{
		err= fVValueSource->Flush( inDataPtr, inContext);
	}
	else
	{
		err=_UpdateVValueSource();
		if(err==VE_OK)
			err= fVValueSource->Flush( inDataPtr, inContext);
	}
	return err;
}

void VPicture::SetReservedBlobNumber(sLONG inBlobNum)
{
	xbox_assert ( fVValueSource != NULL );

	if ( fVValueSource != NULL )
		fVValueSource->SetReservedBlobNumber(inBlobNum);
}

bool VPicture::ForcePathIfEmpty() const
{
	return (fVValueSource == NULL) ? false : fVValueSource->ForcePathIfEmpty();
}

void VPicture::SetForcePathIfEmpty(bool x)
{
	if (fVValueSource)
		fVValueSource->SetForcePathIfEmpty(x);
}

VSize VPicture::GetFullSpaceOnDisk() const
{
	return ( fVValueSource == NULL ) ? 0 : fVValueSource->GetFullSpaceOnDisk();
}

VSize VPicture::GetSpace (VSize inMax) const
{
	if (fVValueSource)
	{
		VError err=_UpdateVValueSource();
		
		return fVValueSource->GetSpace(inMax);		
	}
	else
		return sizeof(sLONG);
}

void* VPicture::WriteToPtr(void* pData,Boolean pRefOnly, VSize /*inMax*/) const
{
	if(_IsValueSourceDirty())
	{
		VError err=_UpdateVValueSource();
	}
	xbox_assert ( fVValueSource != NULL );

	return fVValueSource->WriteToPtr(pData,pRefOnly);
}

void VPicture::AssociateRecord(void* primkey, sLONG FieldNum)
{
	if (fVValueSource != NULL)
		fVValueSource->AssociateRecord(primkey, FieldNum);
}

void VPicture::SetOutsidePath(const VString& inPosixPath, bool inIsRelative)
{
	if (fVValueSource != NULL)
		fVValueSource->SetOutsidePath(inPosixPath, inIsRelative);
	fOutsidePath = inPosixPath;
}

void VPicture::GetOutsidePath(VString& outPosixPath, bool* outIsRelative)
{
	if (fVValueSource != NULL)
	{
		fVValueSource->GetOutsidePath(outPosixPath, outIsRelative);
	}
	else
		outPosixPath = fOutsidePath;
}

void VPicture::SetOutsideSuffixe(const VString& inExtention)
{
	if (fVValueSource != NULL)
		fVValueSource->SetOutsideSuffixe(inExtention);
}

bool VPicture::IsOutsidePath()
{
	if (fVValueSource == NULL)
		return false;
	else
		return fVValueSource->IsOutsidePath();
}

void VPicture::_ClearMap()
{
	if(fPictDataMap.size())
	{
		for(VPictDataMap_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
		{
			if((*it).second)
				(*it).second->Release();
		}
		fPictDataMap.clear();
	}
	fBestForDisplay=0;
	fBestForPrinting=0;
}

void VPicture::_CopyMap_Retain(const VPictDataMap& inSrcMap)
{
	_ClearMap();
	fPictDataMap=inSrcMap;
	for(VPictDataMap_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			(*it).second->Retain();
		}
	}
}
void VPicture::_CopyMap_Copy(const VPictDataMap& inSrcMap)
{
	_ClearMap();
	fPictDataMap=inSrcMap;
	for(VPictDataMap_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			(*it).second=(*it).second->Clone();
		}
	}
}

VSize VPicture::_GetDataSize(_VPictureAccumulator* inRecorder) const
{
	VSize result=0;
	for( VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			if(inRecorder)
			{
				VIndex ind;
				if(inRecorder->AddPictureData((*it).second,ind))
				{
					result += (*it).second->GetDataSize(inRecorder);
				}
			}
			else
				result += (*it).second->GetDataSize(inRecorder);
		}
	}
	if(fExtraData)
		result+=fExtraData->GetDataSize();
	return result;
}

VError VPicture::_LoadFromVValueSource()
{
	VError err=VE_INVALID_PARAMETER;
	_ClearMap();
	_ResetExtraInfo();
	_ReleaseExtraData();
	
	if(fVValueSource)
	{
		
		if(fVValueSource->IsOutsidePath())
		{
			// dans ce cas l'image n'est pas forcement au format 4pct
			if(fVValueSource->GetSize()>0)
			{
				VString outPosixPath;
				bool outIsRelative;
				fVValueSource->GetOutsidePath(outPosixPath);
				
				// create a File in Mem Dataprovider
				VPictureDataProvider* datasource=  VPictureDataProvider::Create(VFilePath(outPosixPath,FPS_POSIX),*fVValueSource);
				
				if(datasource)
				{
					VPictureCodecFactoryRef fact;
					VPictureData *data=fact->CreatePictureData(*datasource);
					if(data)
					{
						FromVPictureData(data);
						data->Release();
						_SetValueSourceDirty(false);
					}
					else
					{
						// pas de pict data, peut etre un code custom
						
						VString ext;
						VFilePath path(outPosixPath);
						path.GetExtension(ext);
						if(!ext.IsEmpty())
						{
							ext=L"."+ext;
							const VPictureCodec* decoder=fact->RetainDecoderByIdentifier(ext);
							if(!decoder)
							{
								decoder=fact->RegisterAndRetainUnknownDataCodec(ext);
							}
							
							if(decoder)
							{
								datasource->SetDecoder(decoder);
								data = decoder->CreatePictDataFromDataProvider(*datasource);
								if(data)
								{
									FromVPictureData(data);
									data->Release();
									_SetValueSourceDirty(false);
								}
								decoder->Release();
							}
						}
					}
					datasource->Release();
				}
			}
			err=VE_OK;
		}
		else
		{
			VBlobStream st(fVValueSource);
			err = _FromStream(&st);
		}
		fVValueSourceLoaded=true;
	}
	else
		err= VE_INVALID_PARAMETER;
	return err;
}

bool VPicture::IsSplited(sWORD* ioLine,sWORD* ioCol)const
{
	sLONG nline,ncol;
	
	nline=fDrawingSettings.GetSplit_Line();
	ncol=fDrawingSettings.GetSplit_Col();
	
	if(ioLine)
		*ioLine=nline;
	if(ioCol)
		*ioCol=ncol;
	return fDrawingSettings.IsSplited();
}
void VPicture::SetSplitInfo(bool inSplit,sWORD inLine,sWORD inCol)
{
	if(inLine<1)
		inLine=1;
	if(inCol<1)
		inCol=1;	
	fDrawingSettings.SetSplitInfo(inSplit,inLine,inCol);
}

void VPicture::_AttachVValueSource(VBlob* inBlob)
{
	_DetachVValueSource();
	fVValueSource=inBlob;
	fVValueSourceLoaded=false;
	_SetValueSourceDirty(false);
	SetNull(inBlob ? inBlob->IsNull() : true);
}
void VPicture::_DetachVValueSource(bool inSetNull)
{
	if(fVValueSource)
	{
		delete fVValueSource;
		fVValueSource=0;
		fVValueSourceLoaded=false;
		_SetValueSourceDirty(false);
	}
	if(inSetNull)
		SetNull(true);
}

VError VPicture::_UpdateVValueSource() const
{
	VError err=VE_OK;
	if(fVValueSource && _IsValueSourceDirty())
	{
		fVValueSource->SetSize(0);
		err=_Save(fVValueSource,false);
		_SetValueSourceDirty(false);
	}
	return err;
}

VError	VPicture::GetPictureForRTF(VString& outRTFKind,VBlob& outBlob)
{
	VString codec="";
	VError err=VE_UNKNOWN_ERROR;
	outBlob.Clear();
	const VPictureData* data=0;
	VPictureCodecFactoryRef fact;
	// on essaye de trouver un format vecto en fonction de la platform
	#if VERSIONMAC
	// force le format pict car le pdf n'est pas support dans le rtf
	data=_GetPictDataByIdentifier(sCodec_pict);
	#else
	data=_GetPictDataByIdentifier(sCodec_emf);
	if(!data)
		data=_GetPictDataByIdentifier(sCodec_pict);
	#endif
	if(data)
	{
		if(data->IsKind(sCodec_emf))
		{
			codec = sCodec_emf;
			outRTFKind="emfblip";
		}
		else if(data->IsKind(sCodec_pict))
		{
			codec = sCodec_pict;
			outRTFKind="macpict";
		}
	//	data->Release();	// T.A. 2008-07-21, ACI0057226. data must not be released
	}
	else
	{
		codec=sCodec_png;
		outRTFKind="pngblip";
		// pas de format vecto on recherche dans l'odre png & jpeg
		data=_GetPictDataByIdentifier(sCodec_png);
		if(!data)
		{
			data=_GetPictDataByIdentifier(sCodec_jpg);
			if(data)
			{
				codec=sCodec_jpg;
				outRTFKind="jpegblip";
			}
		}
	}
	if(codec!="")
	{
		err=fact->EncodeToBlob(*this,codec,outBlob,NULL);
	}
	return err;
}

VError VPicture::SaveToBlob(VBlob* inBlob,bool inEvenIfEmpty,_VPictureAccumulator* inRecorder)const
{
	VError err=VE_OK;
	if(inBlob)
		err=_Save(inBlob,inEvenIfEmpty,inRecorder);
	else
		err=VE_INVALID_PARAMETER;
	return err;
}
bool VPicture::_FindMetaPicture()const
{
	bool result=false;
	result = _GetPictDataByMimeType("image/4DMetaPict")!=0;
	return result;
}
VSize VPicture::_CountSaveable()const
{
	VSize result=0;
	
	for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		result+= ((*it).second!=0);
	}
	
	return result;
}
bool VPicture::IsPictEmpty() const
{
	return 	(!((sLONG)_CountSaveable() > 0 || GetExtraDataSize()>0));
}
VError VPicture::_Save(VBlob* inBlob,bool inEvenIfEmpty,_VPictureAccumulator* inRecorder) const
{
	VError err=VE_OK;
	
	VPtrStream	extensionstream;
	VPtrStream	baginfostream;

	sLONG curpict=0;
	VSize pictsize;
	uLONG headersize,dataoffset,mapsize;
	xVPictureBlockHeaderV7* header;
	xVPictureBlock* map;


	if (inBlob->IsOutsidePath() && (sLONG)_CountSaveable() == 1 && GetExtraDataSize()== 0 && fDrawingSettings.IsIdentityMatrix())
	{
		inBlob->SetOutsideSuffixe(L".4PCT");
		// sauvegarde dans le format natif uniquement si il n'y a q'un seul picture data, sans transformation ni extradata
		const VPictureData* data=_GetNthPictData(1);
		if(data)
		{
			VSize outSize;
			
			const VPictureCodec *codec=data->RetainDecoder();
			if(codec)
			{
				VString ext;
				codec->GetNthExtension(0,ext);
				if(!ext.IsEmpty())
					inBlob->SetOutsideSuffixe(L"." + ext);
				else
					inBlob->SetOutsideSuffixe(L"");
			}
			
			err=data->Save(inBlob,0,outSize);
		}
	}
	else if((sLONG)_CountSaveable() > 0 || inEvenIfEmpty || GetExtraDataSize()>0)
	{
		inBlob->SetOutsideSuffixe(L".4PCT");
		if(!inRecorder)
		{
			if(_FindMetaPicture())
			{
				inRecorder=new _VPictureAccumulator();
			}
		}
		else
			inRecorder->Retain();
			

		headersize=sizeof(xVPictureBlockHeaderV7);
		header=(xVPictureBlockHeaderV7*)VMemory::NewPtr(headersize, 'pict');
		if(header)
		{
			header->fBase.fSign='4PCT';
			header->fBase.fVersion=8;
			header->fBase.fNbPict=(sLONG)_CountSaveable();
			header->fBase.fDataSize=0;
			
			err=_SaveExtensions(extensionstream);
			
			if(err==VE_OK)
			{
				
				header->fExtensionsBufferSize=(sLONG) extensionstream.GetDataSize();
				
				err=_SaveExtraInfoBag(baginfostream);
				
				if(err==VE_OK)
				{
					header->fBagSize= (sLONG) baginfostream.GetDataSize();
					
					header->fExtraDataSize=(sLONG) GetExtraDataSize();
					
					mapsize=sizeof(xVPictureBlock)*header->fBase.fNbPict;
					
					map=(xVPictureBlock*)VMemory::NewPtr(mapsize, 'pict');
					
					if(map)
					{
						dataoffset=headersize + header->fExtraDataSize + header->fBagSize + header->fExtensionsBufferSize + mapsize;
						
						for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
						{
							if((*it).second)
							{
								map[curpict].fKind=0;
								map[curpict].fFlags=0;
								map[curpict].fHRes=0;
								map[curpict].fVRes=0;
								map[curpict].fWidth=0;
								map[curpict].fHeight=0;
								
								if(inRecorder)
								{
									VIndex outIndex=-1;
									if(inRecorder->AddPictureData((*it).second,outIndex))
									{
										// le data n'est pas encore sauvegarder
										map[curpict].fDataOffset=dataoffset;
										(*it).second->Save(inBlob,dataoffset,pictsize,inRecorder);
										map[curpict].fDataSize=(uLONG)pictsize;
										dataoffset+=(uLONG)pictsize;
										header->fBase.fDataSize+=map[curpict].fDataSize;
									}
									else
									{
										// le data est deja sauver, on stock juste la ref
										map[curpict].fDataOffset=-6;
										map[curpict].fDataSize=(uLONG)outIndex;
									}
								}
								else
								{	
									map[curpict].fDataOffset=dataoffset;
									(*it).second->Save(inBlob,dataoffset,pictsize,inRecorder);
									map[curpict].fDataSize=(uLONG)pictsize;
									dataoffset+=(uLONG)pictsize;
									header->fBase.fDataSize+=map[curpict].fDataSize;
								}
								curpict++;
							}
						}

						err=inBlob->PutData(header,headersize,0);
						if(err==VE_OK)
						{
							if(header->fExtensionsBufferSize)
							{
								
								err=inBlob->PutData(extensionstream.GetDataPtr(),header->fExtensionsBufferSize,headersize);
								extensionstream.Clear();
							}
							if(err==VE_OK)
							{
								if(header->fBagSize)
								{
									err=inBlob->PutData(baginfostream.GetDataPtr(),header->fBagSize,headersize+header->fExtensionsBufferSize);
									baginfostream.Clear();
								}
								if(err==VE_OK)
								{
									if(fExtraData && header->fExtraDataSize)
									{
										err=fExtraData->WriteToBlob(*inBlob,headersize+header->fExtensionsBufferSize+header->fBagSize);
									}
									if(err==VE_OK)
									{
										if(header->fBase.fNbPict)
										{
											err=inBlob->PutData(map,mapsize,headersize+header->fExtraDataSize+header->fBagSize+header->fExtensionsBufferSize);
											vThrowError( err);
										}
									}
									else
										vThrowError( err);
								}
								else
									vThrowError( err);
							}
							else
								vThrowError( err);
						}
						else
							vThrowError( err);
						
						VMemory::DisposePtr((VPtr)header);
						VMemory::DisposePtr((VPtr)map);
					}
					else
					{
						vThrowError(VMemory::GetLastError());
					}
				}			
				else
					vThrowError(err);
			}
			else
				vThrowError(err);
		}
		else
		{
			vThrowError(VMemory::GetLastError());
		}
		if(inRecorder)
			inRecorder->Release();
	}
	else // L.R le 21 juin 2006, s'il n'y a pas de picy a sauver on ne met pas de header pour pouvoir ne pas prendre la place d'un blob
	{
		inBlob->SetSize(0);
	}
	return err;
}

#if !VERSION_LINUX
VPicture* VPicture::BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,bool inNoAlpha,const VColor& inColor)const
{
	VPictureData* thumbdata=NULL;
	const VPictureData* data=0;
	VPicture* result=new VPicture();
	if(fBestForDisplay)
		data=fBestForDisplay;
	else if(fBestForPrinting)
		data=fBestForPrinting;
	if(data)
	{
		VPictureDrawSettings set(fDrawingSettings);
		thumbdata=data->BuildThumbnail(inWidth,inHeight,inMode,&set,inNoAlpha,inColor);
	}
	else
	{
		VBitmapData emptybm(inWidth,inHeight,VBitmapData::VPixelFormat32bppARGB);
		emptybm.Clear(VColor(255,255,255,0));
		thumbdata=emptybm.CreatePictureData();
	}
	if(thumbdata)
	{
		result->AppendPictData(thumbdata,true,true);
		thumbdata->Release();
	}
	return result;
}
#endif

const VValueBag* VPicture::RetainMetaDatas(const VString* inPictureIdentifier)
{
	const VValueBag* result=NULL;
	const VPictureData* data;
	if(inPictureIdentifier!=NULL && !inPictureIdentifier->IsEmpty())
	{
		data=RetainPictDataByIdentifier(*inPictureIdentifier);
		if(data)
			result=data->RetainMetadatas();
		ReleaseRefCountable(&data);
	}
	else
	{
		VIndex nbData=CountPictureData();
		for(VIndex i=1 ; i<=nbData && result==NULL ; i++)
		{
			data = _GetNthPictData(i);
			result=data->RetainMetadatas();
		}
	}
	return result;
}

void VPicture::DumpPictureInfo(VString& outDump,sLONG level) const
{
	VString kind;
	VString space;
	for(long i=level;i>0;i--)
		space+="\t";
	outDump.AppendPrintf("%S***** VPicture Begin Dump *****\r\n",&space);
	if(!GetPictureMatrix().IsIdentity())
	{
		outDump.AppendPrintf("%STransformations : \r\n",&space);
		if(GetPictureMatrix().GetScaleX()!=1.0 || GetPictureMatrix().GetScaleY()!=1.0)
		{
			outDump.AppendPrintf("%S\tScaling factor: x=%f y=%f\r\n",&space,GetPictureMatrix().GetScaleX(),GetPictureMatrix().GetScaleY());
		}
		if(GetPictureMatrix().GetTranslateX()!=0.0 || GetPictureMatrix().GetTranslateY()!=0.0)
		{
			outDump.AppendPrintf("%S\tTranslation (in pixel): x=%f y=%f\r\n",&space,GetPictureMatrix().GetTranslateX(),GetPictureMatrix().GetTranslateY());
		}
		outDump.AppendPrintf("\r\n");
	}
	outDump.AppendPrintf("%SPicture Data : %i\r\n",&space,fPictDataMap.size());
	
	if(fBestForDisplay)
	{
		kind=fBestForDisplay->GetEncoderID();
		outDump.AppendPrintf("%S\tcurrent for display: %S\r\n",&space,&kind);
	}
	else
		outDump.AppendPrintf("%S\tcurrent for display: NULL\r\n",&space);
		
	if(fBestForPrinting)
	{
		kind=fBestForPrinting->GetEncoderID();
		outDump.AppendPrintf("%S\tfor printing: %S\r\n",&space,&kind);
	}
	else
		outDump.AppendPrintf("%S\tfor printing: NULL\r\n",&space);
		
	outDump+="\r\n";
	
	for(VPictDataMap_Const_Iterrator it=fPictDataMap.begin();it!=fPictDataMap.end();it++)
	{
		if((*it).second)
		{
			/*outDump.AppendPrintf("%S :\r\n",&(*it).first);
			outDump.AppendPrintf("\twidth : %i height : %i\r\n",(*it).second->GetWidth(),(*it).second->GetHeight());
			outDump.AppendPrintf("\tsize : %i ko\r\n",(*it).second->GetDataSize()/1024);
			*/
			(*it).second->DumpPictureInfo(outDump,level);
		}
		else
		{
			outDump.AppendPrintf("%S%S :\r\n\tEmpty !!\r\n",&space,&(*it).first);
		}
	}
	outDump.AppendPrintf("%S***** VPicture End Dump *****\r\n",&space);
}


_VPictureAccumulator::_VPictureAccumulator()
{

}
_VPictureAccumulator::~_VPictureAccumulator()
{
	for(VIndex ind=0;ind<fList.GetCount();ind++)
	{
		if(fList[ind])
			fList[ind]->Release();
	}
}
bool _VPictureAccumulator::FindPictData(const VPictureData* inData,VIndex& outIndex)
{
	outIndex=fList.FindPos(inData); // attention index 1
	outIndex--;
	return outIndex>=0;
}
const VPictureData* _VPictureAccumulator::GetPictureData(VIndex inIndex)
{
	if(inIndex>=0 && inIndex<fList.GetCount())
		return fList[inIndex];
	else
		return 0;
}
bool _VPictureAccumulator::AddPictureData(const VPictureData* inData,VIndex& outIndex)
{
	bool result=false;
	VIndex index;
	
	// attention il ne faut pas ajouter les meta !!!!

	if(!inData->IsKind(sCodec_4dmeta))
	{
		if(!FindPictData(inData,index))
		{
			fList.AddTail(inData);
			outIndex=fList.GetCount();
			inData->Retain();
			result=true;
		}
		else
		{
			result=false;
			outIndex=index;
		}
	}
	else
		result=true;	
	return result;
}

VSize _VPictureAccumulator::GetDataSize()
{
	VSize result=0;
	for(VIndex ind=0;ind<fList.GetCount();ind++)
	{
		if(fList[ind])
			result+=fList[ind]->GetDataSize();
	}
	return result;
}

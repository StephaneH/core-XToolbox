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
#include "VJavaScriptPrecompiled.h"

#include "VJSRuntime_blob.h"
#include "VJSBuffer.h"

USING_TOOLBOX_NAMESPACE


VJSBlobData::VJSBlobData( void *inDataPtr, VSize inDataSize)
: fDataPtr( inDataPtr)
, fDataSize( inDataSize)
{
}


VJSBlobData::~VJSBlobData()
{
	VMemory::DisposePtr( fDataPtr);
}


//============================================================


VJSBlobValue::VJSBlobValue( const VString& inContentType)
: fContentType( inContentType)
, fStart( 0)
, fSize( 0)
, fSizeIsMaxSize( true)
{
}

VJSBlobValue::VJSBlobValue( sLONG8 inStart, sLONG8 inSize, const VString& inContentType)
: fContentType( inContentType)
, fStart( inStart)
, fSize( inSize)
, fSizeIsMaxSize( false)
{
}


VJSBlobValue::~VJSBlobValue()
{
}


void VJSBlobValue::SetData( VJSBlobData *)
{
	vThrowError( VE_STREAM_CANNOT_WRITE);
}


sLONG8 VJSBlobValue::GetMaxSize() const
{
	return fSize;
}


VError VJSBlobValue::CopyTo( VFile *inDestination, bool inOverwrite)
{
	VError err = VE_OK;
	VJSDataSlice *slice = RetainDataSlice();
	if (slice != NULL)
	{
		VFileDesc *desc = NULL;
		if (inOverwrite)
		{
			err = inDestination->Open( FA_READ_WRITE, &desc, FO_CreateIfNotFound | FO_Overwrite);
		}
		else
		{
			if (inDestination->Exists())
			{
				StThrowError<> errThrow( VE_FILE_ALREADY_EXISTS);
				errThrow->SetString( "path", inDestination->GetPath().GetPath());
				err = errThrow.GetError();
			}
			else
				err = inDestination->Open( FA_READ_WRITE, &desc, FO_CreateIfNotFound);
		}
		if (err == VE_OK)
		{
			err = desc->PutData( slice->GetDataPtr(), slice->GetDataSize(), 0);
		}
		delete desc;
	}
	ReleaseRefCountable( &slice);
	return err;
}



//============================================================


VJSBlobValue_Slice::VJSBlobValue_Slice( VJSBlobData *inBlobData, VSize inStart, VSize inSize, const VString& inContentType)
: inherited( inStart, inSize, inContentType)
{
	if (inSize == 0)
		fSlice = NULL;
	else
		fSlice = new VJSDataSlice( inBlobData, inStart, inSize);
}

			
VJSBlobValue_Slice::~VJSBlobValue_Slice()
{
	ReleaseRefCountable( &fSlice);
}


VJSBlobValue_Slice *VJSBlobValue_Slice::Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const
{
	if (fSlice == NULL)
		return new VJSBlobValue_Slice( NULL, 0, 0, inContentType);
	else
	{
		sLONG8 start = (inStart >= 0) ? std::min<sLONG8>( inStart, GetSize()) : std::max<sLONG8>( GetSize() + inStart, 0);
		sLONG8 end = (inEnd >= 0) ? std::min<sLONG8>( inEnd, GetSize()) : std::max<sLONG8>( GetSize() + inEnd, 0);
		sLONG8 size = (end > start) ? (end - start) : 0;
		return VJSBlobValue_Slice::Create( fSlice->GetBlobData(), fSlice->GetStart(), fSlice->GetSize(), (VSize) start, (VSize) size, inContentType);
	}
}


/*
	static
*/
VJSBlobValue_Slice *VJSBlobValue_Slice::Create( VJSBlobData *inBlobData, VSize inBlobStart, VSize inBlobSize, VSize inSliceStart, VSize inSliceSize, const VString& inContentType)
{
/*
	If you specify values for start and length such that start + length exceeds the size of the source Blob,
	the returned Blob contains data from the start index to the end of the source Blob.

	If you specify a value for start that is larger than the size of the source Blob,
	the returned Blob has size 0 and contains no data.
*/
	VJSBlobValue_Slice *blobData;
	if (inSliceStart < inBlobSize)
	{
		VSize size;
		if (inSliceStart + inSliceSize > inBlobSize)
			size = inBlobSize - inSliceStart;
		else
			size = inSliceSize;

		VSize start = inSliceStart + inBlobStart;
		blobData = new VJSBlobValue_Slice( inBlobData, start, size, inContentType);
	}
	else
	{
		blobData = new VJSBlobValue_Slice( NULL, 0, 0, inContentType);
	}
	
	return blobData;
}


/*
	static
*/
VJSBlobValue_Slice *VJSBlobValue_Slice::Create( const VBlob& inBlob, const VString& inContentType)
{
/*
	If you specify values for start and length such that start + length exceeds the size of the source Blob,
	the returned Blob contains data from the start index to the end of the source Blob.

	If you specify a value for start that is larger than the size of the source Blob,
	the returned Blob has size 0 and contains no data.
*/

	VSize size = inBlob.GetSize();
	if (size == 0)
		return new VJSBlobValue_Slice( NULL, 0, 0, inContentType);
		
	void *data = VMemory::NewPtr( size, 'blob');
	if (data != NULL)
	{
		VError err = inBlob.GetData( data, size, 0);
		if (err != VE_OK)
		{
			VMemory::DisposePtr( data);
			data = NULL;
		}
	}
	
	VJSBlobValue_Slice *value;
	if (data != NULL)
	{
		VJSBlobData *blobData = new VJSBlobData( data, size);
		value = new VJSBlobValue_Slice( blobData, 0, size, inContentType);
		ReleaseRefCountable( &blobData);
	}
	else
	{
		value = NULL;	// means error
	}
	return value;
}


/*
	static
*/
VJSBlobValue_Slice*	VJSBlobValue_Slice::Create( const void* inBlobData, VSize inBlobSize, const VString& inContentType)
{
	if (inBlobSize == 0)
		return new VJSBlobValue_Slice( NULL, 0, 0, inContentType);
		
	VJSBlobValue_Slice *value;

	void *data = VMemory::NewPtr( inBlobSize, 'blob');
	if (data != NULL)
	{
		::memcpy( data, inBlobData, inBlobSize);
		VJSBlobData *blobData = new VJSBlobData( data, inBlobSize);
		value = new VJSBlobValue_Slice( blobData, 0, inBlobSize, inContentType);
		ReleaseRefCountable( &blobData);
	}
	else
	{
		value = NULL;	// means error
	}
	return value;
}


VJSDataSlice *VJSBlobValue_Slice::RetainDataSlice() const
{
	return RetainRefCountable( fSlice);
}


void VJSBlobValue_Slice::SetData( VJSBlobData *inData)
{
	ReleaseRefCountable( &fSlice);
	if ( (inData != NULL) && (inData->GetDataSize() != 0) )
		fSlice = new VJSDataSlice( inData, 0, inData->GetDataSize());
}

//============================================================


void VJSBlob::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "slice", js_callStaticFunction<do_Slice>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "toBuffer", js_callStaticFunction<do_ToBuffer>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		
		{ "toString", js_callStaticFunction<do_ToString>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "copyTo", js_callStaticFunction<do_CopyTo>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },

		// for debugging purpose only.
		// todo: do_WriteString has a bug: it doesn't update the size attribute because by design Blob is immutable
		{ "_writeString", js_callStaticFunction<do_WriteString>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },

		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "size", js_getProperty<do_GetSize>, NULL, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ "type", js_getProperty<do_GetType>, NULL, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "Blob";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}


VJSObject VJSBlob::NewInstance( const VJSContext& inContext, void *inData, VSize inLength, const VString &inContentType)
{
	VJSObject		createdObject(inContext);
	VJSBlobValue_Slice	*blob = VJSBlobValue_Slice::Create(inData, inLength, inContentType);
	
	if (blob == NULL)
	{
		vThrowError( VE_MEMORY_FULL);
	}
	else 
	{
		createdObject = VJSBlob::CreateInstance( inContext, blob);
	}
	
	return createdObject;
}


void VJSBlob::Initialize( const VJSParms_initialize& inParms, VJSBlobValue* inBlob)
{
	inBlob->Retain();
}


void VJSBlob::Finalize( const VJSParms_finalize& inParms, VJSBlobValue* inBlob)
{
	inBlob->Release();
}


void VJSBlob::Construct( VJSParms_construct& ioParms)
{
	// default constructor builds a VBlobPtr

	sLONG8 size = 0;
	ioParms.GetLong8Param( 1, &size);

	sLONG filler = 0;
	ioParms.GetLongParam( 2, &filler);

	// If content type is unspecified, then use empty string as default.
	// Empty string means type cannot be determined, see section 6.3 of W3C's FileAPI.
	// toBlob() has same behavior.

	VString contentType;
	if (!ioParms.GetStringParam(3, contentType))
		contentType.Clear();

	VJSBlobValue_Slice *value = NULL;
	if (size == 0)
	{
		value = new VJSBlobValue_Slice( NULL, 0, 0, contentType);
	}
	else if (size < 0)
	{
		vThrowError( VE_INVALID_PARAMETER);
	}
	#if ARCH_32
	else if (size > kMAX_uLONG)
	{
		vThrowError( VE_INVALID_PARAMETER);
	}
	#endif
	else
	{
		size_t normalizedSize = (VSize) size;
		void *data = VMemory::NewPtr( normalizedSize, 'blob');
		if (data != NULL)
		{
			::memset( data, (int) filler, normalizedSize);
			VJSBlobData *blobData = new VJSBlobData( data, normalizedSize);
			value = new VJSBlobValue_Slice( blobData, 0, normalizedSize, contentType);
			ReleaseRefCountable( &blobData);
		}
		else
		{
			vThrowError( VE_MEMORY_FULL);
			value = NULL;
		}
	}

	ioParms.ReturnConstructedObject<VJSBlob>(value);

	ReleaseRefCountable( &value);
}


void VJSBlob::do_Slice( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob)
{
	sLONG8 start;
	if (!ioParms.GetLong8Param( 1, &start))
		start = 0;

	sLONG8 end;
	if (!ioParms.GetLong8Param( 2, &end))
		end = inBlob->GetSize();

	VString contentType;
	ioParms.GetStringParam( 3, contentType);	// w3c says slice doesn't inherit master blob mimetype

	VJSBlobValue *slice = inBlob->Slice( start, end, contentType);

	if (slice != NULL)
	{
		ioParms.ReturnValue( CreateInstance( ioParms.GetContext(), slice));
	}
	else
	{
		ioParms.ReturnNullValue();
	}
	
	ReleaseRefCountable( &slice);
}


void VJSBlob::do_ToBuffer( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob)
{
	VJSBufferObject *buffer;

	VJSDataSlice *data = inBlob->RetainDataSlice();
	if ( (data != NULL) && (data->GetDataSize() > 0) )
	{
		uBYTE *memBuffer = new uBYTE[data->GetDataSize()];
		if (memBuffer != NULL)
		{
			::memcpy( memBuffer, data->GetDataPtr(), data->GetDataSize());
			buffer = new VJSBufferObject( (memBuffer == NULL) ? 0 : data->GetDataSize(), memBuffer);
		}
		else
		{
			buffer = NULL;
		}
	}
	else
	{
		buffer = new VJSBufferObject( 0, NULL);
	}
	ReleaseRefCountable( &data);

	if (buffer == NULL)
	{
		vThrowError( VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();
	}
	else
	{
		ioParms.ReturnValue( VJSBufferClass::CreateInstance( ioParms.GetContext(), buffer));
	}
	
	// todo: we should call release when VJSBufferClass::CreateInstance is fixed to retain its buffer object
	//ReleaseRefCountable( &buffer);
}


void VJSBlob::do_ToString( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob)
{
	CharSet charset;
	VString charsetName;
	if (ioParms.GetStringParam( 1, charsetName))
	{
		charset = VTextConverters::Get()->GetCharSetFromName( charsetName);
	}
	else
	{
		charset = VTC_UTF_8;
	}
	
	VString result;
	if (charset != VTC_UNKNOWN)
	{
		VJSDataSlice *data = inBlob->RetainDataSlice();
		if (data != NULL)
		{
			VConstPtrStream stream( data->GetDataPtr(), data->GetDataSize());
			stream.OpenReading();
			stream.GuessCharSetFromLeadingBytes( charset);
			stream.GetText( result);
			stream.CloseReading();
		}
		ReleaseRefCountable( &data);
	}
	else
	{
		vThrowError( VE_INVALID_PARAMETER);
	}
	ioParms.ReturnString( result);
}


void VJSBlob::do_WriteString( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob)
{
	VString string;
	ioParms.GetStringParam( 1, string);

	CharSet charset;
	VString charsetName;
	if (ioParms.GetStringParam( 2, charsetName))
	{
		charset = VTextConverters::Get()->GetCharSetFromName( charsetName);
	}
	else
	{
		charset = VTC_UTF_8;
	}
	
	VString result;
	if (charset != VTC_UNKNOWN)
	{
		void *data = NULL;
		VSize size = 0; 
		VFromUnicodeConverter *converter = VTextConverters::Get()->RetainFromUnicodeConverter( charset);
		if (converter != NULL)
		{
			bool okValue = converter->ConvertRealloc( string.GetCPointer(), string.GetLength(), data, size, 0);
			if (okValue)
			{
				VJSBlobData *blobData = new VJSBlobData( data, size);
				inBlob->SetData( blobData);
				ReleaseRefCountable( &blobData);
			}
			else
			{
				VMemory::DisposePtr( data);
			}
		}
		else
		{
			vThrowError( VE_INTL_TEXT_CONVERSION_FAILED);
		}
		ReleaseRefCountable( &converter);
	}
	else
	{
		vThrowError( VE_INVALID_PARAMETER);
	}
}


void VJSBlob::do_GetSize( VJSParms_getProperty& ioParms, VJSBlobValue* inBlob)
{
	ioParms.ReturnNumber( inBlob->GetSize());
}


void VJSBlob::do_GetType( VJSParms_getProperty& ioParms, VJSBlobValue* inBlob)
{
	ioParms.ReturnString( inBlob->GetContentType());
}


void VJSBlob::do_CopyTo( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob)
{
	VError err = VE_OK;
	VFile* dest = ioParms.RetainFileParam( 1);
	if (dest != NULL)
	{
		err = inBlob->CopyTo( dest, ioParms.GetBoolParam( 2, L"OverWrite", L"KeepExisting"));
		dest->Release();
	}
	else
	{
		err = vThrowError( VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
	}
}

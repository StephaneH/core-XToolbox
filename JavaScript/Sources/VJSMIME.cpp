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
#include "VJSRuntime_file.h"
#include "VJSBuffer.h"
#include "VJSMIME.h"
#include "VJSGlobalClass.h"

#include "ServerNet/VServerNet.h"

USING_TOOLBOX_NAMESPACE

// ----------------------------------------------------------------------------


void VJSMIMEMessage::Initialize (const VJSParms_initialize& inParms, VMIMEMessage *inMIMEMessage)
{
}


void VJSMIMEMessage::Finalize (const VJSParms_finalize& inParms, VMIMEMessage *inMIMEMessage)
{
}


void VJSMIMEMessage::GetProperty (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage)
{
	sLONG num = 0;

	if ((NULL != inMIMEMessage) && ioParms.GetPropertyNameAsLong (&num))
	{
		const VectorOfMIMEPart partsList = inMIMEMessage->GetMIMEParts();
		if ((partsList.size() > num) && (num >= 0))
		{
			VMIMEMessagePart	*	formPart = const_cast<VMIMEMessagePart *> (partsList.at (num).Get());
			VJSObject					resultObject = VJSMIMEMessagePart::CreateInstance (ioParms.GetContext(), formPart);
			ioParms.ReturnValue (resultObject);
		}
		else
		{
			ioParms.ReturnNullValue();
		}
	}
	else
	{
		VString	propertyName;

		ioParms.GetPropertyName (propertyName);

		if (propertyName.EqualToUSASCIICString ("count") || propertyName.EqualToUSASCIICString ("length"))
		{
			_GetCount (ioParms, inMIMEMessage);
		}
		else if (propertyName.EqualToUSASCIICString ("encoding"))
		{
			_GetEncoding (ioParms, inMIMEMessage);
		}
		else if (propertyName.EqualToUSASCIICString ("boundary"))
		{
			_GetBoundary (ioParms, inMIMEMessage);
		}
	}
}


void VJSMIMEMessage::GetPropertyNames (VJSParms_getPropertyNames& ioParms, VMIMEMessage *inMIMEMessage)
{
	ioParms.AddPropertyName (CVSTR ("count"));
	ioParms.AddPropertyName (CVSTR ("length"));
	ioParms.AddPropertyName (CVSTR ("encoding"));
	ioParms.AddPropertyName (CVSTR ("boundary"));
}


void VJSMIMEMessage::_GetCount (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage)
{
	if (NULL != inMIMEMessage)
	{
		const VectorOfMIMEPart partsList = inMIMEMessage->GetMIMEParts();
		ioParms.ReturnNumber (partsList.size());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessage::_GetBoundary (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage)
{
	if (NULL != inMIMEMessage)
	{
		ioParms.ReturnString (inMIMEMessage->GetBoundary());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessage::_GetEncoding (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage)
{
	if (NULL != inMIMEMessage)
	{
		ioParms.ReturnString (inMIMEMessage->GetEncoding());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessage::_ToBlob (VJSParms_callStaticFunction& ioParms, VMIMEMessage *inMIMEMessage)
{
	if (NULL == inMIMEMessage)
	{
		vThrowError (VE_INVALID_PARAMETER);
		return;
	}

	VString		mimeType;
	VPtrStream	stream;
	VError		error = VE_OK;
	
	if (ioParms.CountParams() > 0)
		ioParms.GetStringParam (1, mimeType);

	if (mimeType.IsEmpty())
		mimeType.FromCString ("application/octet-stream");

	if (VE_OK == (error = inMIMEMessage->ToStream (stream, VMIMEMessage::ENCODING_BINARY_ONLY)))	// YT 13-Sep-2012 - WAK0078232
	{
		VJSBlobValue_Slice * blob = VJSBlobValue_Slice::Create (stream.GetDataPtr(), stream.GetSize(), mimeType);
		if (NULL == blob)
		{
			vThrowError (VE_MEMORY_FULL);
			ioParms.ReturnUndefinedValue();
		}
		else
		{
			ioParms.ReturnValue (VJSBlob::CreateInstance (ioParms.GetContext(), blob));
		}

		ReleaseRefCountable( &blob);
	}

	vThrowError (error);
}

void VJSMIMEMessage::_ToBuffer (VJSParms_callStaticFunction& ioParms, VMIMEMessage *inMIMEMessage)
{
	xbox_assert(inMIMEMessage != NULL);
	
	sLONG	encoding	= 0;

	if (ioParms.CountParams() > 0 && !ioParms.GetLongParam(1, &encoding) || encoding < 0 || encoding > 2) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;	

	}

	VError		error;
	VPtrStream	stream;

	if ((error = inMIMEMessage->ToStream(stream, encoding)) == VE_OK) {

		VJSBufferObject	*bufferObject;
		
		if ((bufferObject = new VJSBufferObject(stream.GetDataSize(), stream.GetDataPtr(), true)) != NULL) {

			ioParms.ReturnValue(VJSBufferClass::CreateInstance(ioParms.GetContext(), bufferObject));
			stream.StealData();
			ReleaseRefCountable<VJSBufferObject>(&bufferObject);

		} else 

			vThrowError(VE_MEMORY_FULL);
		
	} else

		vThrowError(error);
}

void VJSMIMEMessage::GetDefinition (ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "toBlob", js_callStaticFunction<_ToBlob>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "toBuffer", js_callStaticFunction<_ToBuffer>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "MIMEMessage";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.getProperty = js_getProperty<GetProperty>;
	outDefinition.getPropertyNames = js_getPropertyNames<GetPropertyNames>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}


// ----------------------------------------------------------------------------


void VJSMIMEMessagePart::Initialize (const VJSParms_initialize& inParms, VMIMEMessagePart *inMIMEPart)
{
}


void VJSMIMEMessagePart::Finalize (const VJSParms_finalize& inParms, VMIMEMessagePart *inMIMEPart)
{
}


/*
	static
*/
VJSValue VJSMIMEMessagePart::GetJSValueFromStream( JS4D::ContextRef inContext, const MimeTypeKind& inContentKind, const CharSet& inCharSet, VPtrStream *ioStream)
{
	VJSContext	vjsContext(inContext);
	VJSValue		result(vjsContext);
	MimeTypeKind		contentTypeKind = inContentKind;
	CharSet		charSet = inCharSet;

	if (MIMETYPE_UNDEFINED == contentTypeKind)
		contentTypeKind = MIMETYPE_BINARY;

	if (MIMETYPE_UNDEFINED != contentTypeKind)
	{
		if (VE_OK == ioStream->OpenReading())
		{
			switch (contentTypeKind)
			{
			case MIMETYPE_TEXT:
				{
					VString	stringValue;

					if (VTC_UNKNOWN == charSet)
						charSet = VTC_UTF_8;

					stringValue.FromBlock (ioStream->GetDataPtr(), ioStream->GetDataSize(), charSet);

					result.SetString (stringValue);
					break;
				}

			case MIMETYPE_IMAGE:
				{
#if !VERSION_LINUX
					VPicture image;

					if (VE_OK == ReadPictureFromStream (image, *ioStream))
						result.SetVValue (image);
					else
						result.SetNull();
					break;
#else
                    result.SetNull();
#endif
				}

			case MIMETYPE_BINARY:
				{
					VBlobWithPtr blob (ioStream->GetDataPtr(), ioStream->GetDataSize());

					if (!blob.IsNull())
						result.SetVValue (blob);
					else
						result.SetNull();
					break;
				}
			default:
				break;
			}

			ioStream->CloseReading();
		}
		else
		{
			result.SetNull();
		}
	}

	return result;
}

void VJSMIMEMessagePart::_GetName (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		ioParms.ReturnString (inMIMEPart->GetName());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetFileName (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		ioParms.ReturnString (inMIMEPart->GetFileName());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetMediaType (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		ioParms.ReturnString (inMIMEPart->GetMediaType());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetSize (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		ioParms.ReturnNumber (inMIMEPart->GetSize());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetBodyAsText (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		VPtrStream&	stream = const_cast<VPtrStream &>(inMIMEPart->GetData());
		VString		contentType = inMIMEPart->GetMediaType();
		MimeTypeKind		mimeTypeKind = inMIMEPart->GetMediaTypeKind();
		CharSet		charSet = inMIMEPart->GetMediaTypeCharSet();

		if (MIMETYPE_TEXT == mimeTypeKind)
		{
			VJSValue result = GetJSValueFromStream (ioParms.GetContext(), mimeTypeKind, charSet, &stream);
			ioParms.ReturnValue (result);
		}
		else
		{
			ioParms.ReturnUndefinedValue();
		}
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetBodyAsPicture (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		VPtrStream&	stream = const_cast<VPtrStream &>(inMIMEPart->GetData());
		VString		contentType = inMIMEPart->GetMediaType();
		MimeTypeKind		mimeTypeKind = inMIMEPart->GetMediaTypeKind();

		if (MIMETYPE_IMAGE == mimeTypeKind)
		{
			VJSValue result = GetJSValueFromStream(ioParms.GetContext(), mimeTypeKind, VTC_UNKNOWN, &stream);
			ioParms.ReturnValue (result);
		}
		else
		{
			ioParms.ReturnUndefinedValue();
		}
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_GetBodyAsBlob (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart)
{
	if (NULL != inMIMEPart)
	{
		VPtrStream&	stream = const_cast<VPtrStream &>(inMIMEPart->GetData());
		VString		contentType = inMIMEPart->GetMediaType();

		VJSValue result = GetJSValueFromStream (ioParms.GetContext(), MIMETYPE_BINARY, VTC_UNKNOWN, &stream);
		ioParms.ReturnValue (result);
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEMessagePart::_Save (VJSParms_callStaticFunction& ioParms, VMIMEMessagePart *inMIMEPart)
{
	VString	string;
	VError	error = VE_INVALID_PARAMETER;
	
	if ((NULL != inMIMEPart) && ioParms.GetStringParam (1, string))
	{
		VFilePath filePath (string, FPS_POSIX);
		
		if (filePath.IsFolder())
			filePath.ToSubFile (inMIMEPart->GetFileName());

		if (!filePath.IsEmpty() && filePath.IsFile())
		{
			StErrorContextInstaller errorContext (VE_FILE_CANNOT_CREATE, VE_OK);

			VFile *file = new VFile (filePath);
			if (file != NULL)
			{
				bool bOverwrite = false;
				
				if (!ioParms.GetBoolParam (2, &bOverwrite))
					bOverwrite = false;

				VFileStream *	fileStream = NULL;
				if (VE_OK == (error = file->Create (bOverwrite ? FCR_Overwrite : FCR_Default)))
				{
					if ((fileStream = new VFileStream (file)) != NULL)
					{
						VPtrStream& stream = const_cast<VPtrStream &>(inMIMEPart->GetData());

						if (VE_OK == (error = fileStream->OpenWriting()))
						{
							if (VE_OK == (error = stream.OpenReading()))
								error = fileStream->PutData (stream.GetDataPtr(), stream.GetDataSize());
							stream.CloseReading();
							fileStream->CloseWriting (true);
						}

						delete fileStream;
					}				
					else
					{
						error = VE_MEMORY_FULL;
					}
				}
			}
			else
			{
				error = VE_MEMORY_FULL;
			}
			
			QuickReleaseRefCountable (file);
		}
		else
		{
			error = VE_FOLDER_NOT_FOUND;
		}
	}

	if (VE_OK != error)
		vThrowError (error);
}


void VJSMIMEMessagePart::GetDefinition (ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "save", js_callStaticFunction<_Save>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "name", js_getProperty<_GetName>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "fileName", js_getProperty<_GetFileName>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "mediaType", js_getProperty<_GetMediaType>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "size", js_getProperty<_GetSize>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "asText", js_getProperty<_GetBodyAsText>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "asPicture", js_getProperty<_GetBodyAsPicture>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "asBlob", js_getProperty<_GetBodyAsBlob>, nil, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "MIMEMessagePart";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticValues = values;
	outDefinition.staticFunctions = functions;
}


// ----------------------------------------------------------------------------



void VJSMIMEWriter::Initialize (const VJSParms_initialize& inParms, VMIMEWriter *inMIMEWriter)
{
}


void VJSMIMEWriter::Finalize (const VJSParms_finalize& inParms, VMIMEWriter *inMIMEWriter)
{
}


void VJSMIMEWriter::GetDefinition (ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "getMIMEBoundary", js_callStaticFunction<_GetMIMEBoundary>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "getMIMEMessage", js_callStaticFunction<_GetMIMEMessage>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "addPart", js_callStaticFunction<_AddPart>, JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "MIMEWriter";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticValues = values;
	outDefinition.staticFunctions = functions;
}


void VJSMIMEWriter::_GetMIMEBoundary (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter)
{
	if (NULL != inMIMEWriter)
	{
		ioParms.ReturnString (inMIMEWriter->GetBoundary());
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEWriter::_GetMIMEMessage (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter)
{
	if (NULL != inMIMEWriter)
	{
		VMIMEMessage *	message = const_cast<VMIMEMessage *> (&inMIMEWriter->GetMIMEMessage());
		VJSObject				resultObject = VJSMIMEMessage::CreateInstance (ioParms.GetContext(), message);

		ioParms.ReturnValue (resultObject);
	}
	else
	{
		vThrowError (VE_INVALID_PARAMETER);
		ioParms.ReturnUndefinedValue();
	}
}


void VJSMIMEWriter::_AddPart (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter)
{
	VError	error = VE_INVALID_PARAMETER;

	if ((NULL != inMIMEWriter) && (ioParms.CountParams() > 0))
	{
		VJSValue		jsValue = ioParms.GetParamValue (1);
		VString		name;
		VString		fileName;
		VString		mimeType;
		VString		contentID;
		bool				isInline;
		VPtrStream	stream;

// 		if (!ioParms.GetStringParam (2, name) || name.IsEmpty())
// 			name.FromCString ("Untitled");
		if (!ioParms.GetStringParam (2, name)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			return;

		}

		if (!ioParms.GetStringParam (3, mimeType)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "3");
			return;

		}

		if (ioParms.CountParams() >= 4 && ioParms.IsStringParam(4) && !ioParms.GetStringParam(4, contentID)) {

			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "4");
			return;

		}

		if (ioParms.CountParams() >= 5) {

			if (!ioParms.GetBoolParam(5, &isInline))

				isInline = false;

		} else

			isInline = false;

		if (VE_OK == (error = stream.OpenWriting()))
		{
			if (jsValue.IsString())
			{
				VString valueString;

				if (jsValue.GetString (valueString))
				{
					CharSet	charset = VTC_UTF_8;

					if (mimeType.IsEmpty())
						mimeType.FromCString ("text/plain");

					StStringConverter<char> buffer (valueString, charset);
					error = stream.PutData (buffer.GetCPointer(), buffer.GetSize());
				}				
			}
			else if (!jsValue.IsObject()) 
			{
				// All other supported types are objects.

				error = VE_JVSC_WRONG_PARAMETER;
			}
			else
			{
				VJSObject	object = jsValue.GetObject();
				if (object.IsOfClass(VJSBufferClass::Class())) {

					VJSBufferObject	*bufferObject	= object.GetPrivateData<VJSBufferClass>();

					xbox_assert(bufferObject != NULL);
					error = stream.PutData(bufferObject->GetDataPtr(), bufferObject->GetDataSize());

					fileName.FromString(name);

				} else if (object.IsOfClass(VJSFileIterator::Class())) {

					JS4DFileIterator	*fileObject;
					VFile			*file;
					VMemoryBuffer<>		buffer;

					fileObject = object.GetPrivateData<VJSFileIterator>();
					xbox_assert(fileObject != NULL);

					file = fileObject->GetFile();
					xbox_assert(file != NULL);			

					if ((error = file->GetContent(buffer)) == VE_OK && stream.CloseWriting() == VE_OK) {
						
						stream.SetDataPtr(buffer.GetDataPtr(), buffer.GetDataSize());							
						buffer.ForgetData();
						file->GetName(fileName);

					}
								
				} else {

					VValueSingle *value = jsValue.CreateVValue();

					if (NULL != value)
					{
						if (VK_IMAGE == value->GetValueKind())
						{
	#if !VERSION_LINUX
							VPicture	*picture = (VPicture *)value;
							VString		typeList, detectedType;

							typeList.FromString("image/png;image/jpeg;image/gif");
							error = ExtractBestPictureForWeb(*picture, stream, typeList, true, detectedType);
							
							// The idea is to use the general type list and let the receiver end do the detection,
							// if the declared MIME type is empty or doesn't match the one detected.

							if (mimeType.IsEmpty()) {

								if (detectedType.IsEmpty())

									mimeType.FromString(typeList);

								else

									mimeType.FromString(detectedType);

							} else {

								if (mimeType != detectedType)

									mimeType.FromString(typeList);

							}

							fileName.FromString (name);
	#endif						
						}
						else if (VK_BLOB == value->GetValueKind())
						{
							if (mimeType.IsEmpty())
							{
								VJSObject object = jsValue.GetObject();
								if (!object.GetPropertyAsString (CVSTR ("type"), NULL, mimeType) || mimeType.IsEmpty())
									mimeType.FromCString ("application/octet-stream");
							}

							VBlobWithPtr *blob = dynamic_cast<VBlobWithPtr *>(value);
							error = stream.PutData (blob->GetDataPtr(), blob->GetSize());

							fileName.FromString (name);
						}
						else
						{
							error = VE_JVSC_WRONG_PARAMETER;
						}
					}
					else // Object is null...
					{
						error = VE_JVSC_WRONG_PARAMETER;
					}
				}

			}

			if (stream.IsWriting()) {

				if (error == VE_OK)

					error = stream.CloseWriting();

				else

					stream.CloseWriting();				

			}

		}

		if (VE_OK == error) {

			if (fileName.GetLength())

				inMIMEWriter->AddFilePart(name, fileName, isInline, mimeType, contentID, stream);

			else 

				inMIMEWriter->AddTextPart(name, isInline, mimeType, contentID, stream);

		}
	}

	vThrowError (error);
}


void VJSMIMEWriter::_Construct (VJSParms_construct& ioParms)
{
	VString		boundary;
	VMIMEWriter *	mimeWriter = NULL;

	if (ioParms.CountParams() > 0)
		ioParms.GetStringParam (1, boundary);

	mimeWriter = new VMIMEWriter (boundary);

	ioParms.ReturnConstructedObject<VJSMIMEWriter>( mimeWriter);
	if (NULL == mimeWriter)
	{
		vThrowError (VE_MEMORY_FULL);
	}
}


// ----------------------------------------------------------------------------

const XBOX::VString	VJSMIMEReader::kModuleName	= CVSTR("waf-mail-private-MIMEReader");
 
void VJSMIMEReader::GetDefinition (ClassDefinition& outDefinition)
{
	static VJSClass<VJSMIMEReader, void>::StaticFunction functions[] = {

		{	"parseMail",			js_callStaticFunction<_ParseMail>,			JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"parseEncodedWords",	js_callStaticFunction<_ParseEncodedWords>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,						0,											0																	},

	};

	outDefinition.className = "MIMEReader";
	outDefinition.staticFunctions = functions;
}

void VJSMIMEReader::RegisterModule ()
{
	VJSGlobalClass::RegisterModuleConstructor(kModuleName, _ModuleConstructor);
}

XBOX::VJSObject VJSMIMEReader::_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName)
{
	xbox_assert(inModuleName.EqualToString(kModuleName, true));

	return JS4DMakeConstructor(inContext, Class(), _Construct);
}

void VJSMIMEReader::_Construct (VJSParms_construct &inParms)
{
	// WAK0085392: ReturnConstructedObject() will not create object instance if the private data pointer is NULL.

	static sBYTE	dummyPlaceToPoint;

	inParms.ReturnConstructedObject<VJSMIMEReader>(&dummyPlaceToPoint);
}

void VJSMIMEReader::_ParseMail (VJSParms_callStaticFunction &ioParms, void *)
{
	VJSObject	mailObject(ioParms.GetContext());

	if (!ioParms.GetParamObject(1, mailObject)) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "1");
		return;

	}

	// Retrieve the addField() method from Mail object. Use it as a (poor) way to check 
	// that object given is indeed a Mail object.

	VJSObject	addFieldFunction	= mailObject.GetPropertyAsObject("addField");

	if (!addFieldFunction.IsFunction()) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "1");
		return;

	}

	VJSArray	memoryBufferArray(ioParms.GetContext());

	if (!ioParms.GetParamArray(2, memoryBufferArray) || !memoryBufferArray.GetLength()) {

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY, "2");
		return;

	}
	
	VMemoryBuffer<>	*memoryBuffers;

	if ((memoryBuffers = new VMemoryBuffer<>[memoryBufferArray.GetLength()]) == NULL) {

		vThrowError(VE_MEMORY_FULL);
		return;

	}

	sLONG	i;

	for (i = 0; i < memoryBufferArray.GetLength(); i++) {

		VJSValue	value	= memoryBufferArray.GetValueAt(i);

		if (!value.IsObject())
			
			break;

		else {

			VJSObject	bufferObject	= value.GetObject();

			if (!bufferObject.IsOfClass(VJSBufferClass::Class()))

				break;

			else {

				VJSBufferObject	*buffer;

				buffer = bufferObject.GetPrivateData<VJSBufferClass>();
				xbox_assert(buffer != NULL);

				memoryBuffers[i].SetDataPtr(buffer->GetDataPtr(), buffer->GetDataSize(), buffer->GetDataSize());

			}

		}

	}

	// Parse mail.

	if (i == memoryBufferArray.GetLength()) {

		VMemoryBufferStream	stream(memoryBuffers, i);
		bool						isMIME;
		VString				boundary;
	
		if (stream.OpenReading() == VE_OK) {

			VMIMEMailHeader	header;

			if (_ParseMailHeader(ioParms.GetContext(), mailObject, &stream, &isMIME, &header)) {

				if (isMIME) {

					// Make an VMemoryBufferStream that "contains" only the MIME stream.
					// Patch the memory buffers to make it starts at MIME boundary.

					VSize	index, total, offset, size;
					sLONG	j;
					uBYTE	*p;

					index = stream.GetPos();
					total = 0;
					j = 0;
					for ( ; ; ) {

						total += memoryBuffers[j].GetDataSize();
						if (index < total)

							break;

						if (j == i) {
	
							// Impossible, stream.GetPos() should return an index within bound.

							xbox_assert(false);

						} else

							j++;

					}

					total -= memoryBuffers[j].GetDataSize();
					offset = index - total;
					size = memoryBuffers[j].GetDataSize() - offset;
					p = (uBYTE *) memoryBuffers[j].GetDataPtr() + offset;

					memoryBuffers[j].ForgetData();	// Prevent memory from being freed.
					memoryBuffers[j].SetDataPtr(p, size, size);		

					VMemoryBufferStream	mimeStream(&memoryBuffers[j], i - j);

					// _ParseMIMEBody() will open mimeStream and then close it when done.

					_ParseMIMEBody(ioParms.GetContext(), mailObject, &mimeStream, &header);

				} else {

					_ParseTextBody(ioParms.GetContext(), mailObject, &stream);
				
				}

			}
			stream.CloseReading();

		} else {

			//** stream opening failed

		}

	}

	for (i = 0; i < memoryBufferArray.GetLength(); i++)

		memoryBuffers[i].ForgetData();

	delete[] memoryBuffers;
}

void VJSMIMEReader::_ParseEncodedWords (VJSParms_callStaticFunction &ioParms, void *)
{
	bool			isOk, isAllocated;
	uBYTE			*data;
	VSize			dataSize;
	VString	result;

	result.Clear();
	if (ioParms.IsStringParam(1)) {

		VString	string;

		if (!ioParms.GetStringParam(1, string))

			isOk = false;

		else {

			if ((dataSize = string.GetLength())) {

				if ((data = new uBYTE[dataSize]) == NULL) {

					vThrowError(VE_MEMORY_FULL);
					return;

				} else {

					// Data is supposed to be 7-bit ASCII characters.

					VSize			i;
					uBYTE			*p;
					const UniChar	*q;
					
					for (i = 0, p = data, q = string.GetCPointer(); i < dataSize; i++, p++, q++)

						*p = *q;	

				}

			}
			isOk = isAllocated = true;

		}

	} else if (ioParms.IsObjectParam(1)) {

		VJSObject	jsBuffer(ioParms.GetContext());

		if (!ioParms.GetParamObject(1, jsBuffer) || !jsBuffer.IsOfClass(VJSBufferClass::Class())) 

			isOk = false;

		else {
	
			VJSBufferObject	*buffer;

			buffer = jsBuffer.GetPrivateData<VJSBufferClass>();
			xbox_assert(buffer != NULL);

			data = (uBYTE *) buffer->GetDataPtr();
			dataSize = buffer->GetDataSize();
		
			isOk = true;
			isAllocated = false;

		}

	} else 

		isOk = false;

	if (!isOk) 

		vThrowError(VE_JVSC_WRONG_PARAMETER, "1");
		
	else if (dataSize) {

		xbox_assert(data != NULL);

		VMIMEReader::DecodeEncodedWords(data, dataSize, &result);

		if (isAllocated)

			delete[] data;

	}

	ioParms.ReturnString(result);
}

// Return true if header was parsed successfully. Returned values are valid only if parsing was successful. 
// If *outIsMultiPart is true, then *outBoundary must contain the "boundary" string.

bool VJSMIMEReader::_ParseMailHeader (VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream, bool *outIsMIME, VMIMEMailHeader *outHeader)
{
	xbox_assert(inStream != NULL);
	xbox_assert(outIsMIME != NULL);
	xbox_assert(outHeader != NULL);

	VString	line;

	// Skip "+OK" line from POP server.

	if (!_GetLine(&line, inStream, false))

		return false;

	// Parse header fields.

	VJSObject			addFieldFunction(inContext);
	std::vector<VJSValue>	arguments;
	VJSValue			result(inContext);
	
	addFieldFunction = inMailObject.GetPropertyAsObject("addField");	// Must succeed.
	arguments.push_back(VJSValue(inContext));
	arguments.push_back(VJSValue(inContext));

	sLONG			i, j, k;
	VString	fieldName, fieldBody;

	*outIsMIME = false;
	for ( ; ; ) {

		bool	isFullLine;

		isFullLine = _GetLine(&line, inStream, true);

		// A blank line separate the header from the body.

		if (line.IsEmpty())

			break;

		// Read field name, skipping leading spaces.

		for (i = 0; i < line.GetLength(); i++)

			if (line.GetUniChar(i + 1) != ' ' || line.GetUniChar(i + 1) != '\t')

				break;

		for (j = i; j < line.GetLength(); j++)

			if (line.GetUniChar(j + 1) == ':')

				break;

		// If field name is zero length or there is no ':' then skip and ignore line.

		if (i == j || j == line.GetLength())

			continue;
	
		line.GetSubString(1, j - i, fieldName);
		fieldName.RemoveWhiteSpaces(false, true);

		// If mail has an "MIME-Version" header field, consider it as MIME. 
		// Do not check version number.

		if (fieldName.EqualToString("MIME-Version"))

			*outIsMIME = true;

		// Read field body, skipping leading spaces. An empty field body is possible.

		for (k = j + 2; k < line.GetLength(); k++)

			if (line.GetUniChar(k + 1) != ' ' || line.GetUniChar(k + 1) != '\t')

				break;
		
		if (k < line.GetLength()) {

			line.GetSubString(k + 1, line.GetLength() - k, fieldBody);
			fieldBody.RemoveWhiteSpaces(false, true);

		} else

			fieldBody.Clear();

		// Try to find boundary string, do not care if mail is MIME. 
		// Also save field bodies for "single part" MIME mails.

		if (fieldName.EqualToString("Content-Type")) {

			outHeader->fIsMultiPart = _IsMultiPart(fieldBody);
			_ParseBoundary(fieldBody, &outHeader->fBoundary);

			outHeader->fContentType = fieldBody;

		} else if (fieldName.EqualToString("Content-Disposition"))

			outHeader->fContentDisposition = fieldBody;

		else if (fieldName.EqualToString("Content-Transfer-Encoding"))

			outHeader->fContentTransferEncoding = fieldBody;

		// Add field to Mail object.

		arguments[0].SetString(fieldName);
		arguments[1].SetString(fieldBody);
		inMailObject.CallFunction(addFieldFunction, &arguments, &result);

	}

	return true;
}

// Parse basic mail body, that is a sequence of lines. Set the body property as an array of string(s) (lines).
// Return true if successful.

bool VJSMIMEReader::_ParseTextBody (VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream)
{
	xbox_assert(inStream != NULL);

	VJSArray	lines(inContext);
	VJSObject	arrayObject	= lines;

	if (!arrayObject.IsArray())

		return false;	// This check for successful creation of Array object.

	bool			isOk;
	VString	line;

	for ( ; ; ) {

		if (!_GetLine(&line, inStream, false)) {

			isOk = line.EqualToUSASCIICString(".");
			break;

		} else if (line.EqualToUSASCIICString(".")) {

			// Last line should also be the end of stream. 
			// But still consider that as ok.

			isOk = true;
			break;

		} else 

			lines.PushString(line);

	}

	inMailObject.SetProperty("body", arrayObject);
	
	return isOk;
}

bool VJSMIMEReader::_ParseMIMEBody ( VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream, const VMIMEMailHeader *inHeader)
{
	xbox_assert(inHeader != NULL);

	// Messages using MIME extensions are not necessarly multi-part. For instance, it can 
	// be a single "text/plain" with a UTF-8 charset encoded using base64.

	if (inHeader->fIsMultiPart && inHeader->fBoundary.IsEmpty()) {

		//** No boundary string found in header ! Will be unable to parse MIME parts.

	}

	VMIMEMessage	*mimeMessage;

	if ((mimeMessage = new VMIMEMessage()) == NULL) {

		vThrowError(VE_MEMORY_FULL);
		return false; 

	} else {

		VJSObject	mimeMessageObject(inContext);

		mimeMessage->LoadMail(inHeader, *inStream);

		mimeMessageObject = VJSMIMEMessage::CreateInstance(inContext, mimeMessage);
		inMailObject.SetProperty("messageParts", mimeMessageObject);

		return true;

	}
}

// Return a line. Return true if it is a full line, with carriage return. Return false if end of stream
// has been reached. Set inUnfoldLines to true for header parsing.

bool VJSMIMEReader::_GetLine (VString *outString, VMemoryBufferStream *inStream, bool inUnfoldLines)
{
	xbox_assert(outString != NULL);
	xbox_assert(inStream != NULL);

	bool	isFullLine;

	isFullLine = false;
	outString->Clear();	
	for ( ; ; ) {

		sLONG	c;

		if ((c = inStream->GetNextByte()) < 0)

			break;

		if (c != '\r') {

			outString->AppendChar(c);
			continue;

		} 

		if ((c = inStream->GetNextByte()) < 0)

			break;

		if (c != '\n') {

			outString->AppendChar('\r');
			outString->AppendChar(c);
			continue;

		}

		// A full line has been read, check for lines to fold if needed.

		isFullLine = true;
		if (inUnfoldLines) {
			
			if ((c = inStream->GetNextByte()) < 0) 
				
				break;

			if (c == ' ' || c == '\t') {

				// Fold line, replacing leading space(s) with a single space.

				outString->AppendChar(' ');
				for ( ; ; ) {

					if ((c = inStream->GetNextByte()) < 0) 
				
						break;

					if (c != ' ' && c != '\t') {

						inStream->PutBackByte(c);
						break;

					}

				}
				continue;

			} else {

				inStream->PutBackByte(c);				
				break;

			}

		} else

			break;
	
	}

	return isFullLine;
}

// Check "Content-Type" field for "multipart/*".

bool VJSMIMEReader::_IsMultiPart (const VString &inContentTypeBody)
{
	VSize	length;
	VIndex	i;
	UniChar	c;
	
	length = inContentTypeBody.GetLength();
	i = 1;
	for ( ; ; ) {

		if (i > length)

			return false;
		
		c = inContentTypeBody.GetUniChar(i);
		i++;
		if (c != ' ' && c != '\t')

			break;

	}

	VString	type(c);

	for ( ; ; ) {

		if (i > length)

			break;

		c = inContentTypeBody.GetUniChar(i);
		i++;
		if (!::isalpha(c))

			break;

		else

			type.AppendUniChar(c);

	}

	return type.EqualToString("multipart");
}

// Parse the field body of "Content-Type". Look for boundary="<boundary string>". 
// Return the boundary string in *outBoundary, or leave it untouched if not found.

void VJSMIMEReader::_ParseBoundary (const VString &inContentTypeBody, VString *outBoundary)
{
	xbox_assert(outBoundary != NULL);

	VIndex	i;

	if (!(i = inContentTypeBody.Find("boundary")))

		return;

	VSize	length;
	UniChar	c;
	
	length = inContentTypeBody.GetLength();
	i += 8;	
	for ( ; ; )

		if (i > length)

			return;

		else if ((c = inContentTypeBody.GetUniChar(i)) == '=')

			break;

		else

			i++;

	// Read boundary string, quotes are optionnal.

	outBoundary->Clear();

	i++;
	for ( ; ; )

		if (i > length)

			return;

		else if ((c = inContentTypeBody.GetUniChar(i)) == '\"') 
		
			break;

		else if (c != ' ' && c != '\t') {

			// Variable i will be incremented below, out of loop.

			outBoundary->AppendUniChar(c);
			break;

		} else

			i++;

	i++;	
	for ( ; ; ) 

		if (i > length 
		|| (c = inContentTypeBody.GetUniChar(i)) == '\"'
		|| c == ' ' || c == '\t')

			break;

		else {

			outBoundary->AppendUniChar(c);
			i++;
		
		}
}

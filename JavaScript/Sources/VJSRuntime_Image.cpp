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

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSJSON.h"

#include "VJSRuntime_Image.h"

USING_TOOLBOX_NAMESPACE



void VJSImage::Initialize( const VJSParms_initialize& inParms, VJSPictureContainer* inPict)
{
	inPict->Retain();
}


void VJSImage::Finalize( const VJSParms_finalize& inParms, VJSPictureContainer* inPict)
{
	inPict->Release();
}


void VJSImage::_getPath(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (pic != nil)
	{
		VString path;
		pic->GetOutsidePath(path);
		ioParms.ReturnString(path);
	}
}

void VJSImage::_setPath(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	VString path;
	if (pic != nil)
	{
		if (ioParms.IsNullOrUndefinedParam(1))
		{
			pic->SetOutsidePath(""); // empty string means no outside
		}
		else if (ioParms.IsBooleanParam(1))
		{
			pic->SetOutsidePath("*");
		}
		else if (ioParms.IsStringParam(1))
		{
			ioParms.GetStringParam(1, path);
			pic->SetOutsidePath(path);
			if (!path.IsEmpty())
				pic->ReloadFromOutsidePath();
		}
		else
		{
			VFile* file = ioParms.RetainFileParam(1);
			if (file != nil)
			{
				file->GetPath(path, FPS_POSIX);
				pic->SetOutsidePath(path);
				if (file->Exists())
					pic->ReloadFromOutsidePath();
			}
			else
				pic->SetOutsidePath("");
			QuickReleaseRefCountable(file);
		}
		//inPict->SetModif();
	}
}

#if !VERSION_LINUX
void VJSImage::_thumbnail(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict)
{
	bool ok = true;
	if (!ioParms.IsNumberParam(1))
	{
		ok = false;
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	}
	if (!ioParms.IsNumberParam(2))
	{
		ok = false;
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
	}
	VPicture* pic = inPict->GetPict();
	if (pic != nil && ok)
	{
		sLONG w = 0, h = 0, mode = 0;
		ioParms.GetLongParam(1, &w);
		ioParms.GetLongParam(2, &h);
		ioParms.GetLongParam(3, &mode);
		if (w == 0)
			w = 300;
		if (h == 0)
			h = 300;
		if (mode == 0)
			mode = 4;
		VPicture* thumb = pic->BuildThumbnail(w, h, (PictureMosaic)mode);
		if (thumb != nil)
		{
			VPictureCodecFactoryRef fact;
			VPicture* thumb2 = new VPicture();
			VError err = fact->Encode(*thumb, L".jpg", *thumb2, nil);
			if (err == VE_OK)
			{
				/*
				VJSPictureContainer* pictContains = new VJSPictureContainer(thumb2, true, ioParms.GetContextRef());
				ioParms.ReturnValue(VJSImage::CreateInstance(ioParms.GetContextRef(), pictContains));
				pictContains->Release();
				*/
				ioParms.ReturnVValue(*thumb2, false);
				delete thumb2;
			}
			else
			{
				delete thumb2;
				ioParms.ReturnNullValue();
			}
			delete thumb;
		}
		else
			ioParms.ReturnNullValue();
	}
	else
		ioParms.ReturnNullValue();
}
#endif


void VJSImage::_saveMeta(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (pic != nil)
	{
		bool okmeta = false;
		VString jsonString;
		if (ioParms.IsObjectParam(1))
		{
			VJSObject metadata(ioParms.GetContext());
			ioParms.GetParamObject(1, metadata);
			okmeta = true;
			VJSJSON json(ioParms.GetContext());
			json.Stringify(metadata, jsonString);
		}
		else
		{
			if (inPict->MetaInfoInited())
			{
				okmeta = true;
				VJSValue metadata = inPict->GetMetaInfo();
				VJSJSON json(ioParms.GetContext());
				json.Stringify(metadata, jsonString);
			}
		}
		if (okmeta)
		{
			VValueBag* bag = new VValueBag();
			bag->FromJSONString(jsonString);
			inPict->SetMetaBag(bag);
			QuickReleaseRefCountable(bag);

		}
		//inPict->SetModif();
	}
}


void VJSImage::_save(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict)
{
	VPictureCodecFactoryRef fact;
	const VPictureCodec* encoder = nil;
	bool ok = false;

	VPicture* pic = inPict->GetPict();
	if (pic != nil)
	{
		VFile* file = ioParms.RetainFileParam(1);
		if (file != nil)
		{
			VString mimetype;
			ioParms.GetStringParam(2, mimetype);
			if (mimetype.IsEmpty())
			{
				VString extension;
				file->GetExtension(extension);
				if (extension.IsEmpty())
					extension = L"pic";
				encoder = fact->RetainEncoderForExtension(extension);
			}
			else
				encoder = fact->RetainEncoderByIdentifier(mimetype);

			if (encoder != nil)
			{
				VError err = VE_OK;
				if (file->Exists())
					err = file->Delete();
				if (err == VE_OK)
				{
					VValueBag *pictureSettings = nil;

#if !VERSION_LINUX
					VValueBag *bagMetas = (VValueBag*)inPict->RetainMetaBag();

					if (bagMetas != nil)
					{
						pictureSettings = new VValueBag();
						ImageEncoding::stWriter settingsWriter(pictureSettings);
						VValueBag *bagRetained = settingsWriter.CreateOrRetainMetadatas( bagMetas);
						if (bagRetained) 
							bagRetained->Release(); 
					}

					QuickReleaseRefCountable(bagMetas);
#endif
					err=encoder->Encode(*pic, pictureSettings, *file);

					QuickReleaseRefCountable(pictureSettings);

					if (err == VE_OK)
						ok = true;
				}
				encoder->Release();
			}
			file->Release();
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
	}
	ioParms.ReturnBool(ok);
}


void VJSImage::_getSize( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (pic == nil)
	{
		ioParms.ReturnNumber(0);
	}
	else
	{
		ioParms.ReturnNumber(pic->GetDataSize());
	}
}


void VJSImage::_getWidth( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (pic == nil)
	{
		ioParms.ReturnNumber(0);
	}
	else
	{
		ioParms.ReturnNumber(pic->GetWidth());
	}
}

void VJSImage::_getHeight( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (pic == nil)
	{
		ioParms.ReturnNumber(0);
	}
	else
	{
		ioParms.ReturnNumber(pic->GetHeight());
	}
}


void VJSImage::_getMeta( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict)
{
	VPicture* pic = inPict->GetPict();
	if (inPict->MetaInfoInited())
	{
		ioParms.ReturnValue(inPict->GetMetaInfo());
	}
	else if (pic != nil)
	{
		/*
		const VPictureData* picdata = pic->RetainNthPictData(1);
		if (picdata != nil)
		*/
		{
			//const VValueBag* metadata = picdata->RetainMetadatas();
			const VValueBag* metadata = inPict->RetainMetaBag();
			if (metadata != nil)
			{
				VJSContext	vjsContext(ioParms.GetContext());
				VString jsons;
				metadata->GetJSONString(jsons, JSON_UniqueSubElementsAreNotArrays);
				VJSJSON json(vjsContext);
				metadata->Release();

				VJSValue result(vjsContext);
				json.Parse(result,jsons);

				if (!result.IsUndefined())
				{
	#if 0
					// plante pour l'instant, il faut que je comprenne
					inPict->SetMetaInfo(result, ioParms.GetContext());
					ioParms.ReturnValue(VJSValue(ioParms.GetContext(), inPict->GetMetaInfo()));
	#else
					ioParms.ReturnValue(result);
	#endif
				}
				else
					ioParms.ReturnNullValue();
			}
			//picdata->Release();
		}
	}
}


void VJSImage::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "getPath", js_callStaticFunction<_getPath>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setPath", js_callStaticFunction<_setPath>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "saveMeta", js_callStaticFunction<_saveMeta>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "save", js_callStaticFunction<_save>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
#if !VERSION_LINUX
		{ "thumbnail", js_callStaticFunction<_thumbnail>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
#endif
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "size", js_getProperty<_getSize>, nil, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ "length", js_getProperty<_getSize>, nil, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ "width", js_getProperty<_getWidth>, nil, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ "height", js_getProperty<_getHeight>, nil, JS4D::PropertyAttributeReadOnly  | JS4D::PropertyAttributeDontDelete },
		{ "meta", js_getProperty<_getMeta>, nil, JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "Image";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}


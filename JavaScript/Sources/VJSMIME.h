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
#ifndef __VJSMIME__
#define __VJSMIME__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE



class XTOOLBOX_API VJSMIMEMessage : public VJSClass<VJSMIMEMessage, VMIMEMessage>
{
public:
	typedef VJSClass<VJSMIMEMessage, VMIMEMessage>	inherited;

	static	void			Initialize (const VJSParms_initialize& inParms, VMIMEMessage *inMIMEMessage);
	static	void			Finalize (const VJSParms_finalize& inParms, VMIMEMessage *inMIMEMessage);
	static	void			GetProperty (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage);
	static	void			GetPropertyNames (VJSParms_getPropertyNames& ioParms, VMIMEMessage *inMIMEMessage);
	static	void			GetDefinition (ClassDefinition& outDefinition);

	static	void			_GetCount (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage);
	static	void			_GetBoundary (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage);
	static	void			_GetEncoding (VJSParms_getProperty& ioParms, VMIMEMessage *inMIMEMessage);

	static	void			_ToBlob (VJSParms_callStaticFunction& ioParms, VMIMEMessage *inMIMEMessage);
	static	void			_ToBuffer (VJSParms_callStaticFunction& ioParms, VMIMEMessage *inMIMEMessage);
};


// ----------------------------------------------------------------------------


class XTOOLBOX_API VJSMIMEMessagePart : public VJSClass<VJSMIMEMessagePart, VMIMEMessagePart>
{
public:
	typedef VJSClass<VJSMIMEMessagePart, VMIMEMessagePart>	inherited;

	static	void			Initialize (const VJSParms_initialize& inParms, VMIMEMessagePart *inMIMEPart);
	static	void			Finalize (const VJSParms_finalize& inParms, VMIMEMessagePart *inMIMEPart);
	static	void			GetDefinition (ClassDefinition& outDefinition);

	static	VJSValue		GetJSValueFromStream( JS4D::ContextRef inContext, const MimeTypeKind& inContentKind, const CharSet& inCharSet, VPtrStream *ioStream);

	static	void			_GetName (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetFileName (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetMediaType (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetSize (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetBodyAsText (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetBodyAsPicture (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);
	static	void			_GetBodyAsBlob (VJSParms_getProperty& ioParms, VMIMEMessagePart *inMIMEPart);

	static	void			_Save (VJSParms_callStaticFunction& ioParms, VMIMEMessagePart *inMIMEPart);
};

// ----------------------------------------------------------------------------

class XTOOLBOX_API VJSMIMEReader : public VJSClass<VJSMIMEReader, void>
{
public:

	static const XBOX::VString	kModuleName;	// "waf-mail-private-MIMEReader"

	static void					GetDefinition (ClassDefinition& outDefinition);
	static void					RegisterModule ();

	static void					Initialize(const XBOX::VJSParms_initialize &inParms, void *inData) { ; }

private:

	static XBOX::VJSObject		_ModuleConstructor (const VJSContext &inContext, const VString &inModuleName);
	static void					_Construct (VJSParms_construct &inParms);

	static void					_ParseMail (VJSParms_callStaticFunction &ioParms, void *);
	static void					_ParseEncodedWords (VJSParms_callStaticFunction &ioParms, void *);

	static bool					_ParseMailHeader (VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream, bool *outIsMIME, VMIMEMailHeader *outHeader);

	static bool					_ParseTextBody (VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream);
	static bool					_ParseMIMEBody (VJSContext inContext, VJSObject &inMailObject, VMemoryBufferStream *inStream, const VMIMEMailHeader *inHeader);

	static bool					_GetLine (VString *outString, VMemoryBufferStream *inStream, bool inUnfoldLines);
	static bool					_IsMultiPart (const VString &inContentTypeBody);
	static void					_ParseBoundary (const VString &inContentTypeBody, VString *outBoundary);
};

// ----------------------------------------------------------------------------

class XTOOLBOX_API VJSMIMEWriter : public VJSClass<VJSMIMEWriter, VMIMEWriter>
{
public:
	typedef VJSClass<VJSMIMEWriter, VMIMEWriter>	inherited;

	static	void			Initialize (const VJSParms_initialize& inParms, VMIMEWriter *inMIMEWriter);
	static	void			Finalize (const VJSParms_finalize& inParms, VMIMEWriter *inMIMEWriter);
	static	void			GetDefinition (ClassDefinition& outDefinition);

	static	void			_GetMIMEBoundary (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter);
	static	void			_GetMIMEMessage (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter);
	static	void			_AddPart (VJSParms_callStaticFunction& ioParms, VMIMEWriter *inMIMEWriter);

	/* To Instantiate MIMEWriter JSObject */
	static	void			_Construct (VJSParms_construct& inParms);
};



END_TOOLBOX_NAMESPACE


#endif
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

#ifndef __MIME_MESSAGE_PART_INCLUDED__
#define __MIME_MESSAGE_PART_INCLUDED__

#include "ServerNet/Sources/HTTPTools.h"

BEGIN_TOOLBOX_NAMESPACE

// If a fFileName is not zero length, then part is a file, otherwise it is considered as text.
// For a file, "Content-Disposition" is "attachment", unless fIsInline is true, in which case it is "inline".


class XTOOLBOX_API VMIMEMessagePart : public XBOX::VObject, public XBOX::IRefCountable
{
public:
									VMIMEMessagePart();

	virtual const XBOX::VString&	GetName() const { return fName; }
	virtual	const XBOX::VString&	GetFileName() const { return fFileName; }
	virtual	const XBOX::VString&	GetMediaType() const { return fMediaType; }
	virtual MimeTypeKind		GetMediaTypeKind() const { return fMediaTypeKind; }
	virtual XBOX::CharSet		GetMediaTypeCharSet() const { return fMediaTypeCharSet; }
	virtual XBOX::VSize		GetSize() const { return fStream.GetDataSize(); }
	virtual	const XBOX::VPtrStream&	GetData() const { return fStream; }

	virtual const XBOX::VString&	GetContentID () const	{	return fID;			}
	virtual	bool					IsInline () const		{	return fIsInline;	}

	void							SetName (const XBOX::VString& inName) { fName.FromString (inName); }
	void							SetFileName (const XBOX::VString& inFileName) { fFileName.FromString (inFileName); }
	void							SetMediaType (const XBOX::VString& inMediaType);
	XBOX::VError					SetData (void *inDataPtr, XBOX::VSize inSize);
	XBOX::VError					PutData (void *inDataPtr, XBOX::VSize inSize);

	void							SetContentID (const XBOX::VString &inID)	{	fID.FromString(inID);	}
	void							SetIsInline (bool inIsInline)				{	fIsInline = inIsInline;	}

private:
	XBOX::VString					fName;
	XBOX::VString					fFileName;
	XBOX::VString					fMediaType;
	MimeTypeKind					fMediaTypeKind;
	XBOX::CharSet					fMediaTypeCharSet;
	XBOX::VPtrStream				fStream;

	XBOX::VString					fID;
	bool							fIsInline;

	virtual							~VMIMEMessagePart();
};
typedef std::vector<XBOX::VRefPtr<VMIMEMessagePart> > VectorOfMIMEPart;


END_TOOLBOX_NAMESPACE

#endif // __MIME_MESSAGE_PART_INCLUDED__

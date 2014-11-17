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

#ifndef __MIME_MESSAGE_INCLUDED__
#define __MIME_MESSAGE_INCLUDED__

#include "ServerNet/Sources/VMIMEMessagePart.h"

BEGIN_TOOLBOX_NAMESPACE

class VMemoryBufferStream;

// Before a mail can be loaded, its header must be parsed. 
// This is the structure used to store information for reading the body;

class XTOOLBOX_API VMIMEMailHeader : public XBOX::VObject
{
public:

	bool			fIsMultiPart;

	// If fIsMultiPart is true, then the boundary string is defined and is used to separate all parts.

	XBOX::VString	fBoundary;

	// Otherwise, the header contains the information of the sole part, which is the message body.

	XBOX::VString	fContentType;
	XBOX::VString	fContentDisposition;
	XBOX::VString	fContentTransferEncoding;		
};

class XTOOLBOX_API VMIMEMessage: public XBOX::VObject
{
	friend class VMIMEWriter;

public:

	enum {

		ENCODING_7BIT,			// Old SMTP servers.
		ENCODING_8BITMIME,		// All current STMP servers.
		ENCODING_BINARY,		// Server supporting CHUNKING (BDAT)
		ENCODING_BINARY_ONLY,	// Encode all MimeTypeKinds except MIMETYPE_TEXT

	};

	// Some SMTP servers have a limit of 998 bytes per line. Use 249 quads per line along with CRLF,
	// and this gives 249 * 4 + 2 = 998 bytes.

	static const sLONG				kBASE64_QUADS_PER_LINE	= 249;

									VMIMEMessage();
	virtual							~VMIMEMessage();

	const XBOX::VString&			GetEncoding() const { return fEncoding; }
	const XBOX::VString&			GetBoundary() const { return fBoundary; }
	const XBOX::VectorOfMIMEPart&	GetMIMEParts() const { return fMIMEParts; }

	void							Load (	bool inFromPOST,
											const XBOX::VString& inContentType,
											const XBOX::VString& inURLQuery,
											const XBOX::VStream& inStream);

	void							LoadMail (const VMIMEMailHeader *inHeader, VMemoryBufferStream &inStream);

	void							Clear();

	// Avoid using ENCODING_BINARY as it is not well supported by peers.

	XBOX::VError					ToStream (XBOX::VStream& outStream, sLONG inEncoding = ENCODING_7BIT) const;

protected:
	void							_ReadUrl (const XBOX::VString& inString);
	void							_ReadUrl (const XBOX::VStream& inStream);
	void							_ReadMultipart (const XBOX::VStream& inStream);
	void							_ReadMultiPartMail (const XBOX::VStream& inStream);
	void							_ReadSinglePartMail (const VMIMEMailHeader *inHeader, VStream &inStream);

	void							_AddTextPart (const XBOX::VString& inName, bool inIsInline, const XBOX::VString& inContentType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream);
	void							_AddFilePart (const XBOX::VString& inName, const XBOX::VString& inFileName, bool inIsInline, const XBOX::VString& inContentType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream);

	void							_AddValuePair (const XBOX::VString& inName, const XBOX::VString& inContentType, void *inData, const XBOX::VSize inDataSize);

private:
	XBOX::VString					fEncoding;
	XBOX::VString					fBoundary;
	XBOX::VectorOfMIMEPart			fMIMEParts;

	static void						_UnQuote (XBOX::VString *ioString, UniChar inStartQuote, UniChar inEndQuote);
};


END_TOOLBOX_NAMESPACE

#endif	// #ifndef __MIME_MESSAGE_INCLUDED__

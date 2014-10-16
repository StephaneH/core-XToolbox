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

#ifndef __MIME_WRITER_INCLUDED__
#define __MIME_WRITER_INCLUDED__

#include "ServerNet/Sources/VMIMEMessage.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VMIMEWriter: public XBOX::VObject
{
public:
									VMIMEWriter (const XBOX::VString& inBoundary = CVSTR (""));
	virtual							~VMIMEWriter();

	const XBOX::VString&			GetBoundary() const;
	const XBOX::VMIMEMessage&		GetMIMEMessage() const;

	void							AddTextPart (const XBOX::VString& inName, 
												bool inIsInline, 
												const XBOX::VString& inMIMEType, 
												const XBOX::VString& inContentID, 
												XBOX::VPtrStream& inStream);

	void							AddFilePart (const XBOX::VString& inName, 
												const XBOX::VString& inFileName, 
												bool inIsInline, 
												const XBOX::VString& inMIMEType, 
												const XBOX::VString& inContentID, 
												XBOX::VPtrStream& inStream);

private:
	XBOX::VMIMEMessage				fMIMEMessage;
};


END_TOOLBOX_NAMESPACE

#endif // __MIME_WRITER_INCLUDED__
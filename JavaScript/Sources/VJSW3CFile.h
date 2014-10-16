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
#ifndef __VJS_W3C_FILE__
#define __VJS_W3C_FILE__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSFileReaderSyncClass;

class XTOOLBOX_API VJSFileReader : public XBOX::VObject
{
friend class VJSFileReaderClass;

private:

	enum {

		STATE_EMPTY,

		STATE_LOADING_ARRAY_BUFFER,
		STATE_LOADING_BINARY_STRING,
		STATE_LOADING_TEXT,
		STATE_LOADING_DATA_URL,

		STATE_DONE_OK,
		STATE_DONE_ERRONEOUS,

	};

	sLONG		fState;

	XBOX::VFile	*fFile;
	// BLOB.
};

// FileReader object.

class XTOOLBOX_API VJSFileReaderClass : public XBOX::VJSClass<VJSFileReaderClass, VJSFileReader>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
	static void	Construct (XBOX::VJSParms_callAsConstructor &ioParms);
	
private:

	typedef XBOX::VJSClass<VJSFileReaderClass, VJSFileReader>	inherited;
				
	static void	_readAsArrayBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSFileReader *ioFileReader);
	static void	_readAsBinaryString (XBOX::VJSParms_callStaticFunction &ioParms, VJSFileReader *ioFileReader);
	static void	_readAsText (XBOX::VJSParms_callStaticFunction &ioParms, VJSFileReader *ioFileReader);
	static void	_readAsDataURL (XBOX::VJSParms_callStaticFunction &ioParms, VJSFileReader *ioFileReader);

	static void	_abort (XBOX::VJSParms_callStaticFunction &ioParms, VJSFileReader *ioFileReader);
};

// FileReaderSync allows synchronous reading of File or Blob objects. 

class XTOOLBOX_API VJSFileReaderSyncClass : public XBOX::VJSClass<VJSFileReaderSyncClass, XBOX::VObject>
{
public:

	// FileReaderSync objects are stateless, just a placeholder for functions.

	static void	GetDefinition (ClassDefinition &outDefinition);
	static void	Construct (XBOX::VJSParms_callAsConstructor &ioParms);
	
private:

	typedef XBOX::VJSClass<VJSFileReaderSyncClass, XBOX::VObject>	inherited;

	static void	_readAsArrayBuffer (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);
	static void	_readAsBinaryString (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);
	static void	_readAsText (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);
	static void	_readAsDataURL (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *);
};

END_TOOLBOX_NAMESPACE

#endif
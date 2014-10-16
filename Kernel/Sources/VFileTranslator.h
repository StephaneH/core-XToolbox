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
#ifndef __VFileTranslator__
#define __VFileTranslator__

#include "Kernel/Sources/VObject.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFile;
class VString;

// Class definitions
typedef	enum VTranslationMode
{
	TRM_TRANSLATEINPLACE,
	TRM_TRANSLATECOPY
} VTranslationMode;
	
typedef enum TranslatorType
{
	TT_NAVIGATION = 'NAVI',
	TT_EASYOPEN = 'EASY'
} TranslatorType;
	

class XTOOLBOX_API VFileTranslator : public VObject
{
public:
			VFileTranslator();
			VFileTranslator(VFile* inSrcFile, VFile* inDestFile);
	virtual	~VFileTranslator();
	
	virtual TranslatorType	GetTranslatorType() = 0;
	virtual void GetTranslatorName(VString &inString) const;
	virtual void SetSourceFile(VFile* inSrcFile);
	virtual void SetDestinationFile(VFile* inDestFile);
	virtual Boolean DoTranslation();

protected:
	VFile*	fSrcFile;
	VFile*	fDestFile;
};


END_TOOLBOX_NAMESPACE

#endif

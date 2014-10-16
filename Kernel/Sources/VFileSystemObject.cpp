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
#include "VKernelPrecompiled.h"

#include "VFile.h"
#include "VFolder.h"
#include "VFileSystemObject.h"
#include "VValueBag.h"
#include "VTask.h"

VFileError::VFileError( const VFile *inFile, VError inErrCode, VNativeError inNativeErrCode)
	: VErrorBase( inErrCode, inNativeErrCode)
{
	inFile->GetPath( fPath);
	_Init();
}


VFileError::VFileError( const VFolder *inFolder, VError inErrCode, VNativeError inNativeErrCode)
	: VErrorBase( inErrCode, inNativeErrCode)
{
	inFolder->GetPath( fPath);
	_Init();
}


VFileError::VFileError( const VFilePath& inPath, VError inErrCode, VNativeError inNativeErrCode)
	: VErrorBase( inErrCode, inNativeErrCode)
	, fPath( inPath)
{
	_Init();
}


void VFileError::_Init()
{
	VString name;
	fPath.GetName( name);

	GetBag()->SetString( "name", name);
	GetBag()->SetString( "path", fPath.GetPath());
}


VFileError::~VFileError()
{
}


void VFileError::GetErrorDescription( VString& outError) const
{
	inherited::GetErrorDescription( outError);
}
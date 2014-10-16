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
#include "VObject.h"
#include "VString.h"
#include "VTask.h"
#include "VMemoryCpp.h"


VCppMemMgr*	VObject::sCppMemMgr = NULL;
VCppMemMgr*	VObject::sAlternateCppMemMgr = NULL;



void VObject::DebugGetClassName(VString& outName) const
{
#if VERSIONDEBUG
	outName.FromCString(typeid(*this).name());
#else
	outName.Clear();
#endif
}


CharSet VObject::DebugDump(char* /*inTextBuffer*/, sLONG& inBufferSize) const
{
	inBufferSize = 0;
	return VTC_StdLib_char;
}


void VObject::DebugDump(VString& outDump) const
{
	char	buffer[1024L];
	sLONG	bufSize = sizeof(buffer);
	CharSet	charSet = DebugDump(buffer, bufSize);
	outDump.FromBlock(buffer, bufSize, charSet);
}


void* VObject::operator new(size_t inSize)
{
#if USE_NATIVE_MEM_MGR
	return malloc(inSize);
#else
	if (sCppMemMgr == NULL)
		sCppMemMgr = new VCppMemMgr();
	
	return sCppMemMgr->Malloc( inSize, true, 'obj ');
#endif
}


void VObject::operator delete(void* inAddr)
{
#if USE_NATIVE_MEM_MGR
	free(inAddr);
#else
	// all VCppMemMgr instances can deal with their own blocks
	return sCppMemMgr->Free( inAddr);
#endif
}

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
#ifndef __VKernel__
#define __VKernel__

#if _WIN32
	#pragma pack( push, 8 )
#else
	#pragma options align = power
#endif

// Types & Macros Headers
#include "Kernel/Sources/VKernelFlags.h"
#include "Kernel/Sources/VTypes.h"
#include "Kernel/Sources/VPlatform.h"
#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VTextTypes.h"
#ifndef XTOOLBOX_AS_STANDALONE
#include "Kernel/Sources/VScrapKind.h"
#endif

// Common Utilities Headers
#include "Kernel/Sources/VProfiler.h"
#include "Kernel/Sources/VKernelErrors.h"
#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VAssert.h"
#include "Kernel/Sources/VBitField.h"
#include "Kernel/Sources/VByteSwap.h"
#include "Kernel/Sources/VError.h"
#include "Kernel/Sources/VErrorContext.h"
#include "Kernel/Sources/VProgressIndicator.h"
#include "Kernel/Sources/VSystem.h"
#include "Kernel/Sources/IWatchable.h"
#include "Kernel/Sources/Base64Coder.h"
#include "Kernel/Sources/VRegexMatcher.h"
#include "Kernel/Sources/VPictureHelper.h"
#include "Kernel/Sources/VJSONTools.h"
#include "Kernel/Sources/VJSONValue.h"
#include "Kernel/Sources/VLogger.h"
#include "Kernel/Sources/VLog4jMsgFile.h"
#include "Kernel/Sources/VTextStyle.h"
#if VERSIONMAC || VERSION_LINUX
#include "Kernel/Sources/VSysLogOutput.h"
#endif

// File & Streams Headers
#include "Kernel/Sources/VURL.h"
#include "Kernel/Sources/VFile.h"
#include "Kernel/Sources/VFileSystemObject.h"
#include "Kernel/Sources/VFolder.h"
#include "Kernel/Sources/VFilePath.h"
#include "Kernel/Sources/VFileSystem.h"
#include "Kernel/Sources/VFileTranslator.h"
#include "Kernel/Sources/IStreamable.h"
#include "Kernel/Sources/VStream.h"
#include "Kernel/Sources/VFileStream.h"
#include "Kernel/Sources/VResource.h"
//#include "Kernel/Sources/VArchiveStream.h"
#include "Kernel/Sources/VLibrary.h"

#if VERSIONMAC
#include "Kernel/Sources/XMacResource.h"
#endif

#if VERSIONWIN
#include "Kernel/Sources/XWinResource.h"
#endif

// Memory Headers
#include "Kernel/Sources/VMemory.h"
#include "Kernel/Sources/VDebugBlockInfo.h"
#include "Kernel/Sources/VLeaks.h"
#include "Kernel/Sources/VMemoryCpp.h"
#include "Kernel/Sources/VMemorySlot.h"
#include "Kernel/Sources/VStackCrawl.h"
#include "Kernel/Sources/VMemoryWalker.h"
#include "Kernel/Sources/VMemoryBuffer.h"

// Threads & Messages Headers
#include "Kernel/Sources/IIdleable.h"
#include "Kernel/Sources/VMessage.h"
#include "Kernel/Sources/VMessageCall.h"
#include "Kernel/Sources/VProcess.h"
#include "Kernel/Sources/VSmallCriticalSection.h"
#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/VTask.h"
#include "Kernel/Sources/VInterlocked.h"

// Text Convertion Headers
#include "Kernel/Sources/VUnicodeTableLow.h"
#include "Kernel/Sources/VUnicodeTableFull.h"
#include "Kernel/Sources/VTextConverter.h"
#include "Kernel/Sources/VIntlMgr.h"
#include "Kernel/Sources/VCollator.h"
#include "Kernel/Sources/VUnicodeResources.h"
#include "Kernel/Sources/ILocalizer.h"

// Values Headers
#include "Kernel/Sources/ISortable.h"
#include "Kernel/Sources/VIterator.h"
#include "Kernel/Sources/VArray.h"
#include "Kernel/Sources/VChain.h"
#include "Kernel/Sources/VList.h"
#include "Kernel/Sources/VValue.h"
#include "Kernel/Sources/VBlob.h"
#include "Kernel/Sources/VFloat.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VString_ExtendedSTL.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VUUID.h"
#include "Kernel/Sources/VArrayValue.h"
#include "Kernel/Sources/VValueBag.h"
#include "Kernel/Sources/VValueSingle.h"
#include "Kernel/Sources/VValueMultiple.h"
#include "Kernel/Sources/VChecksumMD5.h"

#include "Kernel/Sources/VFullURL.h"


//#include "Kernel/Sources/VRefCountDebug.h"
	
/** IShellServices
@remarks
	pure virtual interface for VShellServices: this pure virtual class can be used to expose VShellServices (declared in main app)
	in any dll or component (see VStyledTextEditView (GUI) for instance)
*/
class IShellServices
{
public:
	virtual bool OpenURL( const XBOX::VString& inURL, bool inShouldEncode = true) = 0;

	virtual void OpenFileInBrowser( XBOX::VFile *inFile) = 0;
};


#if _WIN32
	#pragma pack( pop )
#else
	#pragma options align = reset
#endif

#endif
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
#ifndef __VRefCountDebug__
#define __VRefCountDebug__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VTextTypes.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VSyncObject.h"

#include <map>

BEGIN_TOOLBOX_NAMESPACE

typedef std::map<VString, sLONG> DebugInfoMap;
typedef std::map<const void*, DebugInfoMap> RefCountDebugInfoMap;

class XTOOLBOX_API VRefCountDebug
{
	public:
		VRefCountDebug();
		virtual ~VRefCountDebug();
		void RetainInfo(const void* object, const char* info);
		Boolean ReleaseInfo(const void* object, const char* info);
		Boolean ReleaseInfo(const void* object);

		static VRefCountDebug sRefCountDebug;

	protected:
		RefCountDebugInfoMap fDebugInfos;
		VCriticalSection* fDebugInfosMutex;
		sLONG fDebugInfoCount;

};

END_TOOLBOX_NAMESPACE

#endif

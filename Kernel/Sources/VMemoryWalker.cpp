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
#include "VMemoryWalker.h"
#include "VArrayValue.h"
#include "VIntlMgr.h"
#include "VString.h"


#if VERSIONDEBUG

VDumpLeaksWalker::VDumpLeaksWalker(VLeaksChecker* inLeaksChecker)
{
	fLeaksChecker = inLeaksChecker;
	fLeaksFile = fLeaksChecker->OpenDumpFile("VObject");
}


VDumpLeaksWalker::~VDumpLeaksWalker()
{
	if (fLeaksFile != NULL)
	{
		fclose(fLeaksFile);
		fLeaksFile = NULL;
	}
}


bool VDumpLeaksWalker::Walk(const VDebugBlockInfo& inBlock)
{
	if (inBlock.GetStackCrawl().IsFramesLoaded())
	{
		if (inBlock.IsVObject())
		{
			char className[512];
			const VObject* obj = inBlock.GetAsVObject(className, sizeof(className));
			if (obj != NULL)
			{
				fprintf(fLeaksFile, "%s (%d bytes)\n", className, (int) inBlock.GetUserSize());
				char dumpInfo[512];
				char dumpInfo2[512];
				sLONG size = sizeof(dumpInfo)-2;
				CharSet cs = obj->DebugDump(dumpInfo, size);
				if (!VIntlMgr::DebugConvertString(dumpInfo, size, cs, dumpInfo2, size, VTC_DefaultTextExport))
					size = 0;
				dumpInfo2[size] = 0;
				if (size != 0)
					fprintf(fLeaksFile, "Dump: %s\n", dumpInfo2);
			}
			else
			{
				fprintf(fLeaksFile, "Bad VObject (%d bytes)\n", (int) inBlock.GetUserSize());
			}
		}
		else
		{
			fprintf(fLeaksFile, "%s (%d bytes)\n", "not a VObject", (int) inBlock.GetUserSize());
		}
		inBlock.GetStackCrawl().Dump(fLeaksFile);
		fprintf(fLeaksFile, "\n");
	}
	return true;
}


#pragma mark-

VGetUsageWalker::VGetUsageWalker(VArrayString* inTabClassNames, VArrayLong* inTabNbObjects, VArrayLong* inTabTotalSize)
{
	fTabClassNames = inTabClassNames;
	fTabNbObjects = inTabNbObjects;
	fTabTotalSize = inTabTotalSize;
}


VGetUsageWalker::~VGetUsageWalker()
{
}


bool VGetUsageWalker::Walk(const VDebugBlockInfo& inBlock)
{
	const VObject* obj = NULL;
	VStr255 uniClassName;

	char className[512];
	if (inBlock.IsVObject())
		obj = inBlock.GetAsVObject(className, sizeof(className));

	if (obj != NULL)
		uniClassName.FromCString(className);
	else
		uniClassName.FromString(CVSTR("__anonymous__"));

	sLONG index = fTabClassNames->Find(uniClassName);
	if (index > 0)
	{
		fTabNbObjects->FromLong(fTabNbObjects->GetLong(index) + 1, index);
		fTabTotalSize->FromLong(fTabTotalSize->GetLong(index) + (sLONG) inBlock.GetUserSize(), index);
	}
	else
	{
		fTabClassNames->AppendString(uniClassName);
		fTabNbObjects->AppendLong(1);
		fTabTotalSize->AppendLong((sLONG) inBlock.GetUserSize());
	}

	return true;
}

#endif
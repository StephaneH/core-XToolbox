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
#include "VString.h"

#include "XLinuxStackCrawl.h"

#include <execinfo.h>
#include <stdio.h>
#include <dlfcn.h>




bool XLinuxStackCrawl::operator < (const XLinuxStackCrawl& other) const
{
	bool res;
	if (fCount == other.fCount)
	{
		res = false;
		for (uLONG i = 0; i < fCount; ++i)
		{
			if (fFrames[i] < other.fFrames[i])
			{
				res = true;
				break;
			}
		}
	}
	else
	{
		if (fCount <= 0 && other.fCount <= 0)
			res = false;
		else
			res = (fCount < other.fCount);
	}
	return res;
}


void XLinuxStackCrawl::LoadFrames( uLONG inStartFrame, uLONG inNumFrames)
{
	xbox_assert( inStartFrame < 10);
	xbox_assert( inNumFrames < kMaxScrawlFrames);

	void *frames[kMaxScrawlFrames + 10];
	int count = backtrace( frames, kMaxScrawlFrames+10);
	if (count >= 0 && (uLONG) count > inStartFrame)
	{
		fCount = (uLONG) std::min(count - inStartFrame, inNumFrames);
		memmove( fFrames, &frames[inStartFrame], fCount * sizeof( void*));
	}
	else
		fCount = 0;
}


void XLinuxStackCrawl::Dump( FILE* inFile) const
{
	if (fCount > 0)
	{
		char** strs = backtrace_symbols( fFrames, fCount);
		for (uLONG i = 0; i < fCount; ++i)
			fprintf( inFile, "  :: frame %u: %s\n", (unsigned int)(fCount - i - 1), strs[i]);
		free(strs);
	}
}


void XLinuxStackCrawl::Dump( VString& ioString) const
{
	bool someUnresolved=false;

	VString address;
	VString frameIndex;

	for (uLONG i = 0; i < fCount ; ++i)
	{
		// Using dladdr is easier than scanning backtrace_symbols result !
		Dl_info info;
		memset(&info, 0, sizeof(info));
		int res = dladdr(fFrames[i], &info);

		// DebugMsg("addr %x {%s ; %x ; %s ; %x}\n", fFrames[i], info.dli_fname, info.dli_fbase, info.dli_sname, info.dli_saddr);

		frameIndex.Clear();
		frameIndex.FromULong8(fCount - i - 1);

		address.Clear();
		address.FromHexLong((uLONG8)(fFrames[i]), true);

		VString symbol;

		if(res!=0 && info.dli_sname!=NULL)
		{
			VSystem::DemangleSymbol(info.dli_sname, symbol);
		}
		else
		{
			symbol = CVSTR("<inline>");
			someUnresolved=true;
		}

		ioString += CVSTR("   {");
		ioString += frameIndex;
		ioString += CVSTR("} ");
		ioString += address;
		ioString += CVSTR(" - ");
		ioString += symbol;
		ioString += '\n';
	}

	#if VERSIONDEBUG
	if (someUnresolved)
		ioString += CVSTR("   (You might want to try addr2line on unresolved symbols)\n");
	#endif
}



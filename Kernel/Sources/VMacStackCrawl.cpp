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
#include "VMacStackCrawl.h"

#include <execinfo.h>
#include <stdio.h>


bool XMacStackCrawl::operator < (const XMacStackCrawl& other) const
{
	bool res;
	if (fCount == other.fCount)
	{
		res = false;
		for (int i = 0; i < fCount; ++i)
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

void XMacStackCrawl::LoadFrames( uLONG inStartFrame, uLONG inNumFrames)
{
	xbox_assert( inStartFrame < 10);
	xbox_assert( inNumFrames < kMaxScrawlFrames);

	void *frames[kMaxScrawlFrames+10];
	int count = backtrace( frames, kMaxScrawlFrames+10);
	if (count > inStartFrame)
	{
		fCount = std::min( count - inStartFrame, inNumFrames);
		memmove( fFrames, &frames[inStartFrame], fCount * sizeof( void*));
	}
	else
	{
		fCount = 0;
	}
}

void XMacStackCrawl::Dump( FILE* inFile) const
{
	if (fCount > 0)
	{
		char** strs = backtrace_symbols( fFrames, fCount);
		for( int i = 0; i < fCount; ++i)
		{
			fprintf( inFile, "%s\n", strs[i]);
		}
		free(strs);
	}
}


void XMacStackCrawl::Dump( VString& ioString) const
{
	if (fCount > 0)
	{
		bool first = true;
		char** strs = backtrace_symbols( fFrames, fCount);
		for( int i = 0; i < fCount; ++i)
		{
			long int n;
			char module[40];
			unsigned long int address;
			char symbol[2048];
			long int offset;
			int r = sscanf( strs[i], "%ld %s %lx %2000s + %ld", &n, module, &address, symbol, &offset);
			
			if ( (r >= 4) && strcmp( module, "???") != 0 && strcmp( symbol, "0x0") != 0)
			{
				VString s;
				VSystem::DemangleSymbol( symbol, s);
				if (!first)
					ioString += "\n";
				first = false;
				ioString += s;
			}
		}
		free(strs);
	}
}

//example
#if 0

0   KernelDebug                         0x0478a192 _ZNK4xbox5VFile4OpenENS_10FileAccessEPPNS_9VFileDescEm + 208
1   4D                                  0x00bbb4af _Z10do_opendoc11OpenDocModeP13runtime4dLink + 1456
2   4D                                  0x002cec43 _Z9runtime4dR13runtime4dLink + 4452
3   4D                                  0x0056a8e4 _ZL17calcul_subanalysesP17champvar_templateILi256EEP9calcblockP10calculLink + 1197
4   4D                                  0x0056aa07 _ZL7do_procP11exploreLink + 79
5   4D                                  0x0056af1c _ZL7explorehP10calculLink + 1218
6   ???                                 0x00000000 0x0 + 0
7   ???                                 0x00000000 0x0 + 0
8   ???                                 0x00000000 0x0 + 0
9   ???                                 0x00000000 0x0 + 0
10  ???                                 0x00000000 0x0 + 0
11  ???                                 0x00000000 0x0 + 0
12  ???                                 0x00000000 0x0 + 0
13  ???                                 0x00000000 0x0 + 0
14  ???                                 0x00000000 0x0 + 0
15  ???                                 0x00000000 0x0 + 0
16  ???                                 0x00000000 0x0 + 0
17  ???                                 0x80000000 0x0 + 2147483648
18  ???                                 0x2189909c 0x0 + 562663580
19  ???                                 0x00000005 0x0 + 5
20  ???                                 0x00000000 0x0 + 0
21  ???                                 0x00000000 0x0 + 0
22  ???                                 0x00000000 0x0 + 0
23  ???                                 0x00000000 0x0 + 0

#endif

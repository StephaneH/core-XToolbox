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
#ifndef __VMacStackCrawl__
#define __VMacStackCrawl__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VStackCrawl.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API XMacStackCrawl
{
public:
			XMacStackCrawl():fCount(0) {}
	virtual ~XMacStackCrawl() {}

	bool operator < (const XMacStackCrawl& other) const;

	// Stack frames support
	void	LoadFrames( uLONG inStartFrame, uLONG inNumFrames);
	void	UnloadFrames()	{}

	bool	IsFramesLoaded() const	{ return fCount > 0;}

	// Stack crawling support
	void	Dump (FILE* inFile) const;
	void	Dump (VString& ioString) const;
	
	// Operators
	void*	operator new(size_t inSize, void* inAddr) { return inAddr; }

	// Class utilities
	static	void	Init ()	{}
	static	void	DeInit () {}

	static	void	RegisterUser (void* /*inInstance*/) {}
	static	void	UnregisterUser (void* /*inInstance*/) {}

private:
			void*	fFrames[kMaxScrawlFrames];
			int		fCount;
};


typedef XMacStackCrawl VStackCrawl;

END_TOOLBOX_NAMESPACE

#endif
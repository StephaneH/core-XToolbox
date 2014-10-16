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
#ifndef __VWinStackCrawl__
#define __VWinStackCrawl__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VStackCrawl.h"

BEGIN_TOOLBOX_NAMESPACE

struct SStackFrame {
	char			name[256];	// name of the function
	const void*		start;		// pointer to the start of the function
	uLONG_PTR		offset;		// offset from the start of the function to the PC (C2664)
};


class XTOOLBOX_API XWinStackCrawl
{
public:
	XWinStackCrawl () { fNumFrames = -1; };

	bool operator < (const XWinStackCrawl& other) const;

	// Stack frames support
	void LoadFrames (uLONG inStartFrame, uLONG inNumFrames);
	void UnloadFrames()	{ fNumFrames = -1; };

	Boolean IsFramesLoaded() const { return fNumFrames >= 0; };

	// Stack crawling support
	void Dump (FILE *inFile) const;
	void Dump (VString& ioString) const;
	
	// Class utilities
	static void	Init();
	static void	DeInit();

	static void	RegisterUser (void *inInstance);
	static void	UnregisterUser (void *inInstance);
	
private:
	const void*	fFrame[kMaxScrawlFrames];	// frames starting at startFrame
	uLONG  	fNumFrames;
	
	void	ReadFrame (const void *inAddr, SStackFrame& outFrame) const;
};

typedef XWinStackCrawl VStackCrawl;

END_TOOLBOX_NAMESPACE

#endif
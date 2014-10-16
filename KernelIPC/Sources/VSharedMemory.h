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
#ifndef __VSharedMemory__
#define __VSharedMemory__


#if VERSIONMAC || VERSION_LINUX
	#include "XSysVIPC.h"
#elif VERSIONWIN
	#include "XWinIPC.h"
#endif



BEGIN_TOOLBOX_NAMESPACE



class XTOOLBOX_API VSharedMemory : public VObject
{
public:
	VSharedMemory() : fSize(0), fAddr(NULL), fState(Invalid) {}

	VError	Init(uLONG inKey, VSize inSize);
	bool	IsNew();	//Returns true if a new shared mem. was created on Init() and false if an existing shared mem. was found
	void*	GetAddr();	//Returns NULL on attach error
	VSize	GetSize()	{ return fSize; }
	VError	Detach();

#if VERSIONMAC || VERSION_LINUX	
	VError  Remove();
#endif

private:
	typedef enum {Invalid, Detached, Attached} State;

	XSharedMemoryImpl	fImpl;
	VSize				fSize;
	void*				fAddr;
	State				fState;
};


END_TOOLBOX_NAMESPACE

#endif

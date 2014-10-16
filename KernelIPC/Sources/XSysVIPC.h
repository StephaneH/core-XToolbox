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
#ifndef __XIPCSysV__
#define __XIPCSysV__



BEGIN_TOOLBOX_NAMESPACE



class XTOOLBOX_API XSysVSemaphore : public VObject
{
public:
	XSysVSemaphore() : fId(-1), fIsNew(false) {};

	VError	Init(uLONG inKey, uLONG inInitCount);
	bool	IsNew()	{ return fIsNew; }

	VError	Lock();
	VError	Unlock();
	VError  Remove();
	
private:
	VError	Get(uLONG inKey);
	VError	Init(uLONG inInitCount);
	
	int		fId;
	bool	fIsNew;
};

typedef XSysVSemaphore XSharedSemaphoreImpl;


class XTOOLBOX_API XSysVSharedMemory : public VObject
{
public:
	XSysVSharedMemory() : fId(-1), fIsNew(false), fAddr(NULL) {};

	VError	Init(uLONG inKey, VSize inSize);
	bool	IsNew()	{ return fIsNew; }
	VError	Attach();
	void*	GetAddr();
	VError	Detach();
	VError  Remove();
	
	static VError Detach(void* inAddr);

private:
	int		fId;
	bool	fIsNew;
	void*	fAddr;
};

typedef XSysVSharedMemory XSharedMemoryImpl;





END_TOOLBOX_NAMESPACE



#endif

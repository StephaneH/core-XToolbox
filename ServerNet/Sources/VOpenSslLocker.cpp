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
#include "VServerNetPrecompiled.h"
#include "VOpenSslLocker.h"



BEGIN_TOOLBOX_NAMESPACE


	
//virtual
VOpenSslLocker::~VOpenSslLocker()
{
	delete[] fLocks;
	fLocks=NULL;
}


//virtual
void VOpenSslLocker::Init(int inCount)
{
	if(fLocks==NULL)
	{
		fLocks=new VCriticalSection[inCount];
		fCount=inCount;
	}
}


//virtual
void VOpenSslLocker::Lock(int inIndex)
{
	bool res=false;
	
	if(fLocks!=NULL && inIndex>=0 && inIndex<fCount)
		res=fLocks[inIndex].Lock();
	
	xbox_assert(res);
}


//virtual
void VOpenSslLocker::Unlock(int inIndex)
{
	bool res=false;
	
	if(fLocks!=NULL && inIndex>=0 && inIndex<fCount)
		res=fLocks[inIndex].Unlock();
	
	xbox_assert(res);
}


//virtual
unsigned long VOpenSslLocker::GetTaskID()
{
	VTaskID signedId=VTaskMgr::Get()->GetCurrentTaskID();
	
	//Task manager give us negative Ids for tasks it doesn't own... But we need positive task Ids !
	return signedId>=0 ? signedId : ULONG_MAX+signedId ;
}


//static
IOpenSslLocker* VOpenSslLocker::GetOpenSSlLocker()
{	
	static VOpenSslLocker* locker=NULL;
	
	if(locker==NULL)
		locker=new VOpenSslLocker();
		
	return locker;
}


END_TOOLBOX_NAMESPACE
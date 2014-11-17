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
#include "VKernelIPCPrecompiled.h"
#include "XLinuxFileSystemNotification.h"

#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/XLinuxFsHelpers.h"


#include <signal.h>
#include <time.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <utility>
#include <unistd.h>

//jmo - We need to size the event buffer. Let's say that the relative path of an event is up to 48 bytes.
//      It gives us 64 bytes for an event struct. A 200 events buffer of 12800 bytes sounds fine to me...

const int CHANGE_BUF_SIZE=12800;



////////////////////////////////////////////////////////////////////////////////
//
// WatchedFolder
//
////////////////////////////////////////////////////////////////////////////////

VError XFSN::WatchedFolder::InotifyRegister(int inId)
{
    fId=inId;

    uint32_t mask=IN_CREATE|IN_DELETE|IN_MODIFY|IN_MOVED_FROM|IN_MOVED_TO;

    PathBuffer tmpBuf;    

    VError verr=tmpBuf.Init(fFolderPath);

    if(verr!=VE_OK)
        return verr;

    fWd=inotify_add_watch(fId, tmpBuf.GetPath(), mask);

    if(fWd<0)
        return vThrowError(errno);
    
	return VE_OK;
}


VError XFSN::WatchedFolder::InotifyUnregister()
{
    return inotify_rm_watch(fId, fWd)==0 ? VE_OK : vThrowNativeError(errno);
}


VError XFSN::WatchedFolder::SignalChange(VFSN* inVfsn)
{
	if(inVfsn==NULL)
		return VE_INVALID_PARAMETER;

	for(EventIterator it=fEvents.begin() ; it!=fEvents.end() ; ++it)
		if(it->second!=VFSN::kNone)
		{
			PathBuffer tmpBuf;
			VError verr=tmpBuf.Init(fFolderPath);

			if(verr!=VE_OK)
				return verr;

			verr=tmpBuf.AppendName(it->first.c_str());

			if(verr!=VE_OK)
				return verr;

			VFilePath fullPath;
			
			tmpBuf.ToPath(&fullPath);
			
			int tabIndex=0;

			switch(it->second)
			{
			case VFSN::kFileAdded		: tabIndex=1 ; break;
			case VFSN::kFileDeleted		: tabIndex=2 ; break;
			case VFSN::kFileModified	: tabIndex=0 ; break;
			default: tabIndex = 0; break;
			}

            //protect access to VChangeData::fData
            {
                VTaskLock lock(&inVfsn->fMutex);
                fOwner->fData[tabIndex].push_back(fullPath);
            }
		}

	inVfsn->SignalChange(fOwner);
	
	fChanged=false;
	
	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////
//
// XLinuxFileSystemNotification
//
////////////////////////////////////////////////////////////////////////////////

VError XFSN::Init()
{
    fId=inotify_init();

    if(fId<0)
        return vThrowNativeError(errno);

	if(fWatchTask==NULL || fWatchTask->IsDying())
	{
        if(fWatchTask!=NULL)
            ReleaseRefCountable(&fWatchTask);

		fWatchTask=new VTask(this, 0, eTaskStylePreemptive, LaunchWatchTask);
		fWatchTask->SetKindData((sLONG_PTR)this);
		fWatchTask->Run();
	}

	return VE_OK;
}


XFSN::~XLinuxFileSystemNotifier()
{
	if(fWatchTask!=NULL)
    {
        fWatchTask->Kill();

        //It's over, we want to quit asap :
        // - We don't want to wait in select() or others
        // - It's ok if we don't release mutexes because they are about to be freed anyway.

        //pthread_cancel(fWatchTask->GetSystemID());
        //pthread_join(fWatchTask->GetSystemID(), NULL);

        ReleaseRefCountable(&fWatchTask);
    }

    close(fId);
}


//static
sLONG XFSN::LaunchWatchTask(VTask *inTask )
{
	xbox_assert(inTask!=NULL);

	inTask->SetName("File System Notification Task");

	XFSN* xfsn=(XFSN*)inTask->GetKindData();
	
	xfsn->WatchAndNotify();
	
	return 0;
}


VError XFSN::StartWatchingForChanges(const VFolder &inFolder, EventKind inFilter, IEventHandler *inHandler, sLONG inLatency)
{
	VFilePath path;
	inFolder.GetPath(path);

	XLinuxChangeData* data=new XLinuxChangeData(path, inFilter, VTask::GetCurrent(), inHandler, this);

	if(AddToFolderMap(data)!=VE_OK)
		return vThrowError(VE_START_WATCHING_FOLDER_FAILED);
		
	fOwner->PushChangeData(data);	//VFSN should remember (and retain) the directory it's watching


    //It might take up to one period for the timeout to be updated, so be careful with long timeout !
    inLatency = inLatency<1 ? 1 : inLatency;
    inLatency = inLatency>1000 ? 1000 : inLatency;

    VInterlocked::Exchange(&fMsTimeout, inLatency);

	return VE_OK;
}


VError XFSN::StopWatchingForChanges(VChangeData *inChangeData, bool inIsLastOne /*unused*/)
{
	XLinuxChangeData* data=dynamic_cast<XLinuxChangeData*>(inChangeData);
	xbox_assert(data!=NULL);

	if(RemoveFromFolderMap(data)!=VE_OK)
		return vThrowError(VE_STOP_WATCHING_FOLDER_FAILED);

    ReleaseRefCountable(&data);

	return VE_OK;
}


VError XFSN::AddToFolderMap(XLinuxChangeData* inData)
{
	if(inData==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	VError verr=VE_OK;
	
    //	if((verr=LockFolders())!=VE_OK)
	//	return verr;

    VTaskLock lock(&fOwner->fMutex);

	WatchedFolder* folder=inData->GetWatchedFolder();

	if((verr=folder->InotifyRegister(fId))!=VE_OK)
		return verr;	

	fFolderMap.insert(std::make_pair(folder->fWd, folder));

	// if((verr=UnlockFolders())!=VE_OK)
	// 	return verr;

	return VE_OK;
}


VError XFSN::RemoveFromFolderMap(XLinuxChangeData* inData)
{
	if(inData==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	VError verr=VE_OK;
	
    //	if((verr=LockFolders())!=VE_OK)
	//	return verr;

    VTaskLock lock(&fOwner->fMutex);

	WatchedFolder* folder=inData->GetWatchedFolder();

	verr=folder->InotifyUnregister();
	xbox_assert(verr==VE_OK);	

	fFolderMap.erase(folder->fWd);
    
	// if((verr=UnlockFolders())!=VE_OK)
	// 	return verr;

	return VE_OK;
}


// VError XFSN::LockFolders()
// {
//     bool res=fFolderMapLock.Lock();

//     return res ? VE_OK : vThrowNativeError(errno);  //jmo - revoir ca !
// }


// VError XFSN::UnlockFolders()
// {
//     bool res=fFolderMapLock.Unlock();

//     return res ? VE_OK : vThrowNativeError(errno);  //jmo - revoir ca !
// }


VError XFSN::WatchAndNotify()
{
    sLONG msTimeout=VInterlocked::AtomicGet(&fMsTimeout);

 	timeval timeout={0,0};

    while(msTimeout>=1000)
        timeout.tv_sec++, msTimeout-=1000;

    timeout.tv_usec=msTimeout*1000;
	
    for(;;)
	{
        if(VTask::GetCurrent()->IsDying())
        {
            break;
        }
	
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fId, &readSet);

        int res=select(fId+1, &readSet, NULL, NULL, &timeout);

        char buf[CHANGE_BUF_SIZE];
        ssize_t n = (res==1) ? read(fId, buf, sizeof(buf)) : 0;

        xbox_assert((res==0) || (res==1 && n>0) || (res==-1 && errno==EINTR)); 

        if(n>0 /*&& LockFolders()==VE_OK*/)
        {
            VTaskLock lock(&fOwner->fMutex);

            char* pos=buf;
            char* past=buf+n;

            while(pos+sizeof(inotify_event)<past)
            {
                pthread_testcancel();   //jmo - Exit asap if we are canceled
                if(VTask::GetCurrent()->IsDying())
                {
                    break;
                }

                inotify_event* evPtr=(inotify_event*)pos;
				
                FolderIterator folderIt=fFolderMap.find(evPtr->wd);

                if(folderIt==fFolderMap.end())
                {
                    pos+=sizeof(inotify_event)+evPtr->len;
                    continue;
                }

                std::string name(evPtr->name, evPtr->len);
                EventKind ev=VFSN::kNone;
						
                if(evPtr->mask&IN_CREATE || evPtr->mask&IN_MOVED_TO)
                    ev=VFSN::kFileAdded;
                else if(evPtr->mask&IN_DELETE || evPtr->mask&IN_MOVED_FROM)
                    ev=VFSN::kFileDeleted;
                else if(evPtr->mask&IN_MODIFY)
                    ev=VFSN::kFileModified;
                
                xbox_assert(ev!=VFSN::kNone);
						
                WatchedFolder::EventIterator eventIt;
								
                eventIt=folderIt->second->fEvents.find(name);
						
                EventKind oldStatus=VFSN::kNone;
                
                if(eventIt!=folderIt->second->fEvents.end())
                    oldStatus=eventIt->second;

                EventKind newStatus=oldStatus;
				
                if(oldStatus==VFSN::kFileAdded && ev==VFSN::kFileDeleted)
                    newStatus=VFSN::kNone;
                else if(oldStatus==VFSN::kFileDeleted && ev==VFSN::kFileAdded)
                    newStatus=VFSN::kFileModified;
                else if(oldStatus==VFSN::kFileModified && ev==VFSN::kFileDeleted)
                    newStatus=VFSN::kFileDeleted;
                else if(oldStatus==VFSN::kNone)
                    newStatus=ev;
                
                if(eventIt!=folderIt->second->fEvents.end() && newStatus!=oldStatus)
                {
                    eventIt->second=newStatus;
                    folderIt->second->fChanged=true;
                }
                else
                {
                    folderIt->second->fEvents.insert(std::make_pair(name, newStatus));
                    folderIt->second->fChanged=true;
                }
				
                pos+=sizeof(inotify_event)+evPtr->len;
            }

            // VError verr=UnlockFolders();
            // xbox_assert(verr==VE_OK);
        }

		if(timeout.tv_sec==0 && timeout.tv_usec<1000)
		{
			// if(LockFolders()==VE_OK)
			// {
				for(FolderIterator folderIt=fFolderMap.begin() ; folderIt!=fFolderMap.end() ; ++folderIt)
				{
                    pthread_testcancel();   //jmo - Exit asap if we are canceled
                    if(VTask::GetCurrent()->IsDying())
                    {
                        break;
                    }

					if(folderIt->second->fChanged)
						folderIt->second->SignalChange(fOwner);						
					
					folderIt->second->fEvents.clear();
				}
					
			// 	VError verr=UnlockFolders();
			// 	xbox_assert(verr==VE_OK);
			// }

            msTimeout=VInterlocked::AtomicGet(&fMsTimeout);
			
			//timeout={0};
			memset(&timeout, 0, sizeof(timeout));

			while(msTimeout>=1000)
                timeout.tv_sec++, msTimeout-=1000;
            
            timeout.tv_usec=msTimeout*1000;
		}
	}
	
	return VE_OK;
}

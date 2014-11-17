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

#include "VProcessIPC.h"

#if VERSIONMAC
#include "XMacFileSystemNotification.h"
#elif VERSIONWIN
#include "XWinFileSystemNotification.h"
#elif VERSION_LINUX
#include "XLinuxFileSystemNotification.h"
#endif


//=========================================================================================================


VFileSystemNotifier::VChangeData::VChangeData( const VFilePath& inPath, EventKind inFilter, VTask *inTargetTask, IEventHandler *inCallBack, XFileSystemNotification *inImplementation)
: fPath( inPath)
, fFilters( inFilter)
, fCallback( inCallBack)
, fTargetTask( RetainRefCountable( inTargetTask))
, fThisPointer( inImplementation)
{
}


VFileSystemNotifier::VChangeData::~VChangeData()
{
	ReleaseRefCountable( &fTargetTask);
}


//=========================================================================================================


VFileSystemNotifier::VFileSystemNotifier()
{
	fImpl = new XFileSystemNotification( this);

#if VERSION_LINUX

    VError verr=fImpl->Init();
    xbox_assert(verr==VE_OK);

#endif
}


VFileSystemNotifier::~VFileSystemNotifier()
{
	while( !fWatchList.empty())
	{
		VRefPtr<VChangeData> refData = fWatchList.back();
		fWatchList.pop_back();

		fImpl->StopWatchingForChanges( refData, fWatchList.empty());
	}
	delete fImpl;
}


VFileSystemNotifier *VFileSystemNotifier::Instance()
{
	return VProcessIPC::Get()->GetFileSystemNotifier();
}


VError VFileSystemNotifier::StartWatchingForChanges( const VFolder& inFolder, EventKind inKindFilter, IEventHandler *inHandler, sLONG inLatency )
{
	VTaskLock lock( &fMutex);

	// Let's see if there's something already watching for changes for this handler and directory.  If there
	// is, we are going to define it as an error.  But we might want to consider changing the event filter.  The
	// worry is: what do we do if we already have queued up data?  Do we flush it?  It's possible we have queued 
	// data for a filter the caller no longer wants.  Do we dump that data?  Who knows.
	for( VectorOfChangeData::iterator i = fWatchList.begin(); i != fWatchList.end(); ++i)
	{
		if ((*i)->fPath == inFolder.GetPath() && (*i)->fCallback == inHandler)
		{
			// We found an entry for the folder, so bail out
			return VE_FOLDER_ALREADY_EXISTS;
		}
	}

	return fImpl->StartWatchingForChanges( inFolder, inKindFilter, inHandler, inLatency);
}


VError VFileSystemNotifier::StopWatchingForChanges( const VFolder &inFolder, IEventHandler *inHandler )
{
	VTaskLock lock( &fMutex);

	VRefPtr<VChangeData> data;
	for( VectorOfChangeData::iterator i = fWatchList.begin() ; i != fWatchList.end(); ++i)
	{
		if ((*i)->fPath == inFolder.GetPath() && (*i)->fCallback == inHandler)
		{
			data = *i;
			
			// Let's remove it from the watch list, since we already know where it's at
			fWatchList.erase( i);
			break;
		}
	}
	if (data == NULL)
		return VE_FOLDER_NOT_FOUND;

	return fImpl->StopWatchingForChanges( data, fWatchList.empty());
}


void VFileSystemNotifier::SignalChange( VChangeData *inChangeData)
{
	VChangeMessage *msg = new VChangeMessage( this, inChangeData);
	if (msg != NULL)
		msg->PostTo( inChangeData->fTargetTask, false);	// always post even if same task to get a well defined context
	ReleaseRefCountable( &msg);
}


void VFileSystemNotifier::EventSignaledCallback( VChangeData *inData)
{
	VTaskLock lock( &fMutex);

	// sc 07/07/2010 Wakanda bugtrack 744 : the watching may have been stopped
	VRefPtr<VChangeData> refData( inData);
	if (std::find( fWatchList.begin(), fWatchList.end(), refData) != fWatchList.end())
	{
		// And fire off events for all of the data that we have
		for (int i = 0; i < 3; i++)
		{
			if (!inData->fData[ i ].empty())
			{
				// swap files before calling the callback to avoid reentrency problems
				std::vector< VFilePath > files;
				inData->fData[ i ].swap( files);
				inData->fCallback->FileSystemEventHandler( files, DataIndexToEventKind( i ) );
			}
		}
	}
}

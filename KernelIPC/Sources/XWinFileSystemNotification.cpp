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
#include "XWinFileSystemNotification.h"


XWinFileSystemNotification::XWinChangeData::XWinChangeData( const VFilePath& inPath, VFileSystemNotifier::EventKind inFilter, VTask *inTargetTask, VFileSystemNotifier::IEventHandler *inCallBack, VFileSystemNotifier::XFileSystemNotification *inImplementation)
: inherited( inPath, inFilter, inTargetTask, inCallBack, inImplementation)
, fTimer( NULL)
, fFolderHandle( NULL)
, fIsCanceled( false)
{
	memset( &fOverlapped, 0, sizeof( fOverlapped));
}


bool XWinFileSystemNotification::XWinChangeData::ReadDirectoryChanges()
{
	bool done = false;
	if (!fIsCanceled)
	{
		// retain the data. It will be released in CompletionRoutine.
		BOOL ok = ::ReadDirectoryChangesW( fFolderHandle, fBuffer, sizeof( fBuffer ), true, fThisPointer->MakeFilter( fFilters ), NULL, &fOverlapped, XWinFileSystemNotification::CompletionRoutine);
		done = ok != 0;
	}
	return done;
}


XWinFileSystemNotification::XWinChangeData::~XWinChangeData()
{
	if (fFolderHandle != NULL)
		::CloseHandle( fFolderHandle );
	if (fOverlapped.hEvent != NULL)
		::CloseHandle( fOverlapped.hEvent );
	if (fTimer)
		::CloseHandle( fTimer );
}


//===========================================================================


XWinFileSystemNotification::XWinFileSystemNotification( VFileSystemNotifier *inOwner)
: fOwner( inOwner)
, fTask( NULL)
{
}


XWinFileSystemNotification::~XWinFileSystemNotification()
{
	if (fTask != NULL)
	{
		fTask->Kill();
		::PostThreadMessage( fTask->GetSystemID(), WM_QUIT, 0, 0 );
		fTask->WaitForDeath( 3000);
	}
	ReleaseRefCountable( &fTask);
}


void XWinFileSystemNotification::LaunchTaskIfNecessary()
{
	if ( (fTask == NULL) || fTask->IsDying() )
	{
		ReleaseRefCountable( &fTask);
		fTask = new VTask( this, 0, eTaskStylePreemptive, TaskRunner );
		fTask->SetKindData( (sLONG_PTR)this );
		fTask->Run();
	}
}


void CALLBACK XWinFileSystemNotification::TimerProc( LPVOID inArg, DWORD, DWORD )
{
	// We're passing in the ChangeData object as the lone argument to this function, and
	// we know that our latency timer has elapsed.  Thus, we know it's time to start firing
	// some events!  Blam, blam!
	XWinChangeData *data = (XWinChangeData *)inArg;
	if (!data->fIsCanceled)
		data->fThisPointer->fOwner->SignalChange( data );
}


void CALLBACK XWinFileSystemNotification::CompletionRoutine( DWORD inErrorCode, DWORD inBytesTransferred, LPOVERLAPPED inOverlapped )
{
	// We have a list of folders that we are watching, and we want to test to see if any
	// of them have met their change filter requirements yet.  When we get a new folder to
	// watch, we open up the folder's handle and we map it to the parameters required by
	// the ReadDirectoryChangesW API.  Then we have a list of this information socked away
	// that we can then use in a loop.  The basic idea is to test for changes, and if there
	// are some changes, add the information to a queue of data to return to the user.
	//
	// However, we don't return the data to the user immediately.  The user is able to pick
	// a latency for the queue, so that we can return batches of notifications to the user
	// instead of returning them as they happen.  This is important for some uses, because it
	// may be possible for the user to make a lot of changes very quickly (imaging a Perforce
	// fetch, if you will).  A consumer could become quickly swamped with very expensive notification
	// calls unless we queued them up into a list.
	//
	// Since this is being called from a preemptive thread, and the calls to ReadDirectoryChangesW
	// will block if called synchronously, we need to use overlapped (asynchronous) calls in order
	// to process multiple directory watches via a single thread.  We can basically test each handle
	// with a wait time of "none."
	//
	// Once a handle has a change associated with it, we need to scrape the change information out
	// into its own buffer.  Then we can update the latency value associated with the handle so that
	// we don't fire an event immediately for the change.  After all of the handles are updated, we
	// can loop over them again to see if any are ready to send out some notifications, based on whether
	// they have queued data to report, as well as their latency counter.
	//
	// We've been a bit sneaky with the overlapped structure that we pass in.  It's actually a pointer
	// to more memory than the overlapped structure itself -- it's a pointer to our ChangeData structure,
	// of which the OVERLAPPED part happens to be first.  This gives us access to all of the information
	// we need to care about.

	XWinChangeData *data = (XWinChangeData *) ( (char*) inOverlapped - offsetof(XWinChangeData,fOverlapped));

	xbox_assert( data->GetRefCount() > 0);

	if (!VTask::GetCurrent()->IsDying() && (inBytesTransferred != 0) && (inErrorCode != ERROR_OPERATION_ABORTED) )
	{
		// We have all of the data from a complete event sitting in our buffer.  We need to copy out
		// the data that we care about.  However, we may get more than one notification for a single
		// pass through the loop.  So we need to loop over the result set as well.
		char *bufferptr = (char *)data->fBuffer;
		bool done = false;
		do {
			FILE_NOTIFY_INFORMATION *finfo = (FILE_NOTIFY_INFORMATION *)bufferptr;
			VString fileName;
			fileName.FromBlock( finfo->FileName, finfo->FileNameLength, VTC_UTF_16_SMALLENDIAN );

			// Add the result to the proper vector of results
			data->fThisPointer->AddResultToProperList( data, fileName, finfo->Action );

			// Either advance to the next record, or we're done reporting changes for this folder
			if (finfo->NextEntryOffset) {
				bufferptr += finfo->NextEntryOffset;
			} else {
				done = true;
			}
		} while (!done);

		// Check to see whether we need to create a latency timer or not.  If we don't, we can fire the events
		// manually from here.  But if we have a latency, we need to make a timer to handle it.  If we already
		// have a timer created -- then we need to cancel the old one and reset the new one!
		if (!data->fLatency)
		{
			data->fThisPointer->fOwner->SignalChange( data );
		}
		else if (data->fTimer)
		{
			// We already have a timer, so we want to cancel it -- we'll be resetting it as needed
			::CancelWaitableTimer( data->fTimer );

			// Set the period of the timer to our latency.  Our latency is in milliseconds, but the timer is
			// in 100-nanosecond intervals.  A negative value denotes a relative time, which is what we're interested
			// in.  There are 10,000 100-nanosecond intervals in a millisecond.
			LARGE_INTEGER dueTime = { 0 };
			dueTime.QuadPart = -data->fLatency * 10000;
			::SetWaitableTimer( data->fTimer, &dueTime, 0, TimerProc, (LPVOID)data, FALSE );
		}
		else
		{
			// We should never get here as that means we have a non-zero latency but no timer to fire the events
			xbox_assert( false );
		}

		if (!data->ReadDirectoryChanges())
		{
			// remove from active list
			VRefPtr<XWinChangeData> ptr( data);
			std::vector<VRefPtr<XWinChangeData> >::iterator k = std::find( data->fThisPointer->fActiveChanges.begin(), data->fThisPointer->fActiveChanges.end(), ptr);
			if (testAssert( k != data->fThisPointer->fActiveChanges.end()))
				data->fThisPointer->fActiveChanges.erase( k);
		}
	}
}


void XWinFileSystemNotification::RunTask()
{
	bool done = false;
	while (!done)
	{
		while (WAIT_IO_COMPLETION == ::MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE ))
			/* Do nothing */;

		// Now we know an APC message has arrived, so we can dispatch it
		MSG msg;
		while (::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
		{
			{
				VTaskLock lock( &fOwner->fMutex);
				{
					for( std::vector<VRefPtr<XWinChangeData> >::iterator i = fCanceledChanges.begin() ; i != fCanceledChanges.end() ; ++i)
					{
						// cancel io
						PrepareDeleteChangeData( *i);
	
						// remove from pending list if any
						std::vector<VRefPtr<XWinChangeData> >::iterator j = std::find( fPendingChanges.begin(), fPendingChanges.end(), *i);
						if (j != fPendingChanges.end())
							fPendingChanges.erase( j);
					}
					fCanceledChanges.clear();

					// start pending changes
					for( std::vector<VRefPtr<XWinChangeData> >::iterator i = fPendingChanges.begin() ; i != fPendingChanges.end() ; ++i)
					{
						if ((*i)->ReadDirectoryChanges())
						{
							// collect active change to retain it while a completion routine is active for it
							fActiveChanges.push_back( *i);
						}
					}
					fPendingChanges.clear();
				}
			}

			if (VTask::GetCurrent()->IsDying() || msg.message == WM_QUIT)
			{
				done = true;
				break;
			}

			::TranslateMessage( &msg );
			::DispatchMessage( &msg );
		}
	}

	// release active changes from owning thread
	fActiveChanges.clear();
}


void XWinFileSystemNotification::AddResultToProperList( XWinChangeData *inData, const VString& inFileName, int inAction )
{
	// Note that we may get notifications for things the user never asked us to report on.  
	// That is because the filters for the OS are different from the filters for the user,
	// and there can be a bit of overlap.  So we have to test whether we want the change
	// to be pushed onto a list or not, as well as which list to push onto.
	int listIndex = -1;		// No list!
	switch (inAction)
	{
		case FILE_ACTION_ADDED:				listIndex = 1; break;
		case FILE_ACTION_REMOVED:			listIndex = 2; break;
		case FILE_ACTION_MODIFIED:			listIndex = 0; break;
		case FILE_ACTION_RENAMED_OLD_NAME:	listIndex = 2; break;
		case FILE_ACTION_RENAMED_NEW_NAME:	listIndex = 1; break;
	}
	if (-1 == listIndex)
		return;

	// Check to see whether we care about events for this list index before pushing the data
	if (inData->fFilters & VFileSystemNotifier::DataIndexToEventKind( listIndex ))
	{
		// The name we get back from the ReadDirectoryChangesW API is always relative to the
		// initial folder.  So we want to construct a relative file path so that the user gets
		// back a path that fully describes the file.
		
		VString fullpath( inData->fPath.GetPath());
		fullpath += inFileName;

		// needs to know if it's a file or a folder 
		if ( (inAction != FILE_ACTION_REMOVED) && (inAction != FILE_ACTION_RENAMED_OLD_NAME) )
		{
			DWORD retval = ::GetFileAttributesW( fullpath.GetCPointer());
			if ( (retval != INVALID_FILE_ATTRIBUTES) && ( (retval & FILE_ATTRIBUTE_DIRECTORY) != 0) )
				fullpath += FOLDER_SEPARATOR;
		}

		VFilePath path( fullpath, FPS_SYSTEM);

		// protect access to VChangeData::fData
		VTaskLock lock( &fOwner->fMutex);

		// Make sure that we only add the change to the list if we've not already got it in there.
		if (std::find( inData->fData[ listIndex ].begin(), inData->fData[ listIndex ].end(), path) == inData->fData[ listIndex ].end())
		{
			inData->fData[ listIndex ].push_back( path );
		}
	}
}


DWORD XWinFileSystemNotification::MakeFilter( VFileSystemNotifier::EventKind inFilters )
{
	// We need to convert from our generic filter mask into an OS filter mask
	DWORD ret = 0;

	if (inFilters & VFileSystemNotifier::kFileModified)
		ret |= FILE_NOTIFY_CHANGE_LAST_WRITE;
	if (inFilters & VFileSystemNotifier::kFileAdded)
		ret |= (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
	if (inFilters & VFileSystemNotifier::kFileDeleted)
		ret |= (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME);
	
	return ret;
}


void XWinFileSystemNotification::WatchForChanges( XWinChangeData *inData )
{
	// ask secondary thread to call ReadDirectoryChanges
	VTaskLock lock( &fOwner->fMutex);
	fPendingChanges.push_back( inData);

	while( fTask->GetState() < TS_RUNNING)
		VTask::YieldNow();
	::PostThreadMessage( fTask->GetSystemID(), WM_APP, 0, 0 );
}

VError XWinFileSystemNotification::StartWatchingForChanges( const VFolder &inFolder, VFileSystemNotifier::EventKind inKindFilter, VFileSystemNotifier::IEventHandler *inHandler, sLONG inLatency )
{
	VString path;
	inFolder.GetPath( path );

	VError err = VE_OK;

	// Now that we know we need to care about this folder, let's see if we can create an entry for it.  First,
	// we will try to open a handle to the folder.
	HANDLE hFolder = ::CreateFileW( path.GetCPointer(), FILE_LIST_DIRECTORY | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
						NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );
	if (hFolder == INVALID_HANDLE_VALUE)
	{
		// We were unable to open the folder, so we need to bail out
		return MAKE_NATIVE_VERROR( ::GetLastError() );
	}

	// launch our task if not already there
	LaunchTaskIfNecessary();

	// Now that we've opened the folder handle, we can add an entry to our watch list
	XWinChangeData *data = new XWinChangeData( inFolder.GetPath(), inKindFilter, VTask::GetCurrent(), inHandler, this);
	data->fFolderHandle = hFolder;
	data->fLatency = inLatency;
	data->fOverlapped.hEvent = ::CreateEvent( NULL, true, false, NULL );
	data->fTimer = NULL;
	if (inLatency)
		data->fTimer = ::CreateWaitableTimer( NULL, FALSE, NULL );

	// Add the new data to our list of things to watch for
	fOwner->PushChangeData( data );

	WatchForChanges( data );

	ReleaseRefCountable( &data);

	return VE_OK;
}

void XWinFileSystemNotification::PrepareDeleteChangeData(XWinChangeData *inData )
{
	// cancel pending io operation
	inData->fIsCanceled = true;
	if (inData->fTimer)
	{
		::CancelWaitableTimer( inData->fTimer );		// This ensures we don't fire an event if we no longer care about changes
	}

	// pas besoin de CancelIoEx car c'est appele depuis la thread qui fait les IO. Et tant mieux car CancelIoEx blocke dans certains cas.
	BOOL ok = CancelIo( inData->fFolderHandle);

#if WITH_ASSERT
	DWORD err = GetLastError();
	if (!ok && err != ERROR_NOT_FOUND)
	{
		char buff[1024];
		sprintf(buff,"CancelIoEx err=%i\r\n", err);
		OutputDebugString(buff);
	}
#endif
	
	inData->fCallback = NULL;
}

VError XWinFileSystemNotification::StopWatchingForChanges( VFileSystemNotifier::VChangeData *inChangeData, bool inIsLastOne)
{
	// NOTE: We don't call the event handler at this point, but we do have to worry about the fact that
	// there can be multiple handlers trying to watch the same directory for changes at the same time.

	XWinChangeData *data = dynamic_cast<XWinChangeData*>( inChangeData);

	// check for completion state before deleting data object

	if (fTask != NULL)
	{
		fCanceledChanges.push_back( data);
		if (inIsLastOne)
		{
			fTask->Kill();
			::PostThreadMessage( fTask->GetSystemID(), WM_QUIT, 0, 0 );
		}
		else
		{
			// just to tell it to cleanup canceled changes
			::PostThreadMessage( fTask->GetSystemID(), WM_APP, 0, 0 );
		}
	}
	else
	{
		PrepareDeleteChangeData( data);
	}

	return VE_OK;
}

sLONG XWinFileSystemNotification::TaskRunner( VTask *inTask )
{
	inTask->SetName( "File System Notification Task" );
	
	// The instance of the FS notifier is stuffed into our private data
	XWinFileSystemNotification *instance = (XWinFileSystemNotification *)inTask->GetKindData();
	
	// The platform-specific implementations will override this to do whatever actions
	// they see fit.
	instance->RunTask();
	
	return 0;
}


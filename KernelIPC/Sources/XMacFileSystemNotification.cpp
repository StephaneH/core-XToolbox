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
#include "XMacFileSystemNotification.h"


#include <sys/types.h>
#include <sys/dir.h>
#include <dirent.h>

BEGIN_TOOLBOX_NAMESPACE


XMacFileSystemNotification::XMacChangeData::XMacChangeData( const VFilePath& inPath, const std::string& inPosixPath, VFileSystemNotifier::EventKind inFilter, VTask *inTargetTask, VFileSystemNotifier::IEventHandler *inCallBack, VFileSystemNotifier::XFileSystemNotification *inImplementation)
: inherited( inPath, inFilter, inTargetTask, inCallBack, inImplementation)
, fPosixPath( inPosixPath)
, fStreamRef( NULL)
{
}


XMacFileSystemNotification::XMacChangeData::~XMacChangeData()
{
	if (fStreamRef != NULL)
		FSEventStreamRelease( fStreamRef );
	ClearDirectorySnapshot( fSnapshot );
}

//===========================================================================================


XMacFileSystemNotification::XMacFileSystemNotification( VFileSystemNotifier *inOwner)
: fOwner( inOwner)
{
}


XMacFileSystemNotification::~XMacFileSystemNotification()
{
}


void XMacFileSystemNotification::CreateDirectorySnapshot( const std::string& inPosixPath, SnapshotListing &outList, bool inRecursive )
{
	xbox_assert(inPosixPath.at(inPosixPath.size()-1) == '/');
	
	DirectoryListing *ret = new DirectoryListing();
	struct dirent **entries = NULL;	// This is an array of directory entry pointers
	
	// Now we can scan the directory for a file listing in alphabetical order
	int numentries = scandir( inPosixPath.c_str(), &entries, NULL, alphasort );
	
	// Next, we can loop over all of the entries, and add them to our snapshot list.  However, 
	// we don't want to add listings for the "this directory" and "parent directory" object that
	// scandir returns to us, so we'll skip those/
	for( int i = 0; i < numentries; i++)
	{
		if (strcmp( entries[ i ]->d_name, "." ) != 0 && strcmp( entries[ i ]->d_name, ".." ) != 0 && strcmp( entries[ i ]->d_name, ".DS_Store" ) != 0 )
		{
			std::string name( entries[ i ]->d_name);

			std::string filePath( inPosixPath);
			filePath += name;

			struct stat s_obj;
			if (lstat( filePath.c_str(), &s_obj ) == 0)
			{
				FileInfo data;
				data.isDirectory = (entries[ i ]->d_type == DT_DIR);
				data.st_mtimespec = s_obj.st_mtimespec;

				(*ret)[name] = data;
				
				if (inRecursive && entries[ i ]->d_type == DT_DIR)
				{
					// We have another directory to process, so we will recurse.
					filePath += "/";
					CreateDirectorySnapshot( filePath, outList, inRecursive );
				}
			}
		}
		free( entries[i]);
	}
	free( entries);
	
	// We will free the data from the list elsewhere, which frees the data from
	// the call to scandir as well.
	outList[inPosixPath] = ret;
}


void XMacFileSystemNotification::ClearDirectorySnapshot( SnapshotListing &ioList )
{
	for (SnapshotListing::iterator outer = ioList.begin(); outer != ioList.end(); ++outer)
		delete outer->second;

	ioList.clear();
}


bool XMacFileSystemNotification::AddChangeToList( XMacChangeData *inData, const std::string& inPosixPath, const std::string& inFileName, bool inIsDirectory, VFileSystemNotifier::EventKind inKind)
{
	// We need to ensure this is actually an event the user is really interested in
	// hearing about before adding the data to the list.  We're just told "there was
	// a change" by the OS, so it's possible we don't care about this information at all.
	if (inData->fFilters & inKind)	
	{
		// The user does care about this, so let's add it to the proper list
		int index;
		switch( inKind)
		{
			case VFileSystemNotifier::kFileModified:	index = 0; break;
			case VFileSystemNotifier::kFileAdded:		index = 1; break;
			case VFileSystemNotifier::kFileDeleted:		index = 2; break;
			default:				xbox_assert( false );
		}
		
		std::string path( inPosixPath);
		path += inFileName;
		if (inIsDirectory)
			path += "/";
		VFilePath filePath( VString( path.c_str(), path.size(), VTC_UTF_8), FPS_POSIX );

		// protect access to VChangeData::fData
		{
			VTaskLock lock( &fOwner->fMutex);
			inData->fData[ index ].push_back( filePath );
		}
		
		return true;
	}
	return false;
}


bool XMacFileSystemNotification::BuildChangeList( XMacChangeData *inData, const std::string& inPosixPath)
{
	// We want to scan the directory we're watching, and compare it against the 
	// snapshot we've stored.  If a file exists in the scandir list, but not the
	// snapshot, then it's a newly added file.  If a file exists in the snapshot,
	// but not the scandir, then it's a file that's been deleted.  If the file 
	// exists in both snapshots, but the modification date has changed (by checking
	// with a call to lstat), then it's a file modification.
	//
	// Unfortunately, it is impossible for us to tell whether a file has been renamed,
	// so that operation will just look like a delete/add pair to the user.
	DirectoryListing *old_snapshot = NULL;
	DirectoryListing *new_snapshot = NULL;

	// Find the directory we care about in our old snapshot
	SnapshotListing::iterator directory_in_old_snapshot = inData->fSnapshot.find( inPosixPath);
	if (directory_in_old_snapshot != inData->fSnapshot.end())
		old_snapshot = directory_in_old_snapshot->second;

	if (!old_snapshot)
		return false;
	
	// Take an entirely new snapshot, just for the directory we care about
	SnapshotListing directory_for_new_snapshot;
	CreateDirectorySnapshot( inPosixPath, directory_for_new_snapshot, false );
	new_snapshot = directory_for_new_snapshot.find( inPosixPath)->second;
	if (!new_snapshot)
		return false;
		
	bool bFoundChanges = false;
		
	for (DirectoryListing::iterator file_from_new_snapshot = new_snapshot->begin(); file_from_new_snapshot != new_snapshot->end(); ++file_from_new_snapshot)
	{
		// We've been given a node of snapshot data from the new snapshot.  We want to see
		// if we can find this piece of information in the old snapshot's list now.  If we
		// can find it, then we need to check the modification date information as well.
		DirectoryListing::iterator file_from_old_snapshot = old_snapshot->find( file_from_new_snapshot->first );
		
		if (file_from_old_snapshot != old_snapshot->end())
		{
			if (!file_from_old_snapshot->second.isDirectory &&	// Directories cannot be modified, only added or deleted
				(file_from_old_snapshot->second.st_mtimespec.tv_sec != file_from_new_snapshot->second.st_mtimespec.tv_sec ||
				file_from_old_snapshot->second.st_mtimespec.tv_nsec != file_from_new_snapshot->second.st_mtimespec.tv_nsec))
			{
				// The modification times are different, so push this node onto the file change
				// stack so that we can process it later
				bFoundChanges |= AddChangeToList( inData, inPosixPath, file_from_new_snapshot->first, file_from_new_snapshot->second.isDirectory, VFileSystemNotifier::kFileModified);
			}
		}
		else
		{
			// We couldn't find the node in our old snapshot, so this means the file has been added to the directory.  So 
			// push the new node onto the file add stack so that we can process it later.
			bFoundChanges |= AddChangeToList( inData, inPosixPath, file_from_new_snapshot->first, file_from_new_snapshot->second.isDirectory, VFileSystemNotifier::kFileAdded);
		}
	}
	
	// We're not out of the woods quite yet.  We still need to determine if any files have been
	// deleted.  In this case, we need to loop over all of the old files and see if we can find
	// anything in the list of new files to match.  If we can't, then we have a delete operation.
	// But, we only need to do this if the user actually said they care about file deletions.
	if (inData->fFilters & VFileSystemNotifier::kFileDeleted)
	{
		for (DirectoryListing::iterator iter = old_snapshot->begin(); iter != old_snapshot->end(); ++iter)
		{
			DirectoryListing::iterator results = new_snapshot->find( iter->first );
			if (results == new_snapshot->end())
			{
				// We found a deletion to add to the list
				bFoundChanges |= AddChangeToList( inData, inPosixPath, iter->first, iter->second.isDirectory, VFileSystemNotifier::kFileDeleted);
			}
		}
	}

	// Destroy the data we snagged for the new snapshot -- we're going to regenerate the snapshot
	// entirely whenever there is a change.  It's ineffecient, but it also means we don't have to
	// track nearly as many edge cases in the code.
	ClearDirectorySnapshot( directory_for_new_snapshot );
	
	return bFoundChanges;
}


void XMacFileSystemNotification::FSEventCallback( ConstFSEventStreamRef inStreamRef, void *inCookie, size_t inNumEvents, void *inEventPaths, 
												const FSEventStreamEventFlags inFlags[], const FSEventStreamEventId inIds[] )
{
	XMacChangeData *data = (XMacChangeData*) inCookie;
	XMacFileSystemNotification *thisPointer = data->fThisPointer;
	
	// Something has happened, and now we get to figure out what that something is!  Whee!  The
	// first thing we need to do is figure out which directory the event has happened for so that
	// we can compare the directory snapshots.  We're given an array of raw C strings that point
	// to paths of changed directories.  We will test to see whether we've already processed any
	// changes for the given directory, and if we have, we won't bother to test again for this
	// notification.
	
    std::set<std::string> paths;
	for (size_t i = 0; i < inNumEvents; i++)
	{
		std::string path( ((const char **)inEventPaths)[ i ]);
		
		bool recursive;
		if (inFlags[i] & kFSEventStreamEventFlagHistoryDone)
		{
			continue;
		}
		else if (inFlags[i] & kFSEventStreamEventFlagRootChanged)
		{
			// inEventPaths is meaningless, we must scan root
			struct stat st;
			if (stat(data->fPosixPath.c_str(), &st) == 0)
			{
				// Root path now exists
				recursive = true;
				path = data->fPosixPath;
			}
			else
			{
				// Root path no longer exists
				//discard_all_dir_items();
				continue;
			}
		}
		else if (inFlags[i] & kFSEventStreamEventFlagMustScanSubDirs)
		{
			recursive = true;
			if ( (inFlags[i] & kFSEventStreamEventFlagUserDropped) || (inFlags[i] & kFSEventStreamEventFlagKernelDropped) )
			{
				// inEventPaths is meaningless, we must scan root
				path = data->fPosixPath;
			}
		}
		else
		{
			recursive = false;
		}
        
        paths.insert( path);
	}

    for( std::set<std::string>::iterator i = paths.begin() ; i != paths.end() ; ++i)
    {
        if (thisPointer->BuildChangeList( data, *i ))
        {
            thisPointer->fOwner->SignalChange( data );
        }
    }
    // This is a bit inefficient, but it also allows us to keep the logic somewhat simple.  Whenever
    // we get an event for a directory, we will clear out the old snapshot entirely and replace it with
    // a new snapshot.
    thisPointer->ClearDirectorySnapshot( data->fSnapshot );
    thisPointer->CreateDirectorySnapshot( data->fPosixPath, data->fSnapshot, true );
}

VError XMacFileSystemNotification::StartWatchingForChanges( const VFolder &inFolder, VFileSystemNotifier::EventKind inKindFilter, VFileSystemNotifier::IEventHandler *inHandler, sLONG inLatency )
{
	// We need to get the folder's path into an array for us to pass along to the OS call.
	VString posixPathString;
	inFolder.GetPath( posixPathString, FPS_POSIX);

	VStringConvertBuffer buffer( posixPathString, VTC_UTF_8);
	std::string posixPath( buffer.GetCPointer());

	CFStringRef pathRef = posixPathString.MAC_RetainCFStringCopy();
	CFArrayRef pathArray = CFArrayCreate( NULL, (const void **)&pathRef, 1, NULL );
	
	FSEventStreamContext context = { 0 };
	
	// The latency the user passed us is expressed in milliseconds, but the OS call requires the latency to be
	// expressed in seconds.
	CFTimeInterval latency = (inLatency / 1000.0);
	
	// Now we can make our change data object
	XMacChangeData *data = new XMacChangeData( inFolder.GetPath(), posixPath, inKindFilter, VTask::GetCurrent(), inHandler, this);
	context.info = data;
	data->fStreamRef = FSEventStreamCreate( NULL, &FSEventCallback, &context, pathArray, kFSEventStreamEventIdSinceNow, latency, kFSEventStreamCreateFlagNone );
	if (data->fStreamRef == NULL)
	{
		CFRelease( pathArray );
		CFRelease( pathRef );
		ReleaseRefCountable( &data);
		return MAKE_NATIVE_VERROR( errno );
	}
	
	// We also need to take an initial snapshot of the directory for us to compare again
	CreateDirectorySnapshot( posixPath, data->fSnapshot, true );
	
	// Now we can add the data object we just made to our list
	fOwner->PushChangeData( data);

	// Next, we can schedule this to run on the main event loop
	FSEventStreamScheduleWithRunLoop( data->fStreamRef, CFRunLoopGetMain(), kCFRunLoopDefaultMode );

	CFRelease( pathArray );
	CFRelease( pathRef );

	// Now that we've scheduled the stream to run on a helper thread, all that is left is to
	// start executing the stream
	VError err;
	if (FSEventStreamStart( data->fStreamRef ))
	{
		err = VE_OK;
	}
	else
	{
		err = MAKE_NATIVE_VERROR( errno );
	}
	ReleaseRefCountable( &data);
	
	return err;
}

VError XMacFileSystemNotification::StopWatchingForChanges( VFileSystemNotifier::VChangeData *inChangeData, bool inIsLastOne)
{	
	// First, we need to find the folder's path so that we can locate our entry
	// in the watch list

	XMacChangeData *data = dynamic_cast<XMacChangeData*>( inChangeData);

	// We are watching this, so we need to stop that
	FSEventStreamStop( data->fStreamRef );
	FSEventStreamInvalidate( data->fStreamRef );
	
	return VE_OK;
}

END_TOOLBOX_NAMESPACE


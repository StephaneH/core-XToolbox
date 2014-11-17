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
#ifndef __XMacFileSystemNotification__
#define __XMacFileSystemNotification__

#include "VSignal.h"
#include "VFileSystemNotification.h"

#include <sys/types.h>
#include <sys/dir.h>
#include <dirent.h>

BEGIN_TOOLBOX_NAMESPACE

class XMacFileSystemNotification : public VObject
{
public:
					XMacFileSystemNotification( VFileSystemNotifier *inOwner);
	virtual			~XMacFileSystemNotification();
	
			VError	StartWatchingForChanges( const VFolder &inFolder, VFileSystemNotifier::EventKind inKindFilter, VFileSystemNotifier::IEventHandler *inHandler, sLONG inLatency );
			VError	StopWatchingForChanges( VFileSystemNotifier::VChangeData *inChangeData, bool inIsLastOne);	
		
private:
	typedef struct FileInfo
	{
		struct timespec	st_mtimespec;
		bool	isDirectory;
	} FileInfo;

	typedef std::map< std::string, FileInfo>			DirectoryListing;
	typedef std::map< std::string, DirectoryListing *>	SnapshotListing;
	
	class XMacChangeData : public VFileSystemNotifier::VChangeData
	{
	public:
		typedef VFileSystemNotifier::VChangeData inherited;
	
		XMacChangeData( const VFilePath& inPath, const std::string& inPosixPath, VFileSystemNotifier::EventKind inFilter, VTask *inTargetTask, VFileSystemNotifier::IEventHandler *inCallBack, VFileSystemNotifier::XFileSystemNotification *inImplementation);
		
		// A snapshot of the file hierarchy at the given path.  We need to keep
		// this information around because OS X doesn't tell us *what* changed, only
		// that something has changed.  So we need to determine the file and change
		// type ourselves, which requires us to keep a snapshot.  We are mapping 
		// directory path hashes to FileSystemObject objects for efficiency.
		SnapshotListing		fSnapshot;

		// This is the file system stream event object that we created when making the
		// change data object.
		FSEventStreamRef	fStreamRef;
		
		// directory to watch in posix representation for conveniency
		std::string			fPosixPath;
	
	private:
		~XMacChangeData();
		XMacChangeData( const XMacChangeData&); // forbidden
		XMacChangeData& operator=( const XMacChangeData&);	// forbidden
	};
	
	static	void	CreateDirectorySnapshot( const std::string& inPosixPath, SnapshotListing &outList, bool inRecursive );
	static	void	ClearDirectorySnapshot( SnapshotListing &ioList );
			bool	BuildChangeList( XMacChangeData *inData, const std::string& inPosixPath);
			bool	AddChangeToList( XMacChangeData *inData, const std::string& inPosixPath, const std::string& inFileName, bool inIsDirectory, VFileSystemNotifier::EventKind inKind);
		
	static	void	FSEventCallback( ConstFSEventStreamRef inStreamRef, void *inCookie, size_t inNumEvents, void *inEventPaths, const FSEventStreamEventFlags inFlags[], const FSEventStreamEventId inIds[] );

			VFileSystemNotifier*	fOwner;
};


END_TOOLBOX_NAMESPACE

#endif
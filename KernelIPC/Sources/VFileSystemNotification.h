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
#ifndef __VFileSystemNotifier__
#define __VFileSystemNotifier__


BEGIN_TOOLBOX_NAMESPACE



// The VFileSystemNotifier is a singleton class that allows you to register a callback for notifications about
// file system changes.  You start watching for changes within a particular folder, and will receive a callback whenever
// a change you're watching for occurs.  When you're done watching for changes, you can unregister your callback.  Note
// that this function always watches directory *trees*, which include subdirectories.
class XTOOLBOX_API VFileSystemNotifier : public VObject
{
public:
#if VERSIONMAC
	typedef class XMacFileSystemNotification XFileSystemNotification;
	friend class XMacFileSystemNotification;
#elif VERSIONWIN
	typedef class XWinFileSystemNotification XFileSystemNotification;
	friend class XWinFileSystemNotification;
#elif VERSION_LINUX
	typedef class XLinuxFileSystemNotifier XFileSystemNotification;
	friend class XLinuxFileSystemNotifier;
#endif

	// Returns a singleton instance shared for the whole application.
	// VProcessIPC holds the singleton.
	static VFileSystemNotifier *Instance();
	
	// You should use VProcessIPC::Get()->GetFileSystemNotifier() instead of creating your own notifier.
	VFileSystemNotifier();
	virtual ~VFileSystemNotifier();

	// Note that file rename notifications are possible, but they are passed to you as an add and delete operation.
	// So if you request information about added files, but not delete operations (or vice versa), you will get some
	// information about file renames, but not all information.
	typedef enum EventKind
	{
		kNone = 0,
		kFileModified = 1 << 0,
		kFileAdded = 1 << 1,
		kFileDeleted = 1 << 2,
		kAll = kFileModified | kFileAdded | kFileDeleted,
	} EventKind;

	class XTOOLBOX_API IEventHandler
	{
	public:
		// The notifier may batch together multiple file change notifications into a list instead of 
		// calling the handler multiple times with one file only.  These batches of files will all be
		// of the same kind.  For instance, let's say you have a batch of changes where there are three
		// new files, two deleted files and one renamed file.  You will get three calls to your handler,
		// one for each event kind.  Each of those calls will contain a vector of paths for all of the
		// files affected by that operation.
		
		// May receive notifications for file or folders.
		// if a folder is deleted, you'll receive only one notification even if the folder contained multiple files.
		virtual void FileSystemEventHandler( const std::vector< VFilePath > &inFilePaths, EventKind inKind ) = 0;
	};

	// The inLatency parameter allows you to coalesce notifications for efficiency reasons.  If you pass in zero
	// for the latency, then you will receive a callback as the notifications arrive from the OS.  However, if you
	// pass in a non-zero value, then we will queue notifications from the OS until the latency time period expires.
	// For instance, if you pass a latency of 15 and two changes happen to the directory being observed (one at time
	// X, and another at time X + 10), you will get a notification at time X + 25 instead of two separate notices as
	// the events happened.  This can greatly reduce the amount of method call overhead involved when large batches
	// of changes happen and is the recommended approach for applications which interact directly with the user.
	// The latency is expressed in milliseconds, but may be rounded to the nearest unit of time depending on the
	// platform.  So it should be treated as an approximation instead of hard time.
	VError StartWatchingForChanges( const VFolder &inFolder, EventKind inKindFilter, IEventHandler *inHandler, sLONG inLatencyMilliseconds );
	VError StopWatchingForChanges( const VFolder &inFolder, IEventHandler *inHandler );

private:
	/*
		private class to hold file change information
	*/
	class VChangeData : public VObject, public IRefCountable
	{
	public:
				VChangeData( const VFilePath& inPath, EventKind inFilter, VTask *inTargetTask, IEventHandler *inCallBack, XFileSystemNotification *inImplementation);
		virtual	~VChangeData();
		
		// The path to the directory we are watching
		VFilePath		fPath;

		// The filter the user wants us to apply when checking for directory modifications.
		// Note that this filter needs to be translated when calling ReadDirectoryChangesW.
		EventKind		fFilters;

		// The callback to fire when we want to issue some results.  Note, we should 
		// *NEVER* call this directly without protecting it with our critical section.  Failing
		// to do so can lead to a race condition between firing the event after the latency
		// period has expired, during which time the project is closed down and isn't able to
		// receive callbacks.
		IEventHandler*	fCallback;
		VTask*			fTargetTask;

		// A list of file paths that have some sort of changes.  Each index into the array
		// is for a different type of change mask.  The indexes are as follows:
		//		0 -- modified files
		//		1 -- added files
		//		2 -- deleted files
		// Each vector holds a list of files for the given change type.  Note that the
		// index corresponds directly to the EventKind enumeration in DataIndexToEventKind
		std::vector< VFilePath >	fData[ 3 ];

		// The "this" pointer, which we need to track since the APC runs in a static function
		XFileSystemNotification *fThisPointer;
	
	private:
		VChangeData( const VChangeData&);	// forbidden
		VChangeData& operator=( const VChangeData&);	// forbidden
	};


	/*
		private class for messaging
	*/
	class VChangeMessage : public VMessage
	{
	public:
		VChangeMessage( VFileSystemNotifier *inMgr, VChangeData *inData) : fMgr( inMgr), fChangeData( inData) {}

	protected:
		VChangeMessage( const VChangeMessage&);
		VChangeMessage& operator=( const VChangeMessage&);
		virtual void	DoExecute ()
		{
			fMgr->EventSignaledCallback( fChangeData);
		}
		
		VFileSystemNotifier*	fMgr;
		VRefPtr<VChangeData>		fChangeData;
	};

	VFileSystemNotifier( const VFileSystemNotifier&);	// forbidden
	VFileSystemNotifier& operator=( const VFileSystemNotifier&);	// forbidden

	static	EventKind								DataIndexToEventKind( int inIndex )				{ return (EventKind)(1 << inIndex); }
			void									PushChangeData( VChangeData *inChangeData)		{ fWatchList.push_back( inChangeData);}
			void									SignalChange( VChangeData *inChangeData);
			void									EventSignaledCallback( VChangeData *inData);

			XFileSystemNotification*				fImpl;
			VCriticalSection						fMutex;

	typedef std::vector<VRefPtr<VChangeData> >		VectorOfChangeData;
			VectorOfChangeData						fWatchList;
};

END_TOOLBOX_NAMESPACE

#endif

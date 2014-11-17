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
#ifndef __XWinFileSystemNotification__
#define __XWinFileSystemNotification__

#include "VFileSystemNotification.h"
#include "VSignal.h"
 
BEGIN_TOOLBOX_NAMESPACE

// The Windows implementation is a bit different in that it performs the watches on a background thread.  It
// does this so that the thread can be put to sleep while no events are happening, and wake on demand.  However,
// it also means that we have to do a bit of work since the ReadDirectoryChangesW API assumes the completion routine
// is executing on the current thread.  To this end, the the calls to that API are queued via APC calls, so they can
// be executed within the context of the proper thread.  This allows us to have a highly efficient implementation with
// minimal footprint, at the expense of code complexity.
class XWinFileSystemNotification : public VObject
{
public:
	XWinFileSystemNotification( VFileSystemNotifier *inOwner);
	virtual ~XWinFileSystemNotification();

	VError StartWatchingForChanges( const VFolder &inFolder, VFileSystemNotifier::EventKind inKindFilter, VFileSystemNotifier::IEventHandler *inHandler, sLONG inLatency );
	VError StopWatchingForChanges( VFileSystemNotifier::VChangeData *inChangeData, bool inIsLastOne);

protected:
	class XWinChangeData : public VFileSystemNotifier::VChangeData
	{
	public:
		typedef VFileSystemNotifier::VChangeData inherited;
	
		XWinChangeData( const VFilePath& inPath, VFileSystemNotifier::EventKind inFilter, VTask *inTargetTask, VFileSystemNotifier::IEventHandler *inCallBack, VFileSystemNotifier::XFileSystemNotification *inImplementation);

		bool	ReadDirectoryChanges();

		// This is the overlapped io structure that we are using for our APC, however,
		// there are a few caveats.  It *must* be the first field in this structure, and
		// it must never be a pointer to the OVERLAPPED structure.
		OVERLAPPED fOverlapped;

		// The OS handle to the directory that we've opened.  Will be a valid handle
		HANDLE fFolderHandle;

		// The buffer for us to put event data itself
		char fBuffer[ 1024 * 32 ];

		// The amount of time the user wishes us to wait until we decide to fire the
		// callback when changes occur.  This allows us to buffer changes into a batch.
		// Expressed in milliseconds.
		sLONG fLatency;

		// This is a handle to a waitable timer that we may need to use in order to handle
		// the latency for a notification.  When a file system event happens, we will create
		// a timer to fire when the latency period has expired.
		HANDLE fTimer;

		bool	fIsCanceled;

	private:
		~XWinChangeData();
		XWinChangeData( const XWinChangeData&); // forbidden
		XWinChangeData& operator=( const XWinChangeData&);	// forbidden
	};

	DWORD	MakeFilter( VFileSystemNotifier::EventKind inFilters );
	void	AddResultToProperList( XWinChangeData *inData, const VString& inFileName, int inAction );

	void	WatchForChanges( XWinChangeData *inData );
	void	PrepareDeleteChangeData(XWinChangeData *inData );
	void	LaunchTaskIfNecessary();

private:
	static sLONG TaskRunner( VTask *inTask );
	static void CALLBACK CompletionRoutine( DWORD inErrorCode, DWORD inBytesTransferred, LPOVERLAPPED inOverlapped );
	static void CALLBACK TimerProc( LPVOID inArg, DWORD, DWORD );

			void						RunTask();

			VTask*										fTask;
			VFileSystemNotifier*						fOwner;
			std::vector<VRefPtr<XWinChangeData> >		fActiveChanges;
			std::vector<VRefPtr<XWinChangeData> >		fCanceledChanges;
			std::vector<VRefPtr<XWinChangeData> >		fPendingChanges;
};

END_TOOLBOX_NAMESPACE

#endif
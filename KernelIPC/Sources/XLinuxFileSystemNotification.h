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
#ifndef __XLinuxFileSystemNotification__
#define __XLinuxFileSystemNotification__


#include "VFileSystemNotification.h"

#include "VSignal.h"
#include "Kernel/Sources/VTask.h"

#include <map>
#include <set>
#include <string>



BEGIN_TOOLBOX_NAMESPACE


typedef VFileSystemNotifier			VFSN;
typedef XLinuxFileSystemNotifier	XFSN;


class XLinuxFileSystemNotifier : public VObject
{
public:

	typedef VFSN::VChangeData	VChangeData;
	typedef VFSN::EventKind		EventKind;
	typedef VFSN::IEventHandler	IEventHandler;
	
	XLinuxFileSystemNotifier(VFSN* inOwner) : fOwner(inOwner), fId(-1), fMsTimeout(100), fWatchTask(NULL) { xbox_assert(fOwner!=NULL); }
	virtual	~XLinuxFileSystemNotifier();
	
	VError  Init();
	VError	StartWatchingForChanges(const VFolder &inFolder, EventKind inFilter, IEventHandler *inHandler, sLONG inLatency);
	VError	StopWatchingForChanges(VChangeData *inChangeData, bool inIsLastOne);	
	

private:

    class XLinuxChangeData;
	
    class WatchedFolder
	{
    public :

		WatchedFolder(XLinuxChangeData* inOwner, const VFilePath& inFolderPath, EventKind inFilter) : 
            fId(-1), fWd(-1), fChanged(false), fFolderPath(inFolderPath), fFilter(inFilter), fOwner(inOwner) {};
		
		VError InotifyRegister(int inId);
		VError InotifyUnregister();
		VError SignalChange(VFSN* inVfsn);
		
		bool operator==(const WatchedFolder& inWF) const { return fWd==inWF.fWd; }
		bool operator!=(const WatchedFolder& inWF) const { return fWd!=inWF.fWd; }
		bool operator<(const WatchedFolder& inWF)  const { return fWd<inWF.fWd; }

		
        int                 fId;
		int					fWd;
		bool				fChanged;
		VFilePath			fFolderPath;
		EventKind			fFilter;
		XLinuxChangeData*	fOwner;

		typedef std::map<std::string, EventKind>    EventMap;
		typedef EventMap::iterator                  EventIterator;
		
		EventMap			fEvents;		
	};
	
	
	class XLinuxChangeData : public VChangeData
	{
	public:
		
		XLinuxChangeData(const VFilePath& inPath, EventKind inFilter, VTask* inTargetTask, IEventHandler* inCallBack, XLinuxFileSystemNotifier* inNotifyImpl) :
			VChangeData(inPath, inFilter, inTargetTask, inCallBack, inNotifyImpl), fWatchedFolder(this, inPath, inFilter) {/*fBreakTag=1, fTag=1;*/}
		
		WatchedFolder* GetWatchedFolder() { return &fWatchedFolder; }
		
	private:

		virtual ~XLinuxChangeData() {};							//ref countable
		XLinuxChangeData(const XLinuxChangeData&);				//forbidden
		XLinuxChangeData& operator=(const XLinuxChangeData&);	//forbidden

        WatchedFolder fWatchedFolder;
	};


	typedef std::map<int, WatchedFolder*>	FolderMap;
	typedef FolderMap::iterator          FolderIterator;
    
	static sLONG LaunchWatchTask(VTask *inTask);
	VError WatchAndNotify();
	
	VError AddToFolderMap(XLinuxChangeData* inData);
	VError RemoveFromFolderMap(XLinuxChangeData* inData);	
	// VError LockFolders();
	// VError UnlockFolders();

	VFileSystemNotifier*	fOwner;
    int                     fId;
    sLONG                   fMsTimeout;
	VTask*					fWatchTask;
	// VSystemCriticalSection	fFolderMapLock;
	FolderMap				fFolderMap;
};



END_TOOLBOX_NAMESPACE

#endif

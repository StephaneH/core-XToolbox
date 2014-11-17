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
#include "VKernelPrecompiled.h"
#include "VTask.h"
#include "VProcess.h"
#include "VString.h"
#include "VSyncObject.h"
#include "VError.h"
#include "XWinCrashDump.h"

#include <process.h>
#include <imagehlp.h>

#define	WMX_TOOLBOX_FIRST	WM_USER
#define	WMX_MESSAGE WMX_TOOLBOX_FIRST	// tell the thread that a XToolBox message is waiting for it



typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;	// must be 0x1000
	LPCSTR szName;	// pointer to name (in user addr space)
	DWORD dwThreadID;	// thread ID (-1=caller thread)
	DWORD dwFlags;	// reserved for future use, must be zero
} THREADNAME_INFO;


static void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
//	if (::IsDebuggerPresent())
	{
		THREADNAME_INFO	info;
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try
		{
			//Correction du C2264 modification des types
			RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
		}
		__except(EXCEPTION_CONTINUE_EXECUTION)
		{
		}
	}
}


//================================================================================================================


_FiberData::_FiberData( VTask *inTask):fTask( inTask), fTaskID( inTask ? inTask->GetID() : NULL_TASK_ID)
{
}


void _FiberData::SetTask( VTask *inTask)
{
	fTask = inTask;
	fTaskID = inTask->GetID();
}

//================================================================================================================


XWinTask::XWinTask( VTask *inOwner, bool inMainTask)
: fOwner( inOwner)
, fStackAddress( NULL)
, fStackSize( 0)
, fFiber( NULL)
, fFiberGroupOwner( NULL)
, fMainTask( inMainTask)
{
}


XWinTask::~XWinTask()
{
	// preemptive threads delete their fiber themselves on terminating
	if ( (fFiber != NULL) && (fFiberGroupOwner != fOwner))
		::DeleteFiber( fFiber);
}

/*
	static
*/
XWinTaskMgr *XWinTask::GetManager()
{
	return VTaskMgr::Get()->GetImpl();
}


size_t XWinTask::GetCurrentFreeStackSize()
{
	char c;
	return fStackSize - (fStackAddress - &c);
}


VTaskID XWinTask::GetValueForVTaskID() const
{
	return NULL_TASK_ID;
}


void XWinTask::_Run()
{
	char c;
	fStackAddress = &c;
	
	// on vista, the random seed is per fiber
	srand( time( NULL) );

	GetManager()->SetCurrentTask( GetOwner());

#if !VERSIONDEBUG
	__try
	{
#endif

	fOwner->_Run();

#if !VERSIONDEBUG
	}
	__except(SaveMiniDump(GetExceptionInformation()))
	{
	}
#endif
	
	VTask *destinationTask = (fOwner == fFiberGroupOwner) ? NULL : fFiberGroupOwner;
	Exit( destinationTask);
}


bool XWinTask::GetCPUUsage(Real& outCPUUsage) const
{
	return false;
}


bool XWinTask::GetCPUTimes(Real& outSystemTime, Real& outUserTime) const
{
	return false;
}

//================================================================================================================

XWinTask_preemptive::XWinTask_preemptive( VTask *inOwner, bool /* for main task */ )
: XWinTask( inOwner, true)
, fThread( ::GetCurrentThread())
, fSystemID( ::GetCurrentThreadId())
{
	if (GetManager()->WithFibers())
	{
		// main task has already been converted into a fiber by the XTaskMgr,
		// we just have to set the VTask now we know it.
		SetFiber( ::GetCurrentFiber(), inOwner);
		
		_FiberData::Current()->SetTask( inOwner);
	}

	SetThreadName( -1, "Main");
}


XWinTask_preemptive::XWinTask_preemptive( VTask *inOwner)
: XWinTask( inOwner, false)
, fThread( NULL)
, fSystemID( 0)
{
}


XWinTask_preemptive::~XWinTask_preemptive()
{
	if ( (fThread != NULL) && !IsMainTask())
	{
		::CloseHandle( fThread);
	}
}


VTaskID XWinTask_preemptive::GetValueForVTaskID() const
{
	if (testAssert( (VTaskID) fSystemID == fSystemID))
		return (VTaskID) fSystemID;
	
	return NULL_TASK_ID;
}


void XWinTask_preemptive::Exit( VTask *inDestinationTask)
{
	if (GetManager()->WithFibers())
		delete _FiberData::Current();

	// deinit COM (inited in XWinTask_preemptive::Init)
	::CoUninitialize();

	// now this can be deleted at anytime
	GetOwner()->_SetStateDead();
	
	// let the std library deinitialize itself
	::_endthreadex(0); // null return code
}


UINT APIENTRY XWinTask_preemptive::_ThreadProc(void* inParam)
{
	XWinTask_preemptive *task = static_cast<XWinTask_preemptive*>( inParam);
	VTask *owner = task->GetOwner();
	
	if (task->GetManager()->WithFibers())
	{
		LPVOID fiber = ::ConvertThreadToFiber( new _FiberData( owner));
		
		task->SetFiber( fiber, owner);
	}

	task->GetManager()->SetCurrentTask( owner);

	// initialize here the model for COM for the thread.
	// must be done for each thread (the mode cannot be done on the fly, that's why we fix it now for all)
	// COINIT_MULTITHREADED is the fastest but maybe problematic with OLE objects
	// (see also XWinApplication::Init for the main thread)
	// HRESULT r = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	HRESULT r = ::OleInitialize(NULL);	// J.A. : COINIT_MULTITHREADED doesn't work with OLE objects

	task->_Run();
	
	return 0; // will never get there
}


bool XWinTask_preemptive::CheckSystemMessages()
{
	MSG msg;
	bool gotAnEvent = false;
	
	BOOL test = ::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);
	if (test == 0)// no messsage, wait 1 ms
	{
		::MsgWaitForMultipleObjects( 0, 0, false, 1, QS_ALLEVENTS);
	}
	else if (msg.message == WM_QUIT)
	{
		VProcess::Get()->QuitAsynchronously();
		gotAnEvent = true; //
	}
	else
	{
		// pp test purge thread event queue before returning
		while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				VProcess::Get()->QuitAsynchronously();
				break;
			}
			else
			{
				::TranslateMessage( &msg);
				::DispatchMessage( &msg);
			}
		}
		gotAnEvent = true;
	}
	
	return gotAnEvent;
}


bool XWinTask_preemptive::_CreateThread()
{
	xbox_assert( fThread == NULL);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = 0; 
	sa.bInheritHandle = true;

	// L.E. 30/04/1999 use standard library call to let it initialize itself properly
	// create the thread in a suspended state to be sure fThread and fSystemID are properly set before it runs
	UINT threadID;
	size_t stackSize = GetOwner()->GetStackSize();
	SetStackSize( stackSize);
	fThread = (HANDLE)::_beginthreadex(&sa, (UINT) stackSize, &_ThreadProc, this, CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION, &threadID);

	if (testAssert( fThread != NULL))
	{
		fSystemID = threadID;
	}
	else
	{
		vThrowPosixError( errno);
	}
	
	return fThread != NULL;
}


/*
	static
*/
XWinTask_preemptive *XWinTask_preemptive::Create( VTask* inOwner)
{
	XWinTask_preemptive *task = new XWinTask_preemptive( inOwner);
	if (!task->_CreateThread())
	{
		delete task;
		task = NULL;
	}
	return task;
}


bool XWinTask_preemptive::Run()
{
	if (testAssert( fThread != NULL))
	{
		// let it go
		::ResumeThread( fThread);
	}
	return fThread != NULL;
}


void XWinTask_preemptive::Sleep(sLONG inMilliSeconds)
{
	::Sleep( inMilliSeconds);	
}


void XWinTask_preemptive::WakeUp()
{
	::ResumeThread( fThread);
}


void XWinTask_preemptive::Freeze()
{
	::SuspendThread( fThread);
}


bool XWinTask_preemptive::GetCPUUsage(Real& outCPUUsage) const
{
	return false;
}


bool XWinTask_preemptive::GetCPUTimes(Real& outSystemTime, Real& outUserTime) const
{
	bool valuesFound = false;

	FILETIME creationTime, exitTime, kernelTime, userTime;
	if(GetThreadTimes(fThread, &creationTime, &exitTime, &kernelTime, &userTime))
	{

		ULARGE_INTEGER kernelTimeInteger, userTimeInteger;
		kernelTimeInteger.LowPart = kernelTime.dwLowDateTime; 
		kernelTimeInteger.HighPart = kernelTime.dwHighDateTime; 

		userTimeInteger.LowPart = userTime.dwLowDateTime; 
		userTimeInteger.HighPart = userTime.dwHighDateTime; 

		outSystemTime = (Real)(signed __int64)kernelTimeInteger.QuadPart / 1e7; //100ns -> s
		outUserTime = (Real)(signed __int64)userTimeInteger.QuadPart / 1e7;

		valuesFound = true;
	}

	return valuesFound;
}


//================================================================================================================

XWinTask_fiber::XWinTask_fiber( VTask *inOwner):XWinTask( inOwner, false),fSleepStamp( 0)
{
}


XWinTask_fiber::~XWinTask_fiber()
{
}


void XWinTask_fiber::Exit( VTask *inDestinationTask)
{
	delete _FiberData::Current();

	// now this can be deleted at anytime
	GetOwner()->_SetStateDead();

	inDestinationTask->GetImpl()->SwitchToFiber();
}


bool XWinTask_fiber::Run()
{
	// the new fiber will belong to the calling thread.
	
	size_t stackSize = GetOwner()->GetStackSize();
	SetStackSize( stackSize);

	/*commit size: 0 = default */
	// FIBER_FLAG_FLOAT_SWITCH n'est utilisable que sur Windows Server 2003!
	LPVOID fiber = ::CreateFiberEx( 48*1024L, stackSize, 0 /*FIBER_FLAG_FLOAT_SWITCH*/, _ThreadProc, new _FiberData( GetOwner()));

	if (testAssert( fiber != NULL))
	{
		VTask *currentTask = GetManager()->GetCurrentTask();
		VTask *fiberGroupOwner = currentTask->GetImpl()->GetFiberGroupOwner();
		if (fiberGroupOwner == NULL)
			fiberGroupOwner = currentTask;

		SetFiber( fiber, fiberGroupOwner);

		SwitchToFiber();

		GetManager()->SetCurrentTask( currentTask);
	}
	return fiber != NULL;
}


VOID CALLBACK XWinTask_fiber::_ThreadProc( PVOID inParam)
{
	_FiberData *data = static_cast<_FiberData*>( inParam);
	VTask *owner = data->GetTask();

	owner->GetImpl()->_Run();
}


void XWinTask_fiber::Sleep( sLONG inMilliSeconds)
{
	VTaskMgr::Get()->CurrentFiberWaitFlagWithTimeout( &fSleepStamp, fSleepStamp, inMilliSeconds);
}


void XWinTask_fiber::WakeUp()
{
	++fSleepStamp;
	VTaskMgr::Get()->YieldNow();
}


void XWinTask_fiber::Freeze()
{
	VTaskMgr::Get()->CurrentFiberWaitFlag( &fSleepStamp, fSleepStamp);
}


bool XWinTask_fiber::CheckSystemMessages()
{
	return false;
}


//================================================================================================

XWinTaskMgr::XWinTaskMgr()
{
	fSlot_CurrentTask = ::TlsAlloc();
	fSlot_CurrentTaskID = ::TlsAlloc();
}


XWinTaskMgr::~XWinTaskMgr()
{
	
}


bool XWinTaskMgr::Init( VProcess *inProcess)
{
	fWithFibers = inProcess->IsFibered();
	if (fWithFibers)
	{
		// on convertit tout de suite la main thread, la VTask sera fournie plus tard
		::ConvertThreadToFiber( new _FiberData( NULL));
	}

	// allocate a slot to store the this pointer
	bool ok = (fSlot_CurrentTask != 0xffffffff) && (fSlot_CurrentTaskID != 0xffffffff);
	xbox_assert( ok);

	return ok;
}

void XWinTaskMgr::Deinit( )
{
	if (WithFibers())
		delete _FiberData::Current();	// main thread fiber data

	::TlsFree( fSlot_CurrentTask);
	::TlsFree( fSlot_CurrentTaskID);
}

XWinTask *XWinTaskMgr::Create( VTask *inOwner, ETaskStyle inStyle)
{
	XWinTask* winTask;
	
	switch( inStyle)
	{
		case eTaskStyleFiber:
			{
				winTask = fWithFibers ? new XWinTask_fiber( inOwner) : NULL;
				break;
			}

		case eTaskStyleCooperative:
			xbox_assert( false);
			winTask = NULL;
			break;

		case eTaskStylePreemptive:
			{
				winTask = XWinTask_preemptive::Create( inOwner);
				break;
			}
		
		default:
			winTask = NULL;
	}
	return winTask;
}


XWinTask *XWinTaskMgr::CreateMain( VTask *inOwner)
{
	XWinTask_preemptive *task = new XWinTask_preemptive( inOwner, true);

	SetCurrentTask( inOwner);

	return task;
}


void XWinTaskMgr::TaskStarted( VTask *inTask)
{
	if (!inTask->IsMain())
	{
		xbox_assert( VTask::GetMain() != NULL );
		::AttachThreadInput( inTask->GetImpl()->GetSystemID(), VTask::GetMain()->GetImpl()->GetSystemID(), true);
	}
}


void XWinTaskMgr::TaskStopped(VTask* inTask)
{
}


size_t XWinTaskMgr::ComputeMainTaskStackSize() const
{
	size_t stackSize = 0; // means can't find it
	
	// retrieve stack space
	SECURITY_ATTRIBUTES sa;
	char				filename[4096];
	CHAR				pathPart[4096];
	CHAR				*namePart;
	LOADED_IMAGE		imageInfo;

	long error;

	::GetModuleFileNameA(NULL,filename,sizeof(filename));

	if (GetFullPathName(filename,sizeof(pathPart),pathPart,&namePart) > 0)
	{
		namePart[-1] = '\0';
		imageInfo.SizeOfImage = sizeof(LOADED_IMAGE);
		if (MapAndLoad(namePart,pathPart,&imageInfo,FALSE,TRUE))
		{
			stackSize = imageInfo.FileHeader->OptionalHeader.SizeOfStackReserve;
			UnMapAndLoad(&imageInfo);
		}
	}

	return stackSize;
}


size_t XWinTaskMgr::GetCurrentFreeStackSize() const
{
	VTask *task = GetCurrentTask();
	if (task == NULL)
		return GetCurrentFreeStackSizeStatic();
	return task->GetImpl()->GetCurrentFreeStackSize();
}


size_t XWinTaskMgr::GetCurrentFreeStackSizeStatic()
{
	return 1024*1024L;
}


void XWinTaskMgr::YieldToFiber( VTask *inPreferredTask)
{
	// on ne peut switcher qu'entre fibres du meme groupe
	VTask *currentTask = GetCurrentTask();
	if (GetCurrentTask()->GetImpl()->GetFiberGroupOwner() == inPreferredTask->GetImpl()->GetFiberGroupOwner())
	{
		inPreferredTask->GetImpl()->SwitchToFiber();
		SetCurrentTask( currentTask);
	}
}

bool XWinTaskMgr::IsFibersThreadingModel()
{
	return true;
}


void XWinTaskMgr::SetCurrentThreadName( const VString& inName, VTaskID inTaskID) const
{
	// don't use system converters that use a fiber lock which is forbidden here
	VStringConvertBuffer buffer( inName, VTC_UTF_8 /*VTC_Win32Ansi*/);
	SetThreadName( -1, buffer.GetCPointer());
}




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
#include "IIdleable.h"
#include "VTask.h"



// ---------------------------------------------------------------------------
//	IIdleable::~IIdleable
// ---------------------------------------------------------------------------
// Default destructor
//
IIdleable::~IIdleable()
{
	StopIdling();
}


// ---------------------------------------------------------------------------
//	IIdleable::StartIdling
// ---------------------------------------------------------------------------
// Call to start being called on idle time.
// If it's allready idling, does nothing.
//
void IIdleable::StartIdling( VTask *inTask)
{
	assert( inTask != NULL);
	
	fIdling = true;

	if (fRegisteredTask != inTask)
	{
		if (fRegisteredTask)
			fRegisteredTask->UnregisterIdler( this);
		CopyRefCountable( &fRegisteredTask, inTask);
		inTask->RegisterIdler( this);
	}
}


void IIdleable::StartIdling()
{
	StartIdling( VTask::GetCurrent());
}


// ---------------------------------------------------------------------------
//	IIdleable::StopIdling
// ---------------------------------------------------------------------------
// Call whenever you no longer want or need to be called on idle time.
// If it's not idling, does nothing.
//
void IIdleable::StopIdling()
{
	if (fRegisteredTask)
	{
		fIdling = false;

		// Unregister before to be sure that no idle is received during DoStopIdle
		fRegisteredTask->UnregisterIdler( this);

		ReleaseRefCountable( &fRegisteredTask);
	}
}

// ---------------------------------------------------------------------------
//	IIdleable::Idle
// ---------------------------------------------------------------------------
// Calls DoIdle if idling
//
void IIdleable::Idle()
{
	if (fIdling)
	{
		assert( VTask::GetCurrent() == fRegisteredTask);
		DoIdle();
	}
}


// ---------------------------------------------------------------------------
//	IIdleable::DoIdle
// ---------------------------------------------------------------------------
// Override to be called at idle time.
//
void IIdleable::DoIdle()
{
}



#pragma mark-

// ---------------------------------------------------------------------------
//	XIdleMessage::XIdleMessage
// ---------------------------------------------------------------------------
// 
//
XIdleMessage::XIdleMessage(IIdleable* inIdleable)
{
	fIdleable = inIdleable;
}


// ---------------------------------------------------------------------------
//	XIdleMessage::~XIdleMessage
// ---------------------------------------------------------------------------
// 
//
XIdleMessage::~XIdleMessage()
{
}


// ---------------------------------------------------------------------------
//	XIdleMessage::DoExecute
// ---------------------------------------------------------------------------
// 
//
void XIdleMessage::DoExecute()
{
	if (fIdleable->IsIdling())
	{
		fIdleable->DoIdle();
		
		XIdleMessage*	message = new XIdleMessage(fIdleable);
		if (message != NULL)
		{
			message->PostTo( VTask::GetCurrent(), false, 16); // each 1/60 second
			message->Release();
		}
	}
}

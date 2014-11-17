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
#include "VSmallCriticalSection.h"
#include "VTask.h"
#include "VInterlocked.h"
#include "VSyncObject.h"


// Class statics
sLONG	VSmallCriticalSection::sUnlockCount = 0;
sLONG	VNonVirtualCriticalSection::sUnlockCount = 0;


// Class definitions
typedef struct {
	sWORD first;
	sWORD second;
} LongAsDoubleWord;


VNonVirtualCriticalSection::VNonVirtualCriticalSection()
{
	fUseCount = 0;
	fOwner = NULL_TASK_ID;
	fEvent = NULL;
}


Boolean VNonVirtualCriticalSection::TryToLock()
{
	VTaskID	currentTaskID = VTask::GetCurrentID();
	VTaskID	owner = (VTaskID) VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID, (sLONG) currentTaskID);

	if (owner == NULL_TASK_ID)
	{
		assert(fUseCount == 0);
		fUseCount = 1;
	}
	else if (owner == currentTaskID)
	{
		++fUseCount;
	}

	return (fOwner == currentTaskID);
}


Boolean VNonVirtualCriticalSection::Lock()
{

	VTaskID currentTaskID = VTask::GetCurrentID();
	do {

		VTaskID owner = (VTaskID) VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID, (sLONG) currentTaskID);
		if (owner == NULL_TASK_ID)
		{
			assert(fUseCount == 0);
			fUseCount = 1;
		}
		else if (fOwner == currentTaskID)
		{
			++fUseCount;
		}
		else
		{
			sLONG	unlockCount = sUnlockCount;

			VSyncEvent*	event = VInterlocked::ExchangePtr( &fEvent);
			if (event != NULL)
			{
				event->Retain();
				if (VInterlocked::CompareExchangePtr((void **)&fEvent, NULL, event) != NULL)
				{
					event->Unlock();
					event->Release();
					VTask::YieldNow();
				}
				else
				{
					while (VInterlocked::CompareExchange(&sUnlockCount, sUnlockCount, sUnlockCount) != sUnlockCount)
						VTask::YieldNow();

					if (sUnlockCount == unlockCount)
						event->Lock( 100);
					else
						VTask::YieldNow();
				}
				event->Release();
			}
			else
			{
				event = new VSyncEvent;

				if (VInterlocked::CompareExchangePtr((void **)&fEvent, NULL, event) != NULL)
				{
					event->Unlock();
					event->Release();
					VTask::YieldNow();
				}
			}
		}

	} while(fOwner != currentTaskID);
	
	return true;
}


Boolean VNonVirtualCriticalSection::Unlock()
{
	assert((fOwner == VTask::GetCurrentID()) && (fUseCount > 0) && (fUseCount < 32000L));

	//assert(fUseCount > 0);
	VSyncEvent*	event = NULL;

	VInterlocked::Increment(&sUnlockCount);

	if (--fUseCount == 0)
	{
		event = VInterlocked::ExchangePtr(&fEvent);
		VTaskID	owner = VInterlocked::Exchange((sLONG*) &fOwner, (sLONG) NULL_TASK_ID);
		//assert(owner == VTask::GetCurrentID());

		if (event != NULL)
		{
			event->Unlock();
			event->Release();
		}
	}
	
	return true;
}


VNonVirtualCriticalSection::~VNonVirtualCriticalSection()
{
	assert(fUseCount == 0 && fOwner == NULL_TASK_ID && fEvent == NULL);

	while (fUseCount > 0)
		Unlock();
}


#pragma mark-

VSmallCriticalSection::VSmallCriticalSection()
{
	fEvent = NULL;
	fOwner = NULL_TASK_ID;
	fUseCount = 0;
}


Boolean VSmallCriticalSection::TryToLock()
{
	sWORD currentTaskID = (sWORD)VTask::GetCurrentID();
	LongAsDoubleWord doubleWord;

	doubleWord.first = (sWORD)currentTaskID;
	doubleWord.second = 0;

	sLONG owner = VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) 0, *((sLONG*) &doubleWord));

	if (owner == 0)
	{
		assert(fUseCount == 0);
		fUseCount = 1;
	}
	else if (((LongAsDoubleWord*)&owner)->first == currentTaskID)
	{
		++fUseCount;
	}

	return (fOwner == currentTaskID);
}


Boolean VSmallCriticalSection::Lock()
{
	sWORD	currentTaskID = (sWORD)VTask::GetCurrentID();
	LongAsDoubleWord	doubleWord;

	doubleWord.first = (sWORD)currentTaskID;
	doubleWord.second = 0;

	do {

		sLONG owner = VInterlocked::CompareExchange((sLONG*) &fOwner, (sLONG) 0, *((sLONG*) &doubleWord));
		if (owner == 0)
		{
			assert(fUseCount == 0);
			fUseCount = 1;
		}
		else if (fOwner == currentTaskID)
		{
			++fUseCount;
		}
		else
		{
			sLONG	unlockCount = sUnlockCount;
			VSyncEvent* event = VInterlocked::ExchangePtr(&fEvent);
			if (event != NULL)
			{
				event->Retain();

				if (VInterlocked::CompareExchangePtr((void**)&fEvent, NULL, event) != NULL)
				{
					event->Unlock();
					VTask::YieldNow();
				}
				else
				{
					if (sUnlockCount == unlockCount)
						event->Lock(100);
					else
						VTask::YieldNow();
				}
				
				event->Release();
			}
			else
			{
				event = new VSyncEvent;

				if (VInterlocked::CompareExchangePtr((void**)&fEvent, NULL, event) != NULL)
				{
					event->Release();
					VTask::YieldNow();
				}
			}
		}
	} while (fOwner != currentTaskID);
	
	return true;
}


Boolean VSmallCriticalSection::Unlock()
{
	//assert((fOwner == (sWORD)VTask::GetCurrentID()) && (fUseCount > 0) && (fUseCount < 32000L));

	assert(fUseCount > 0);
	VSyncEvent*	event = NULL;

	VInterlocked::Increment(&sUnlockCount);

	// Unlock or another successfull lock can only be called from same thead (so no contention can occur on fUseCount)
	if (--fUseCount == 0)
	{
		// capture currently set event
		event = VInterlocked::ExchangePtr(&fEvent);
		VTaskID owner = VInterlocked::Exchange((sLONG*) &fOwner, (sLONG) 0);
		//assert(owner == VTask::GetCurrentID());

		if (event != NULL)
		{
			event->Unlock();
			event->Release();
		}
	}
	return true;
}


VSmallCriticalSection::~VSmallCriticalSection()
{
	assert(fUseCount == 0 && fOwner == NULL_TASK_ID && fEvent == NULL);

	while (fUseCount > 0)
		Unlock();
}

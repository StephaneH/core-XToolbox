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
#include "VSignal.h"

BEGIN_TOOLBOX_NAMESPACE

/*
static void tt()
{
	struct TT
	{
		SignalParameterCloner<VUUID*>	f;
	} a;
	VUUID *t;
	a.f.Clone( t);
}
*/


VSignal::~VSignal()
{
}


void VSignal::SetEnabled(bool inSet)
{
	fEnabled = inSet;
}


VSignalDelegate* VSignal::DoRetainTriggerDelegate()
{
	return NULL;
}


void VSignal::ConnectDelegate( VSignalDelegate *inDelegate)
{
	if (inDelegate != NULL)
	{
		// first remove delegates for deleted targets
		PurgeVoidDelegates();

		// check if we already have the same target connected
		VTaskLock lock( &fMutex);

		bool found = false;	
		for( VSignalDelegates::iterator i = fDelegates.begin() ; i != fDelegates.end() ; ++i)
		{
			if (inDelegate == *i)
			{
				found = true;
				break;
			}
			else if ((*i)->IsSameTarget( inDelegate) )
			{
				// different delegate but same target object?
				// must disconnect from messaging task
				assert( ((*i)->GetMessageable() == NULL) || ((*i)->GetMessageable()->GetMessagingTask() == VTask::GetCurrentID()) );
				(*i)->SetEnabled( false);
				fDelegates.erase( i);
				break;
			}
		}

		if (!found)
		{
			fDelegates.push_back( inDelegate);
		}
	}
}


void VSignal::Disconnect( const VObject *inTarget)
{
	if (inTarget != NULL)
	{
		VTaskLock lock( &fMutex);

		for( VSignalDelegates::iterator i = fDelegates.begin() ; i != fDelegates.end() ; ++i)
		{
			if ((*i)->IsVObject( inTarget))
			{
				// must disconnect from messaging task
				assert( ((*i)->GetMessageable() == NULL) || ((*i)->GetMessageable()->GetMessagingTask() == VTask::GetCurrentID()) );
				(*i)->SetEnabled( false);
				fDelegates.erase( i);
				break;
			}
		}
	}
}


void VSignal::PurgeVoidDelegates()
{
	VTaskLock lock(&fMutex);

	for( VSignalDelegates::iterator i = fDelegates.begin() ; i != fDelegates.end();)
	{
		if ((*i)->GetMessageable() == NULL)
		{
			i = fDelegates.erase( i);
		}
		else
			++i;
	}
}


#pragma mark-

VSignalDelegate::VSignalDelegate(IMessageable* inMessageable, VObject* inTargetAsVObject)
{
	if (inMessageable == NULL)
		inMessageable = VTask::GetCurrent();

	fMessagingContext = (inMessageable == NULL) ? NULL : inMessageable->RetainMessagingContext();
	fTargetAsVObject = inTargetAsVObject;
	fEnabled = true;
}


VSignalDelegate::~VSignalDelegate()
{
	if (fMessagingContext != NULL)
		fMessagingContext->Release();
}



END_TOOLBOX_NAMESPACE
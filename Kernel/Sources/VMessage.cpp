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
#include "VMessage.h"
#include "VAssert.h"
#include "VMemoryCpp.h"
#include "VTask.h"
#include "VInterlocked.h"
#include "VSyncObject.h"


class IMessageableCompareTarget
{
public:
	IMessageableCompareTarget( const IMessageable *inTarget):fTarget( inTarget)	{}
	bool operator() ( const VMessage *inMessage) const		{ return inMessage->GetTarget() == fTarget;}

	const IMessageable *fTarget;
};


VMessagingContext::VMessagingContext(IMessageable *inTarget)
{
	fTarget = inTarget;
	fMessagingTask = VTask::GetCurrentID();
	fIsEnabled = true;
}


VMessagingContext::~VMessagingContext()
{
}


VTaskID VMessagingContext::GetMessagingTask() const
{
	return (fTarget == NULL) ? NULL_TASK_ID : fMessagingTask;
}


IMessageable* VMessagingContext::GetTarget() const
{
	if (fTarget == NULL)
		return NULL;
	return fTarget;
}


#pragma mark-

VMessage::VMessage() 
: fReplyEvent( NULL)
, fIsAnswered( false)
, fIsAborted( false)
, fContext( NULL)
{
}


VMessage::~VMessage()
{
	// deleting a pending message ?
	VMessagingContext* context = VInterlocked::ExchangePtr(&fContext);
	if (!testAssert(context == NULL))
		context->Release();

	// deleting a message someone is waiting for ?
	xbox_assert(fReplyEvent == NULL);
}


bool VMessage::Answered() const
{
	return fIsAnswered;
}


void VMessage::AnswerIt()
{
	xbox_assert(!fIsAnswered);
	fIsAnswered = true;
}


void VMessage::Execute()
{
	// capture the context
	VMessagingContext* context = VInterlocked::ExchangePtr(&fContext);
	if (context != NULL)
	{
		DoExecute();

		fIsAnswered = true;

		// this means that this message has been sent (and not posted) from another task which is waiting for the reply
		if (fReplyEvent != NULL)
		{
			VSyncEvent* syncEvent = fReplyEvent;
			syncEvent->Retain();
			syncEvent->Unlock();	// VSyncEvent::Unlock is not atomic. fReplyEvent will be release by caller and gets NULL.
			syncEvent->Release();
		}

		context->Release();
	}
}


void VMessage::Abort()
{
	// capture the context
	VMessagingContext* context = VInterlocked::ExchangePtr(&fContext);
	if (context != NULL)
	{
		fIsAborted = true;

		if (fReplyEvent != NULL)
		{
			VSyncEvent* syncEvent = fReplyEvent;
			syncEvent->Retain();
			syncEvent->Unlock();
			syncEvent->Release();
		}

		context->Release();
	}
}


void VMessage::DoExecute()
{
	IMessageable* target = (fContext == NULL) ? NULL : fContext->GetTarget();
	if (target != NULL)
		target->DoMessage(* this);
}


bool VMessage::ActivateContext( IMessageable* inTarget)
{
	VMessagingContext *context = inTarget->RetainMessagingContext();
	CopyRefCountable( &fContext, context);
	ReleaseRefCountable( &context);
	return fContext != NULL;
}


bool VMessage::SendTo( IMessageable* inTarget, sLONG inTimeoutMilliseconds)
{
	// synchronous sending of a message.
	// the message is not retained.

	xbox_assert(inTimeoutMilliseconds >= 0);
	xbox_assert(fReplyEvent == NULL);
	XBOX_ASSERT_VOBJECT(dynamic_cast<VObject* >(inTarget));

	if (this == NULL)
		return false;

	// NULL target ?
	if (!testAssert(inTarget != NULL))
		return false;

	// sending a posted message ?
	if (!testAssert(fContext == NULL))
		return false;

	bool ok = _Send( inTarget->RetainMessagingContext(), inTimeoutMilliseconds, false);

	return ok;
}


bool VMessage::SendTo( IMessageable* inTarget)
{
	// synchronous sending of a message.
	// the message is not retained.

	xbox_assert(fReplyEvent == NULL);
	XBOX_ASSERT_VOBJECT(dynamic_cast<VObject* >(inTarget));

	if (this == NULL)
		return false;

	// NULL target ?
	if (!testAssert(inTarget != NULL))
		return false;

	// sending a posted message ?
	if (!testAssert(fContext == NULL))
		return false;

	bool ok = _Send( inTarget->RetainMessagingContext(), 0, true);

	return ok;
}


bool VMessage::PostTo(IMessageable* inTarget, bool inSendIfSameTask, sLONG inDelayMilliseconds)
{
	// asynchronous sending of a message.
	// the message is retained and will be released after dispatching

	xbox_assert(inDelayMilliseconds >= 0);
	xbox_assert(fReplyEvent == NULL);
	xbox_assert(!inSendIfSameTask || inDelayMilliseconds == 0);
	XBOX_ASSERT_VOBJECT(dynamic_cast<VObject* >(inTarget));

	if (this == NULL)
		return false;

	// NULL target ?
	if (!testAssert(inTarget != NULL))
		return false;

	// already posted ?
	if (!testAssert(fContext == NULL))
		return true;

	bool ok;
	if (inDelayMilliseconds != 0)
	{
		// delay the message
		VTask *task = _InstallContextAndRetainTask( inTarget->RetainMessagingContext());
		if (testAssert( task != NULL))
		{
			ok = VTaskMgr::Get()->RegisterDelayedMessage(this, inDelayMilliseconds);
			ReleaseRefCountable( &task);
		}
		else
			ok = false;
	}
	else
	{
		ok = _Post( inTarget->RetainMessagingContext(), inSendIfSameTask);
	}

	return ok;
}


bool VMessage::PostToAndWaitExecutingMessages( IMessageable* inTarget)
{
	// asynchronous sending of a message.
	// the message is retained and will be released after dispatching

	xbox_assert(fReplyEvent == NULL);
	XBOX_ASSERT_VOBJECT(dynamic_cast<VObject* >(inTarget));

	if (this == NULL)
		return false;

	// NULL target ?
	if (!testAssert(inTarget != NULL))
		return false;

	// already posted ?
	if (!testAssert(fContext == NULL))
		return false;	// maybe we should just keep waiting executing messages

	xbox_assert( fReplyEvent == NULL);
	fReplyEvent = VTask::GetCurrent()->RetainMessageQueueSyncEvent();

	bool ok = _Post( inTarget->RetainMessagingContext(), true);
	if (ok)
	{
		while( !Answered())
		{
			VTask::ExecuteMessagesWithTimeout( 1000, true /* with idlers */);
			if (fIsAborted)
			{
				ok = false;
				break;
			}
		}
	}
	
	ReleaseRefCountable( &fReplyEvent);

	return ok;
}


void VMessage::RePost()
{
	// for delayed messages
	
	// would mean this msg is being sent ??
	xbox_assert(fReplyEvent == NULL);

	// message may have been aborted

	// temporarly capture the context (to protect against concurrent abortion)
	VMessagingContext* context = VInterlocked::ExchangePtr(&fContext);
	if (context == NULL)
		return;

	if (!context->IsEnabled())
	{
		context->Release();	// abort
	}
	else
	{
		VTaskRef msgTask(context->GetMessagingTask());
		if (testAssert((VTask*) msgTask != NULL))
		{
			// restore the context and push the msg
			VInterlocked::ExchangePtr(&fContext, context);
			msgTask->AddMessage(this);
		}
		else
		{
			// bad task id
			context->Release();	// abort
		}
	}
}


bool VMessage::SendOrPostTo(VMessagingContext* inContext, sLONG inDelayMilliseconds, bool inSynchronousIfSameTask, bool inSynchronous, sLONG inTimeoutMilliseconds)
{
	// asynchronous sending of a message.
	// the message context is retained and will be released after dispatching

	if (this == NULL)
		return false;

	// already posted ?
	if (!testAssert(fContext == NULL))
		return false;

	if (!testAssert(inContext != NULL))
		return false;
	
	xbox_assert(inDelayMilliseconds >= 0);
	xbox_assert(fReplyEvent == NULL);

	bool ok;
	if (inDelayMilliseconds != 0)
	{
		// delay the message
		ok = VTaskMgr::Get()->RegisterDelayedMessage( this, inDelayMilliseconds);
	}
	else if (inSynchronous)
	{
		ok = _Send( RetainRefCountable( inContext), inTimeoutMilliseconds, false);
	}
	else
	{
		ok = _Post( RetainRefCountable( inContext), inSynchronousIfSameTask);
	}
	
	return ok;
}


VTask *VMessage::_InstallContextAndRetainTask( VMessagingContext* inRetainedContext)
{
	xbox_assert( fContext == NULL);
	fIsAborted = false;
	fIsAnswered = false;
	
	VTask *task = NULL;
	if ( (inRetainedContext != NULL) && inRetainedContext->IsEnabled())
	{
		VTaskID msgTaskID = inRetainedContext->GetMessagingTask();
		task = (msgTaskID != NULL_TASK_ID) ? VTaskMgr::Get()->RetainTaskByID( msgTaskID) : NULL;
	}

	if (task != NULL)
		fContext = inRetainedContext;
	else if (inRetainedContext != NULL)
		inRetainedContext->Release();
	
	return task;
}


bool VMessage::_Send( VMessagingContext* inRetainedContext, sLONG inTimeoutMilliseconds, bool inIndefiniteTimeout)
{
	VTask *task = _InstallContextAndRetainTask( inRetainedContext);

	// does the target currently accept messaging ?

	// intentionnaly left uninitialized so that te c++ compiler inform us of unchecked cases
	bool ok;
	
	if (task != NULL)
	{
		if (task->IsCurrent())
		{
			// same task -> direct call
			DoExecute();
			fIsAnswered = true;
			ok = true;
		}
		else
		{
			fReplyEvent = new VSyncEvent;
			if (fReplyEvent == NULL)
				ok = false;
			else
				ok = task->AddMessage( this);
			if (ok)
			{
				if (inIndefiniteTimeout)
					ok = fReplyEvent->Lock();
				else
					ok = fReplyEvent->Lock( inTimeoutMilliseconds);

				if (!ok)
				{
					// timeout expired, needs to abort the message

					// try to capture the context
					VMessagingContext* context = VInterlocked::ExchangePtr(&fContext);
					if (context == NULL)
					{
						// the message is currently executing or aborting
						// we MUST wait else we would delete an executing message
						
						// if we need to specify a timeout, take care of releasing the context
						fReplyEvent->Lock(); // indefinite wait
						ok = !fIsAborted;
					}
					else
					{
						// we captured the context so this msg will never get executed
						fIsAborted = true;
						ReleaseRefCountable( &context);
					}
				}
				else
				{
					ok = !fIsAborted;
				}
			}
		}
	}
	else
	{
		ok = false;
	}

	ReleaseRefCountable( &task);
	ReleaseRefCountable( &fContext);
	ReleaseRefCountable( &fReplyEvent);

	return ok;
}


bool VMessage::_Post( VMessagingContext* inRetainedContext, bool inSynchronousIfSameTask)
{
	VTask *task = _InstallContextAndRetainTask( inRetainedContext);

	// intentionnaly left uninitialized so that te c++ compiler inform us of unchecked cases
	bool ok;
	
	if (task != NULL)
	{
		if (inSynchronousIfSameTask && task->IsCurrent())
		{
			// same task -> direct call
			DoExecute();
			fIsAnswered = true;
			ReleaseRefCountable( &fContext);
			ok = true;
		}
		else
		{
			ok = task->AddMessage( this);
			if (!ok)
				ReleaseRefCountable( &fContext);
		}
	}
	else
	{
		ok = false;
		ReleaseRefCountable( &fContext);
	}

	ReleaseRefCountable( &task);

	return ok;
}


OsType VMessage::GetCoalescingSignature() const
{
	return 0;
}


bool VMessage::DoCoalesce(const VMessage& /*inNewMessage*/)
{
	return true;
}


#pragma mark-


VMessageQueue::VMessageQueue()
{
	fCriticalSection = new VCriticalSection;
	fEvent = new VSyncEvent;
}


VMessageQueue::~VMessageQueue()
{
	fMessageBox.clear();
	delete fCriticalSection;
	ReleaseRefCountable( &fEvent);
}


bool VMessageQueue::AddMessage( VMessage* inMessage)
{
	xbox_assert(!inMessage->Answered() /* on envoie un msg deja valide ??? */);

	// process special messages
	bool isOK = true;
	
	OsType signature = inMessage->GetCoalescingSignature();
	if (signature != 0)
	{
		VTaskLock lock( fCriticalSection);
		
		DequeOfVMessage::iterator i = fMessageBox.begin();
		for(  ; (i != fMessageBox.end()) && ((*i)->GetCoalescingSignature() != signature) ; ++i)
			;

		if (i != fMessageBox.end())
		{
			VMessage* tocoalesce = *i;
			// warning: called from inside the task lock! (to avoid Getting this message while processing it)
			isOK = tocoalesce->DoCoalesce( *inMessage);
		}
	}

	// push it
	if (isOK)
	{
		VTaskLock lock( fCriticalSection);

		try
		{
			// optim: if the message box was not empty, no need to set the event because it should be already set.
			if (fMessageBox.empty())
				fEvent->Unlock();
			fMessageBox.push_back( inMessage);
		}
		catch(...)
		{
			isOK = false;
		}
	}
	
	return isOK;
}


VMessage* VMessageQueue::_RetainFrontMessage()
{
	VMessage *msg = fMessageBox.front().Forget();
	fMessageBox.pop_front();
	XBOX_ASSERT_VOBJECT( msg);
	xbox_assert(!msg->Answered() /* on envoie un msg deja executed ??? */);
	if (fMessageBox.empty())
		fEvent->Reset();
	
	return msg;
}


VMessage* VMessageQueue::RetainMessage()
{
	VTaskLock lock( fCriticalSection);

	return fMessageBox.empty() ? NULL : _RetainFrontMessage();
}


VMessage* VMessageQueue::RetainMessageWithTimeout( sLONG inTimeoutMilliseconds)
{
	if (!fEvent->Lock( inTimeoutMilliseconds))
		return NULL;
	
	VTaskLock lock( fCriticalSection);

	VMessage *msg;
	if (fMessageBox.empty())
	{
		// the event was triggered from the outside, we must reset here ourselves
		fEvent->Reset();
		msg = NULL;
	}
	else
	{
		msg = _RetainFrontMessage();
	}
	return msg;
}


void VMessageQueue::CancelMessages( IMessageable* inTarget)
{
	VTaskLock lock( fCriticalSection);

	DequeOfVMessage::iterator i = std::remove_if( fMessageBox.begin(), fMessageBox.end(), IMessageableCompareTarget( inTarget));
	
	for( ; i != fMessageBox.end() ; ++i)
		(*i)->Abort();
	
	fMessageBox.erase( i, fMessageBox.end());

	if (fMessageBox.empty())
		fEvent->Reset();
}


void VMessageQueue::CancelMessages()
{
	VTaskLock lock( fCriticalSection);

	DequeOfVMessage::iterator i = fMessageBox.begin();

	for( ; i != fMessageBox.end() ; ++i)
		(*i)->Abort();
	
	fMessageBox.erase( i, fMessageBox.end());

	if (fMessageBox.empty())
		fEvent->Reset();
}


bool VMessageQueue::RemoveMessage( VMessage* inMessage)
{
	VTaskLock lock( fCriticalSection);
	
	DequeOfVMessage::iterator i = std::find( fMessageBox.begin(), fMessageBox.end(), VRefPtr<VMessage>( inMessage));

	bool isFound = (i != fMessageBox.end());
	if (isFound)
	{
		fMessageBox.erase( i);
		if (fMessageBox.empty())
			fEvent->Reset();
	}

	return isFound;
}


sLONG VMessageQueue::CountMessages() const
{
	VTaskLock lock( fCriticalSection);
	return (sLONG) fMessageBox.size();
}


sLONG VMessageQueue::CountMessagesFor(IMessageable* inTarget) const
{
	VTaskLock lock( fCriticalSection);

	sLONG count = 0;
	for( DequeOfVMessage::const_iterator i = fMessageBox.begin() ; i != fMessageBox.end() ; ++i)
	{
		if ((*i)->GetTarget() == inTarget)
			++count;
	}
	return count;
}


bool VMessageQueue::IsEmpty() const
{
	VTaskLock lock( fCriticalSection);
	
	return fMessageBox.empty();
}


void VMessageQueue::SetEvent()
{
	fEvent->Unlock();
}

#pragma mark-

IMessageable::IMessageable()
{
	fContext = new VMessagingContext(this);
}


void IMessageable::DoMessage(VMessage& /*inMessage*/)
{
	// unhandled message, not necessarly an error
}


IMessageable::~IMessageable()
{
	if (fContext != NULL)
	{
		// ensure this object is being deleted in its messaging task
		// else, one may be sending a message to us while it's being deleted.
		xbox_assert(!fContext->IsEnabled() || (fContext->GetMessagingTask() == VTask::GetCurrentID()));
		
		fContext->SetTarget(NULL);
		fContext->Release();
		fContext = NULL;
	}
}


void IMessageable::StartMessaging()
{
	if (fContext != NULL)
		fContext->SetEnabled(true);
}


void IMessageable::StopMessaging()
{
	if (fContext != NULL)
		fContext->SetEnabled(false);
}


VMessagingContext* IMessageable::RetainMessagingContext()
{
	if (fContext == NULL)
		return NULL;
	
	fContext->Retain();
	
	return fContext;
}


VTaskID IMessageable::GetMessagingTask() const
{
	return (fContext == NULL) ? NULL_TASK_ID : fContext->GetMessagingTask();
}


void IMessageable::SetMessagingTask(VTaskID inTaskID)
{
	if (fContext != NULL)
		fContext->SetMessagingTask(inTaskID);
}

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
#ifndef __VMessage__
#define __VMessage__

#include <deque>

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IRefCountable.h"

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
class IMessageable;
class VMessage;


// Needed declarations
class VTask;
class VCriticalSection;
class VSemaphore;
class VSyncEvent;

// Internal class
class XTOOLBOX_API VMessagingContext : public VObject, public IRefCountable
{ 
public:
							VMessagingContext( IMessageable* inTarget);
	virtual					~VMessagingContext();

			IMessageable*	GetTarget() const;
			void			SetTarget( IMessageable* inTarget)			{ fTarget = inTarget; }
	
			void			SetMessagingTask( VTaskID inMesssagingTask)	{ fMessagingTask = inMesssagingTask; }
			VTaskID			GetMessagingTask() const;
	
			bool			IsEnabled() const							{ return fIsEnabled && (fTarget != NULL); }
			void			SetEnabled( bool inEnabled)					{ fIsEnabled = inEnabled; }

private:
			IMessageable*	fTarget;
			VTaskID			fMessagingTask;
			bool			fIsEnabled;
};


typedef std::deque<VRefPtr<VMessage> >	DequeOfVMessage;


/*!
	@class	VMessage
	@abstract	A VMessage is the mecanism to communicate some information between objects.
	@discussion
		To be able to receive a message an object must inherit the IMessageable interface.
		Then it must tell in which task context any message received will be processed.
		This is its 'messaging task' which by default is the current task at the time the
		object is constructed.
		
		1- Using a message to send data.
		
			A message may be used to send some data to an IMessageable object.
			You need to build your own class inherited from VMessage to put your data.
			Then you build your VMessage and send it to destination by writing:

			VMyNameMessage msg (firstName, lastName);
			msg.SendTo (messageableDestination);
			
			The virtual method IMessageable::DoMessage of messageableDestination will be called in its messaging
			task context and will receive the VMyNameMessage as parameter.
			
			Typically VFrame uses VFrameMessage objects to communicate between frames not
			necessarly in the same task and triggering a VCommand sends a VCommandMsg to
			command listeners.
			
		2- Using a message to execute some code in a specific task context.

			A message may also contain no data at all but just a method pointer to be executed
			in the destination task context.
			
			The method is VMessage::DoExecute that you may override. If you need some parameters, store
			them in a derived version of VMessage.
			
			For instance if you want to call 'Beep()' in task whose id is targetTaskID:
			Declare a VBeepMessage class inherited from VMessage for which VBeepMessage::DoExecute calls Beep.
			then to execute 'Beep' in target task, write:

			VTask* task = gTaskMgr->RetainTaskByID (targetTaskID);
			VBeepMessage msg;
			msg.SendTo (task);
			task->Release();
			
		3- Send / Post
			
			Sending a message means the sender waits until the receiver has processed the message.
			Posting a message means you don't want to wait.
			
		4- Answering a message
		
			A VMessage provides a boolean flag that you may use to detect if the message receiver has
			properly received and processed the message.
			This boolean is there as a facility for application usage and is not used by the messaging mecanism
			internally.
			
			VMessage objects are retainable so there should be no ambguities about messages deallocation.
		
*/
class XTOOLBOX_API VMessage : public VObject, public IRefCountable
{ 
friend class VTask;
friend class VTaskMgr;
friend class VMessageQueue;
friend class IMessageable;
public:

	/*!
		@function	VMessage
		@abstract	VMessage constructor
	*/
	VMessage();

	
	/*!
		@function	Answered
		@abstract	Returns the 'answered' boolean flag
		@discussion
			The 'answered' boolean flag is set to false when constructing a VMessage.
			The receiver of a message should call AnswerIt on it to tell the sender it has
			successfully processed the message.
	*/
	bool	Answered() const;

	
	/*!
		@function	AnswerIt
		@abstract	Sets to true the 'answered' boolean flag
	*/
	void	AnswerIt();

	/*!
		@function	SendTo
		@abstract Sends this VMessage to a target.
		@param inTarget the destination object
		@param inTimeoutMilliseconds timeout in milliseconds (defaults to 20s)
		@result Boolean true if the message has been successfully sent.
		@discussion
			The caller is blocked until the target has replied to the message.
			returns false if the message could not be sent.
			You may specify a timeout.

			Typical use is such as:

			VMessage msg;
			msg.SendTo (target);
			if (msg.Answered()) { 
				// he got my message
			 };

			Note that since SendTo is synchronous, it is just as safe as writing:
			
			VMessage* msg = new VMessage;
			msg->SendTo (target);
			msg->Release();
	*/
	bool	SendTo( IMessageable* inTarget, sLONG inTimeoutMilliseconds);

	// indefinite timeout
	bool	SendTo( IMessageable* inTarget);

	/*!
		@function	PostTo
		@abstract Post this VMessage to the target message queue.
		@param inTarget the destination object
		@param inSendIfSameTask calls SendTo if destination is in current task
		@param inDelayMilliseconds delay in milliseconds for actually posting the message
		@result Boolean true if the message has been successfully posted.
		@discussion
			if inSendIfSameTask is false (default) PosTo push the messge into destination task message queue and returns.
			The VMessage is processed at a later time when the target owner task calls CheckMessages.
			Pass a non zero delay if you want the message to be pushed to the target message queue later.
			It retains the VMessage and release it when done.
			If inSendIfSameTask is true and the target owner task is the current task, a SendTo is performed instead. (inDelayMilliseconds must be 0).

			Typical use is such as:

			VMessage* msg = new VMessage;
			msg->PostTo (target);
			msg->Release();
	*/
	bool	PostTo( IMessageable* inTarget, bool inSendIfSameTask = false, sLONG inDelayMilliseconds = 0);

	/*!
		@function	PostToAndWaitExecutingMessages
		@abstract Post this VMessage to the target message queue and wait until the message is executed.
		During the wait, the task executes received messages.
		@param inTarget the destination object
		@result Boolean true if the message has been successfully executed.
		@discussion
			Use PostToAndWaitExecutingMessages instead of SendTo when you want to execute messages while waiting for message execution.
	*/
	bool	PostToAndWaitExecutingMessages( IMessageable* inTarget);

	/*!
		@function SendOrPostTo
		@abstract Same behavior as SendTo or PostTo depending on paramerers.
		@param inContext valid messaging context obtained from IMessageable::RetainMessagingContext.
		@param inDelayMilliseconds if not zero, message will be posted with specified delay and extra params ignored.
		@param inSynchronousIfSameTask if true and messaging task is the current one, it turns into a direct procedure call.
		@param inSynchronous if true, the msg is sent and blocks until replied. Else, it is posted without waiting for the reply.
		@param inTimeoutMilliseconds timeout used only for waiting for a sent message.
	*/
	bool	SendOrPostTo( VMessagingContext* inContext, sLONG inDelayMilliseconds, bool inSynchronousIfSameTask, bool inSynchronous, sLONG inTimeoutMilliseconds = 20000L);
	
	/*!
		@function	GetCoalescingSignature
		@abstract Returns the signature used for coalescing messages.
		@result OsType coalescing signatture
		@discussion
			A message with a non 0 coalescing signature means it wants DoCoalesce be called
			when queuing messages with same signature.
	*/
	virtual OsType	GetCoalescingSignature() const;

	// called from inside CheckMessages
	void			Execute();
	
	/*!
		@function	GetTarget
		@abstract Returns the current target for this VMessage.
		@result IMessageable target
		@discussion
			The target may be non-NULL only after SendTo and PostTo.
			But it is not guaranteed to be non-NULL even after SendTo or PostTo because
			the target may get destroyed while a message is posted for it.
			It's typically used from inside DoExecute (that's why it should be protected).
	*/
	IMessageable*	GetTarget() const { return (fContext == NULL) ? NULL : fContext->GetTarget(); }

	// for executing a message without sending or posting it.
	bool			ActivateContext( IMessageable* inTarget);
	
protected:

	VMessagingContext*	fContext;	// intermediate class for safe access to target
	VSyncEvent*			fReplyEvent;	// used for synchronous sending
	#if ( __GNUC__ && __i386__ )
	long		fIsAnswered;	// waiting for gcc bugfix
	long		fIsAborted;
	#else
	bool		fIsAnswered;	// for application needs
	bool		fIsAborted;
	#endif

	/*!
		@function	~VMessage
		@abstract	VMessage destructor
		@discussion
			VMessage is managed thru Retain/Release so you should never call delete by yourself.
			Asserts if deleting a posted message.
	*/
	virtual		~VMessage();

	/*!
		@function	DoExecute
		@abstract The message is being executed in target's messaging task.
		@discussion
			By default DoExecute calls the target method IMessageable::DoMessage.
			You may override DoExecute to do whatever you want.
	*/
	virtual void	DoExecute();

	/*!
		@function	DoCoalesce
		@abstract Gives oportunity to control new message queuing.
		@result Boolean true if new message must be queued.
		@discussion
			If you gave him this message a non NULL coalescing signature
			then DoCoalesce is called for any message with same signature tentatively being queued.
			You may reject message queuing by returning false;
			Beware while DoCoalesce is called the receiving task queuing system is blocked.
			If you want to forget this message you may call Abort.
	*/
	virtual bool	DoCoalesce( const VMessage& inNewMessage);

	// Try to Abort a message (release context)
			void	Abort();

	// Post again a delayed message
			void	RePost();
	
private:
			bool	_Send( VMessagingContext* inRetainedContext, sLONG inTimeoutMilliseconds, bool inIndefiniteTimeout);
			bool	_Post( VMessagingContext* inRetainedContext, bool inSynchronousIfSameTask);
			VTask*	_InstallContextAndRetainTask( VMessagingContext* inRetainedContext);
};


/*!
	@class	VMessageQueue
	@abstract	Thread safe queue for VMessage
	@discussion
*/

class XTOOLBOX_API VMessageQueue : public VObject
{
public:
								VMessageQueue();
	virtual						~VMessageQueue();
	
			VMessage*			RetainMessage();
			VMessage*			RetainMessageWithTimeout( sLONG inTimeoutMilliseconds);
			
			bool				AddMessage( VMessage* inMessage);
			bool				RemoveMessage( VMessage* inMessage);
			
			void				CancelMessages( IMessageable* inTarget);
			void				CancelMessages();
			
			sLONG				CountMessages() const;
			sLONG				CountMessagesFor( IMessageable* inTarget) const;
			
			bool				IsEmpty() const;
			
			void				SetEvent();

			VSyncEvent*			GetSyncEvent() const	{ return fEvent;}

private:
			VMessage*			_RetainFrontMessage();
			
			DequeOfVMessage		fMessageBox;
			VCriticalSection*	fCriticalSection;
			VSyncEvent*			fEvent;
};


/*!
	@class	IMessageable
	@abstract	Inherits from this interface if you want to receive VMessages.
	@discussion
		A VMessage can only be sent to a IMessageable.
		See VMessage for more details.
		An IMessageable is built enabled for messaging and messaging is stopped in destructor.
		You may want to start/stop messaging at different times.
*/

class XTOOLBOX_API IMessageable
{ 
public:
	/*!
		@function	IMessageable
		@abstract	Constructor
		@discussion
			Messaging is enabled by default
			By default, the current task will be the task into which to execute received messages.
	*/
	IMessageable();


	/*!
		@function	~IMessageable
		@abstract	Destructor
		@discussion
			Messaging is then stopped.
	*/
	virtual ~IMessageable();

	
	/*!
		@function	SetMessagingTask
		@abstract	Sets the task in which VMessages for this object will be executed.
		@discussion
			The default is the current task at the time this object is built.
		@param	inTaskID The messaging task ID for subsequent messages.
	*/
	void	SetMessagingTask (VTaskID inTaskID);
	
	
	/*!
		@function	GetMessagingTask
		@abstract	Returns the task in which VMessages for this object will be executed.
		@result VTaskID messaging task id
	*/
	VTaskID	GetMessagingTask() const;


	/*!
		@function	StartMessaging
		@abstract	Starts messaging.
		@discussion
			Enable messaging for this object.
	*/
	void	StartMessaging();


	/*!
		@function	StopMessaging
		@abstract	Stops messaging.
		@discussion
			Disable messaging for this object.
			Already posted messages will remain queued but will not be executed and will be simply dropped.
	*/
	void	StopMessaging();


	/*!
		@function	IsMessagingEnabled
		@abstract	Tells if messaging is enabled for this object.
		@result Boolean true is enabled
	*/
	bool	IsMessagingEnabled() const { return (fContext == NULL) ? false : fContext->IsEnabled(); }

	VMessagingContext*	RetainMessagingContext();

protected:
	friend class VMessage;
	
	VMessagingContext*	fContext;
	
	/*!
		@function	DoMessage
		@abstract	The method called to execute a message.
		@discussion
			The default behavior for VMessage::DoExecute is to call IMessageable::DoMessage.
			Override this method to do whatever you need in response to the given message.
			You are called in the task context given by IMessageable::GetMessagingTask.
		@param	inMessage The VMessage sent to you.
	*/
	virtual void DoMessage (VMessage& inMessage);
};

END_TOOLBOX_NAMESPACE

#endif
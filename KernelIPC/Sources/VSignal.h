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
#ifndef __VSignal__
#define __VSignal__

#include "Kernel/VKernel.h"

#include <list>

BEGIN_TOOLBOX_NAMESPACE



/*
	utility template used for message parameter passing.
	
	you may need to specialize it.
*/
template<class ARG, class Enable = void>
class SignalParameterCloner
{
public:
	static ARG		Clone( ARG inArg)				{return inArg;}
	static void		DisposeClone( ARG /*inArg*/)	{;}
};


#if WITH_SUPPORT_FOR_TEMPLATE_SPECIALIZATION
/*
	marche avec CW 9.3 mais pas avec VStudio 2002
	
	Il parait que ca marche avec VStudio 2003
*/


/*
	private utility template for using is_base_and_derived
*/
template<class Base, class Candidate>
struct signal_parameter_derived_from
{
	static const bool value = is_pointer<Base>::value
							&& is_pointer<Candidate>::value
							&& is_base_and_derived<Base,Candidate>::value;
};

/*
	Specialization for IRefCountable.
*/
template<class ARG>
class SignalParameterCloner<ARG, typename enable_if< signal_parameter_derived_from<const IRefCountable*,ARG> >::type>
{
public:
	static ARG		Clone( ARG inArg)				{if (inArg != NULL) inArg->Retain(); return inArg;}
	static void		DisposeClone( ARG inArg)		{if (inArg != NULL) inArg->Release();}
};


/*
	Specialization for VValue.
*/
template<class ARG>
class SignalParameterCloner<ARG, typename enable_if< signal_parameter_derived_from<const VValue*,ARG> >::type>
{
public:
	static ARG		Clone( ARG inArg)				{return (inArg == NULL) ? NULL : (ARG) inArg->Clone();}
	static void		DisposeClone( ARG inArg)		{delete inArg;}
};
#else


#define SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(ARG) template<> class SignalParameterCloner<ARG*, void> { public:\
	static ARG*		Clone( ARG* inArg)					{if (inArg != NULL) inArg->Retain(); return inArg;}\
	static void		DisposeClone( ARG* inArg)			{if (inArg != NULL) inArg->Release();} };

SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(IRefCountable)
SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(VValueBag)
SIGNAL_PARAMETER_CLONER_RETAIN_RELEASE(const VValueBag)

#define SIGNAL_PARAMETER_CLONER_CLONE_DELETE(ARG) template<> class SignalParameterCloner<ARG*, void> { public:\
	static ARG*		Clone( ARG* inArg)					{return (inArg == NULL) ? NULL : (ARG*) inArg->Clone();}\
	static void		DisposeClone( ARG* inArg)			{delete inArg;} };

SIGNAL_PARAMETER_CLONER_CLONE_DELETE(VUUID)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VUUID)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(VString)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VString)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(VLong)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VLong)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(VLong8)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VLong8)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayReal)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayBoolean)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayString)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayLong)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayWord)
SIGNAL_PARAMETER_CLONER_CLONE_DELETE(const VArrayLong8)


#endif



class XTOOLBOX_API VSignalDelegate : public VObject, public IRefCountable
{
public:
								VSignalDelegate( IMessageable* inMessageable, VObject* inTargetAsVObject);
	virtual						~VSignalDelegate ();

			IMessageable*		GetMessageable()											{ return (fMessagingContext == NULL) ? NULL : fMessagingContext->GetTarget(); }
			VMessagingContext*	GetMessagingContext()										{ return fMessagingContext; }

	virtual void				InvokeWithMessage( VMessage *inMessage) = 0;

			bool				IsVObject( const VObject* inObject) const					{ return fTargetAsVObject == inObject; }
			bool				IsSameTarget( const VSignalDelegate* inDelegate) const		{ return fTargetAsVObject == inDelegate->fTargetAsVObject; }

			bool				IsEnabled() const											{ return fEnabled;}
			void				SetEnabled( bool inEnabled)									{ fEnabled = inEnabled;}
			
protected:
								VSignalDelegate()											{ fMessagingContext = NULL; }

private:
								VSignalDelegate( const VSignalDelegate&);
								
			VMessagingContext*	fMessagingContext;
			VObject*			fTargetAsVObject;	// may be destroyed objects, may even points to another object
			bool				fEnabled;
};


typedef std::list<VRefPtr<VSignalDelegate> >		VSignalDelegates;


template<class ARG1 = VoidType, class ARG2 = VoidType, class ARG3 = VoidType, class ARG4 = VoidType>
class VSignalMessageT : public VMessage
{
public:
	VSignalMessageT( VSignalDelegate* inDelegate, ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4)
		:fDelegate(inDelegate)
		, fArg1(SignalParameterCloner<ARG1>::Clone(inArg1))
		, fArg2(SignalParameterCloner<ARG2>::Clone(inArg2))
		, fArg3(SignalParameterCloner<ARG3>::Clone(inArg3))
		, fArg4(SignalParameterCloner<ARG4>::Clone(inArg4))
	{
		inDelegate->Retain();
	};

	virtual ~VSignalMessageT()
	{
		SignalParameterCloner<ARG1>::DisposeClone(fArg1);
		SignalParameterCloner<ARG2>::DisposeClone(fArg2);
		SignalParameterCloner<ARG3>::DisposeClone(fArg3);
		SignalParameterCloner<ARG4>::DisposeClone(fArg4);
		fDelegate->Release();
	}
	
	virtual void DoExecute()
	{
		if (fDelegate->IsEnabled())
			fDelegate->InvokeWithMessage(this);
	}
	
	VSignalDelegate*	fDelegate;
	ARG1				fArg1;
	ARG2				fArg2;
	ARG3				fArg3;
	ARG4				fArg4;
};


template<class OBJECT, class T>
class VSignalDelegateT_0 : public VSignalDelegate
{
public:
	VSignalDelegateT_0 (OBJECT* inTarget, IMessageable* inMessageable, T inMethod)
		: VSignalDelegate(inMessageable, dynamic_cast<VObject*>(inTarget)), fTarget(inTarget), fMethod(inMethod) {}

	virtual void InvokeWithMessage( VMessage* /*inMessage*/)
	{
		(fTarget->*fMethod)();
	}

private:
	OBJECT*	fTarget;
	T		fMethod;
};

#if COMPIL_GCC
// gcc 4.0.1 fails at dynamic_casting this
#define CAST_MESSAGE( CASTED_MSG, T, MSG)	T* CASTED_MSG = reinterpret_cast< T* >( MSG);
#else
#define CAST_MESSAGE( CASTED_MSG, T, MSG)	T* CASTED_MSG = dynamic_cast< T* >( MSG);
#endif

template<class OBJECT, class T, class T1>
class VSignalDelegateT_1 : public VSignalDelegate
{
public:
	typedef VSignalMessageT<T1>	message_type;
	
	VSignalDelegateT_1 (OBJECT* inTarget, IMessageable* inMessageable, T inMethod)
		: VSignalDelegate(inMessageable, dynamic_cast<VObject*>(inTarget)), fTarget(inTarget), fMethod(inMethod) {}

	virtual void InvokeWithMessage( VMessage* inMessage)
	{
		CAST_MESSAGE( msg, message_type, inMessage);
		if (testAssert( msg != NULL))
			(fTarget->*fMethod)(msg->fArg1);
	}

private:
	OBJECT* fTarget;
	T	fMethod;
};


template<class OBJECT, class T, class T1, class T2>
class VSignalDelegateT_2 : public VSignalDelegate
{
public:
	typedef VSignalMessageT<T1,T2>	message_type;

	VSignalDelegateT_2 (OBJECT* inTarget, IMessageable* inMessageable, T inMethod)
		: VSignalDelegate(inMessageable, dynamic_cast<VObject*>(inTarget)), fTarget(inTarget), fMethod(inMethod) {}

	virtual void InvokeWithMessage (VMessage* inMessage)
	{
		CAST_MESSAGE( msg, message_type, inMessage);
		if (testAssert( msg != NULL))
			(fTarget->*fMethod)(msg->fArg1, msg->fArg2);
	}

private:
	OBJECT* fTarget;
	T	fMethod;
};


template<class OBJECT, class T, class T1, class T2, class T3>
class VSignalDelegateT_3 : public VSignalDelegate
{
public:
	typedef VSignalMessageT<T1,T2,T3>	message_type;

	VSignalDelegateT_3 (OBJECT* inTarget, IMessageable* inMessageable, T inMethod)
		: VSignalDelegate(inMessageable, dynamic_cast<VObject*>(inTarget)), fTarget(inTarget), fMethod(inMethod) {}

	virtual void InvokeWithMessage (VMessage* inMessage)
	{
		CAST_MESSAGE( msg, message_type, inMessage);
		if (testAssert( msg != NULL))
			(fTarget->*fMethod)(msg->fArg1, msg->fArg2, msg->fArg3);
	}
	
private:
	OBJECT* fTarget;
	T	fMethod;
};


template<class OBJECT, class T, class T1, class T2, class T3, class T4>
class VSignalDelegateT_4 : public VSignalDelegate
{
public:
	typedef VSignalMessageT<T1,T2,T3,T4>	message_type;

	VSignalDelegateT_4 (OBJECT* inTarget, IMessageable* inMessageable, T inMethod)
		: VSignalDelegate(inMessageable, dynamic_cast<VObject*>(inTarget)), fTarget(inTarget), fMethod(inMethod) {}

	virtual void InvokeWithMessage (VMessage* inMessage)
	{
		CAST_MESSAGE( msg, message_type, inMessage);
		if (testAssert( msg != NULL))
			(fTarget->*fMethod)(msg->fArg1, msg->fArg2, msg->fArg3, msg->fArg4);
	}
	
private:
	OBJECT* fTarget;
	T	fMethod;
};

#undef CAST_MESSAGE

/*!
	@class	VSignal
	@abstract	Thread aware broadcast function pointer.
	@discussion
		A signal might be seen as an array of function pointers.
		When you trigger a signal, you call a set of function pointers previously connected to the signal object.
		
		It's a notification mechanism between a signal triggerer and signal listeners.
		
		Example:
		========
			
			Imagine you want to advertise the opening of a window so that some other part of the program
			may collect new window titles.
			
			First declare the signal object.
			
			// the "window opened" signal will accept one VString* parameter for the window title.
			VSignalT_1<VString*> gWindowOpened;

			// VSignalT_1 is a class template for a signal with 1 parameter.


			Then other parts of the program may connect to the signal by providing an object and a member function pointer.
			An IMessageable interface is also required to manage inter-thread communication.
			
			// connect the window list to the "window opened" signal.
			// we'll be notified in the main task context.
			gWindowOpened.Connect(gWindowList, VTask::GetMain(), VWindowList::ANewWindowHasBeenOpened);
			
			// VWindowList might be declared like this:
			class VWindowList : public VObject
			{
				public:
					void ANewWindowHasBeenOpened(VString* inWindowTitle)
					{
						fWindowTitles.Add(inWindowTitle);
					}
			};


			Then when a window is opened, the window manager triggers the signal:
			
			VString windowTitle;
			newWindow.GetTitle(windowTitle);
			gWindowOpened(&windowTitle);	// calls gWindowList->ANewWindowHasBeenOpened(&windowTitle);
		

		Connection:
		===========
			Connections for a signal are identified by the target object.
			There may be only one connection per target object.
			If you call Connect() on a signal a second time with the same target object,
			the first connection is replaced by the new one.
			
		
		Synchronousity:
		===============
			
			By default a signal listener is called synchronously if its messaging task is the current one
			at the time the signal is triggered. Else the signal listener is called asynchronously via a message post.
			
			You can control this behavior for instance to force synchronousity or to force asynchronousity via the SetAsynchronousity method.
			
	
		Signal Parameters:
		==================
			
			Signals may have from 0 to 4 parameters.
			To declare a signal use templates VSignalT_0, VSignalT_1, VSignalT_2, VSignalT_3, VSignalT_4.
			
			Parameters can be of any type but cannot be passed as reference.
			
			While dispatching a signal call, parameters are copied into intermediate buffers using the SignalParameterCloner template.
			By default this cloning function is the identity function.
			It is specialized for the VValue* type for which it calls VValue::Clone
			And for the IRefCountable* type for which it calls IRefCountable::Retain
			This ensures proper behavior for asynchronous signaling.
			
			If you intend to use you own parameter type in a signal that might be used in asynchronous mode, you MUST provide
			SignalParameterCloner template specialisation.


*/
class XTOOLBOX_API VSignal : public VObject, public IRefCountable
{
public:
	
	typedef enum {
		ESM_Synchonous = 1,
		ESM_Asynchonous = 2,
		ESM_SynchonousIfSameTask = 3
	} ESignalingMode;
	
	/*!
		@function VSignal
		@abstract Constructor
		@discussion
			By default is signal is build enabled and in "synchronous if same task" mode.
	*/
	
								VSignal( ESignalingMode inMode):fEnabled( true),fSignalingMode( inMode)	{;}
	virtual						~VSignal();
	
			/*!
				@function IsEnabled
				@abstract Tells if the signal is enabled.
				@discussion
					A disabled signal silently ignore signal triggering.
			*/
			bool				IsEnabled() const							{ return fEnabled; }
			
			/*!
				@function SetEnabled
				@abstract Enable or Disable the signal
				@parameter inSet pass false to enable the signal
				@discussion
					A disabled signal silently ignore signal triggering.
			*/
			void				SetEnabled( bool inSet);
			
			/*!
				@function IsAllwaysAsynchronous
				@abstract Returns true if signal listeners are always called in asynchronous mode 
				@discussion
					Calling a signal listener in asynchronous mode means a message is always posted to it.
					Beware to properly provide SignalParameterCloner template specialisation for custom parameter types.
			*/
			bool				IsAllwaysAsynchronous() const				{ return fSignalingMode == ESM_Asynchonous; }

			/*!
				@function IsAllwaysSynchronous
				@abstract Returns true if signal listeners are always called in synchronous mode 
				@discussion
					Calling a signal listener in synchronous mode means a message is always sent to it.
			*/
			bool				IsAllwaysSynchronous() const				{ return fSignalingMode == ESM_Synchonous; }

			/*!
				@function IsSynchronousIfSameTask
				@abstract Returns true if signal listeners are always called in "synchronous if same task" mode 
				@discussion
					This the default mode.
					Calling a signal listener in "synchronous if same task" mode means a message is always sent to it
					if the listener messaging task is the current one at the time the signal is triggered.
					Else a message is posted.
			*/
			bool				IsSynchronousIfSameTask() const				{ return fSignalingMode == ESM_SynchonousIfSameTask; }

			/*!
				@function SetAsynchronousity
				@abstract Set the synchronousity mode 
				@parameter inAsynchronous mode
				@discussion
					Pass TRI_STATE_ON for "always asynchronous"
					Pass TRI_STATE_OFF for "always synchronous"
					Pass TRI_STATE_LATENT for "synchronous if same task"
			*/
			void				SetSignalingMode( ESignalingMode inMode)	{ fSignalingMode = inMode; }
			
			/*!
				@function Disconnect
				@abstract Disconnect a signal listener
				@parameter inTarget object passed as first parameter in VSignal::Connect
				@discussion
					All connections established using Connect on specified target are removed.
			*/
			void				Disconnect( const VObject* inTarget);

			void				ConnectDelegate( VSignalDelegate* inDelegate);

protected:
	// override this to provide a trigger listener when the signal is being triggered.
	virtual	VSignalDelegate*	DoRetainTriggerDelegate();

			void				ConnectRetainedDelegate( VSignalDelegate* inDelegate)	{ ConnectDelegate( inDelegate); if (inDelegate != NULL) inDelegate->Release(); }

			void				PurgeVoidDelegates();
	
	template< class ARG1, class ARG2, class ARG3, class ARG4>
	VIndex _TriggerExcept( const VObject* inExceptTarget, ARG1 v1, ARG2 v2, ARG3 v3, ARG4 v4)
	{
		VIndex nbCalls = 0;
		
		if (IsEnabled())
		{
			VSignalDelegate* extraDelegate = DoRetainTriggerDelegate();
			if (extraDelegate != NULL)
			{
				if (extraDelegate->GetMessageable() != NULL && !extraDelegate->IsVObject(inExceptTarget))
				{
					VSignalMessageT<ARG1, ARG2, ARG3, ARG4>* msg = new VSignalMessageT<ARG1, ARG2, ARG3, ARG4>(extraDelegate, v1, v2, v3, v4);
					msg->SendOrPostTo(extraDelegate->GetMessagingContext(), 0, IsAllwaysSynchronous() || IsSynchronousIfSameTask(), IsAllwaysSynchronous());
					msg->Release();
					++nbCalls;
				}
				extraDelegate->Release();
			}

			// the message destination may need the signal mutex for disconnect.
			// that's why we copy the list and free the mutex
			fMutex.Lock();
			VSignalDelegates delegates( fDelegates);
			fMutex.Unlock();

			for( VSignalDelegates::iterator i = delegates.begin() ; i != delegates.end() ; ++i)
			{
				if ((*i)->GetMessageable() != NULL && !(*i)->IsVObject(inExceptTarget))
				{
					VSignalMessageT<ARG1, ARG2, ARG3, ARG4> *msg = new VSignalMessageT<ARG1, ARG2, ARG3, ARG4>( *i, v1, v2, v3, v4);
					
					// use PostToAndWaitExecutingMessages to be able to process messages whil waiting for response
					if (IsAllwaysSynchronous() && (*i)->GetMessagingContext()->GetMessagingTask() != VTask::GetCurrentID())
						msg->PostToAndWaitExecutingMessages( (*i)->GetMessageable());
					else
						msg->SendOrPostTo((*i)->GetMessagingContext(), 0, IsAllwaysSynchronous() || IsSynchronousIfSameTask(), IsAllwaysSynchronous());
					msg->Release();
					++nbCalls;
				}
			}
		}
		return nbCalls;
	}

private:
	mutable	VCriticalSection						fMutex;	// protects fDelegates
			VSignalDelegates						fDelegates;
			ESignalingMode							fSignalingMode;	// this command needs to be sent asynchronously (like a close window or a quit app)
			bool									fEnabled;

};



/*!
	@class VSignalT_0
	@abstract Signal for zero argument.
*/

class VSignalT_0 : public VSignal
{
public:
	VSignalT_0( ESignalingMode inMode = ESM_SynchonousIfSameTask):VSignal( inMode) {; }

	/*!
		@function Connect
		@abstract Connect a signal listener.
		@parameter inObject messageable signal listener
		@parameter inMemberFunction funtion to be called
	*/
	template<class OBJECT>
	void Connect (OBJECT* inObject, void (OBJECT::*inMemberFunction)())
	{
		ConnectRetainedDelegate(new VSignalDelegateT_0<OBJECT,void (OBJECT::*)()>(inObject, inObject, inMemberFunction));
	}
	
	/*!
		@function Connect
		@abstract Connect a signal listener.
		@parameter inObject signal listener
		@parameter inMessageable messageable interface used for messaging
		@parameter inMemberFunction funtion to be called
	*/
	template<class OBJECT>
	void Connect (OBJECT* inObject, IMessageable* inMessageable, void (OBJECT::*inMemberFunction)())
	{
		ConnectRetainedDelegate(new VSignalDelegateT_0<OBJECT,void (OBJECT::*)()>(inObject, inMessageable, inMemberFunction));
	}
	
	/*!
		@function TriggerExcept
		@abstract Triggers the signal.
		@parameter inExceptTarget calls all signal listeners except this one.
	*/
	VIndex TriggerExcept (const VObject* inExceptTarget = NULL)
	{
		return _TriggerExcept<VoidType,VoidType,VoidType>(inExceptTarget, VoidType(), VoidType(), VoidType(), VoidType());
	}

	/*!
		@function Trigger()
		@abstract Triggers the signal.
	*/
	VIndex Trigger ()
	{
		return _TriggerExcept<VoidType, VoidType, VoidType>(NULL, VoidType(), VoidType(), VoidType(), VoidType());
	}

	/*!
		@function operator()
		@abstract Triggers the signal.
	*/
	void operator() ()
	{
		_TriggerExcept<VoidType, VoidType, VoidType>(NULL, VoidType(), VoidType(), VoidType(), VoidType());
	}
};

#define DEFINE_COMMAND_0_ARG(SignalName)	\
class SignalName: public VSignalT_0 {};


/*!
	@class VSignalT_1
	@abstract Signal for un argument.
*/
template<class ARG1>
class VSignalT_1 : public VSignal
{
	typedef	ARG1	&NotByRefPlease1;
public:
	VSignalT_1( ESignalingMode inMode = ESM_SynchonousIfSameTask):VSignal( inMode) {; }
	
	template<class OBJECT>
	struct delegate
	{
		typedef VSignalDelegateT_1<OBJECT,void (OBJECT::*)(ARG1),ARG1> type;
	};

	template<class OBJECT>
	void Connect (OBJECT* inObject, void (OBJECT::*inMemberFunction)(ARG1))
	{
		ConnectRetainedDelegate(new typename delegate<OBJECT>::type(inObject, inObject, inMemberFunction));
	}

	template<class OBJECT>
	void Connect (OBJECT* inObject, IMessageable* inMessageable, void (OBJECT::*inMemberFunction)(ARG1))
	{
		ConnectRetainedDelegate(new typename delegate<OBJECT>::type(inObject, inMessageable, inMemberFunction));
	}

	VIndex TriggerExcept (const VObject* inExceptTarget, ARG1 v1)
	{
		return _TriggerExcept<ARG1,VoidType,VoidType>(inExceptTarget, v1, VoidType(), VoidType(), VoidType());
	}

	VIndex Trigger (ARG1 v1)
	{
		return _TriggerExcept<ARG1, VoidType, VoidType>(NULL, v1, VoidType(), VoidType(), VoidType());
	}

	void operator() (ARG1 v1)
	{
		_TriggerExcept<ARG1, VoidType, VoidType>(NULL, v1, VoidType(), VoidType(), VoidType());
	}
};


#define DEFINE_COMMAND_1_ARG(SignalName,ARG1)	\
class SignalName: public VSignalT_1<ARG1> {};



/*!
	@class VSignalT_2
	@abstract Signal for two arguments.
*/
template<class ARG1, class ARG2>
class VSignalT_2: public VSignal
{
	typedef	ARG1	&NotByRefPlease1;
	typedef	ARG2	&NotByRefPlease2;
public:
	VSignalT_2( ESignalingMode inMode = ESM_SynchonousIfSameTask):VSignal( inMode) {; }
	
	template<class OBJECT>
	void Connect (OBJECT* inObject, void (OBJECT::*inMemberFunction)(ARG1,ARG2))\
	{
		ConnectRetainedDelegate(new VSignalDelegateT_2<OBJECT,void (OBJECT::*)(ARG1,ARG2),ARG1,ARG2>(inObject, inObject, inMemberFunction));
	}
	
	template<class OBJECT>
	void Connect (OBJECT* inObject, IMessageable* inMessageable, void (OBJECT::*inMemberFunction)(ARG1,ARG2))
	{
		ConnectRetainedDelegate(new VSignalDelegateT_2<OBJECT,void (OBJECT::*)(ARG1,ARG2),ARG1,ARG2>(inObject, inMessageable, inMemberFunction));
	}
	
	VIndex TriggerExcept (const VObject* inExceptTarget, ARG1 v1, ARG2 v2)
	{
		return _TriggerExcept<ARG1, ARG2, VoidType>(inExceptTarget, v1, v2, VoidType(), VoidType());
	}

	VIndex Trigger (ARG1 v1, ARG2 v2)
	{
		return _TriggerExcept<ARG1, ARG2, VoidType>(NULL, v1, v2, VoidType(), VoidType());
	}
	
	void operator() (ARG1 v1, ARG2 v2)
	{
		_TriggerExcept<ARG1, ARG2, VoidType>(NULL, v1, v2, VoidType(), VoidType());
	}
};

#define DEFINE_COMMAND_2_ARGS(SignalName,ARG1,ARG2) \
class SignalName: public VSignalT_2<ARG1,ARG2> {};


/*!
	@class VSignalT_3
	@abstract Signal for three arguments.
*/
template<class ARG1, class ARG2, class ARG3>
class VSignalT_3 : public VSignal
{
	typedef	ARG1	&NotByRefPlease1;
	typedef	ARG2	&NotByRefPlease2;
	typedef	ARG3	&NotByRefPlease3;
public:
	VSignalT_3( ESignalingMode inMode = ESM_SynchonousIfSameTask):VSignal( inMode) {; }

	template<class OBJECT>
	void Connect (OBJECT* inObject, void (OBJECT::*inMemberFunction)(ARG1,ARG2,ARG3))
	{
		ConnectRetainedDelegate(new VSignalDelegateT_3<OBJECT,void (OBJECT::*)(ARG1,ARG2,ARG3),ARG1,ARG2,ARG3>(inObject, inObject, inMemberFunction));
	}

	template<class OBJECT>
	void Connect (OBJECT* inObject, IMessageable* inMessageable, void (OBJECT::*inMemberFunction)(ARG1,ARG2,ARG3))
	{
		ConnectRetainedDelegate(new VSignalDelegateT_3<OBJECT,void (OBJECT::*)(ARG1,ARG2,ARG3),ARG1,ARG2,ARG3>(inObject, inMessageable, inMemberFunction));
	}
	
	VIndex TriggerExcept (const VObject* inExceptTarget, ARG1 v1, ARG2 v2, ARG3 v3)
	{
		return _TriggerExcept<ARG1, ARG2, ARG3>(inExceptTarget, v1, v2, v3, VoidType());
	}

	VIndex Trigger (ARG1 v1, ARG2 v2, ARG3 v3)
	{
		return _TriggerExcept<ARG1, ARG2, ARG3>(NULL, v1, v2, v3, VoidType());
	}

	void operator() (ARG1 v1, ARG2 v2, ARG3 v3)
	{
		_TriggerExcept<ARG1, ARG2, ARG3>(NULL, v1, v2, v3, VoidType());
	}
};


#define DEFINE_COMMAND_3_ARGS(SignalName,ARG1,ARG2,ARG3) \
class SignalName : public VSignalT_3<ARG1,ARG2,ARG3> {};



/*!
	@class VSignalT_4
	@abstract Signal for four arguments.
*/
template<class ARG1, class ARG2, class ARG3, class ARG4>
class VSignalT_4 : public VSignal
{
	typedef	ARG1	&NotByRefPlease1;
	typedef	ARG2	&NotByRefPlease2;
	typedef	ARG3	&NotByRefPlease3;
	typedef	ARG4	&NotByRefPlease4;
public:
	VSignalT_4( ESignalingMode inMode = ESM_SynchonousIfSameTask):VSignal( inMode) {; }

	template<class OBJECT>
	void Connect (OBJECT* inObject, void (OBJECT::*inMemberFunction)(ARG1,ARG2,ARG3,ARG4))
	{
		ConnectRetainedDelegate(new VSignalDelegateT_4<OBJECT,void (OBJECT::*)(ARG1,ARG2,ARG3,ARG4),ARG1,ARG2,ARG3,ARG4>(inObject, inObject, inMemberFunction));
	}

	template<class OBJECT>
	void Connect (OBJECT* inObject, IMessageable* inMessageable, void (OBJECT::*inMemberFunction)(ARG1,ARG2,ARG3,ARG4))
	{
		ConnectRetainedDelegate(new VSignalDelegateT_4<OBJECT,void (OBJECT::*)(ARG1,ARG2,ARG3,ARG4),ARG1,ARG2,ARG3,ARG4>(inObject, inMessageable, inMemberFunction));
	}
	
	VIndex TriggerExcept (const VObject* inExceptTarget, ARG1 v1, ARG2 v2, ARG3 v3, ARG3 v4)
	{
		return _TriggerExcept<ARG1, ARG2, ARG3, ARG4>(inExceptTarget, v1, v2, v3, v4);
	}

	VIndex Trigger (ARG1 v1, ARG2 v2, ARG3 v3, ARG4 v4)
	{
		return _TriggerExcept<ARG1, ARG2, ARG3>(NULL, v1, v2, v3, v4);
	}

	void operator() (ARG1 v1, ARG2 v2, ARG3 v3, ARG4 v4)
	{
		_TriggerExcept<ARG1, ARG2, ARG3>(NULL, v1, v2, v3, v4);
	}
};


#define DEFINE_COMMAND_4_ARGS(SignalName,ARG1,ARG2,ARG3,ARG4) \
class SignalName : public VSignalT_4<ARG1,ARG2,ARG3,ARG4> {};

END_TOOLBOX_NAMESPACE

#endif
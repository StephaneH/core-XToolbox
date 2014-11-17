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
#ifndef __VMessageCall__
#define __VMessageCall__

#include "Kernel/Sources/VMessage.h"
#include "Kernel/Sources/VTask.h"

BEGIN_TOOLBOX_NAMESPACE


template<class MSG, class RESULT, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6, class ARG7, class ARG8>
struct MessageCaller
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5, inMessage->fArg6, inMessage->fArg7, inMessage->fArg8);
}
};

template<class MSG, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6, class ARG7, class ARG8>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5, inMessage->fArg6, inMessage->fArg7, inMessage->fArg8);
}
};

template<class MSG, class RESULT, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6>
struct MessageCaller<MSG,RESULT, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5, inMessage->fArg6);
}
};

template<class MSG, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5, inMessage->fArg6);
}
};

template<class MSG, class RESULT, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5>
struct MessageCaller<MSG,RESULT, ARG1, ARG2, ARG3, ARG4, ARG5, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5);
}
};

template<class MSG, class ARG1, class ARG2, class ARG3, class ARG4, class ARG5>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, ARG3, ARG4, ARG5, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4, inMessage->fArg5);
}
};

template<class MSG, class RESULT, class ARG1, class ARG2, class ARG3, class ARG4>
struct MessageCaller<MSG,RESULT, ARG1, ARG2, ARG3, ARG4, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4);
}
};

template<class MSG, class ARG1, class ARG2, class ARG3, class ARG4>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, ARG3, ARG4, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3, inMessage->fArg4);
}
};

template<class MSG, class RESULT, class ARG1, class ARG2, class ARG3>
struct MessageCaller<MSG,RESULT, ARG1, ARG2, ARG3, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3);
}
};

template<class MSG, class ARG1, class ARG2, class ARG3>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, ARG3, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2, inMessage->fArg3);
}
};

template<class MSG, class RESULT, class ARG1, class ARG2>
struct MessageCaller<MSG,RESULT, ARG1, ARG2, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2);
}
};

template<class MSG, class ARG1, class ARG2>
struct MessageCaller<MSG,VoidType, ARG1, ARG2, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1, inMessage->fArg2);
}
};

template<class MSG, class RESULT, class ARG1>
struct MessageCaller<MSG,RESULT, ARG1, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1);
}
};

template<class MSG, class ARG1>
struct MessageCaller<MSG,VoidType, ARG1, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)( inMessage->fArg1);
}
};

template<class MSG, class RESULT>
struct MessageCaller<MSG,RESULT, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	inMessage->fResult = (inMessage->fObject->*inMessage->fMethod)();
}
};

template<class MSG>
struct MessageCaller<MSG,VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType, VoidType>
{
static void Call( MSG *inMessage)
{
	(inMessage->fObject->*inMessage->fMethod)();
}
};



template<class OBJECT, class METHOD, class RESULT_TYPE, class ARG1_TYPE = VoidType, class ARG2_TYPE = VoidType, class ARG3_TYPE = VoidType, class ARG4_TYPE = VoidType, class ARG5_TYPE = VoidType, class ARG6_TYPE = VoidType, class ARG7_TYPE = VoidType, class ARG8_TYPE = VoidType>
class VMethodCallMessage : public VMessage
{
public:
	VMethodCallMessage( OBJECT *inObject, METHOD inMethod, ARG1_TYPE inArg1, ARG2_TYPE inArg2, ARG3_TYPE inArg3, ARG4_TYPE inArg4, ARG5_TYPE inArg5, ARG6_TYPE inArg6, ARG7_TYPE inArg7, ARG8_TYPE inArg8)
		: fObject( inObject)
		, fMethod( inMethod)
		, fArg1( inArg1)
		, fArg2( inArg2)
		, fArg3( inArg3)
		, fArg4( inArg4)
		, fArg5( inArg5)
		, fArg6( inArg6)
		, fArg7( inArg7)
		, fArg8( inArg8)
		{
		}
		
	virtual	void DoExecute()
	{
		MessageCaller<VMethodCallMessage, RESULT_TYPE, ARG1_TYPE, ARG2_TYPE, ARG3_TYPE, ARG4_TYPE, ARG5_TYPE, ARG6_TYPE, ARG7_TYPE, ARG8_TYPE>::Call( this);
	}

	OBJECT*				fObject;
	METHOD				fMethod;
	RESULT_TYPE			fResult;
	ARG1_TYPE			fArg1;
	ARG2_TYPE			fArg2;
	ARG3_TYPE			fArg3;
	ARG4_TYPE			fArg4;
	ARG5_TYPE			fArg5;
	ARG6_TYPE			fArg6;
	ARG7_TYPE			fArg7;
	ARG8_TYPE			fArg8;

	class VRefPtr
	{
	public:
		VRefPtr( OBJECT *inObject, METHOD inMethod, ARG1_TYPE inArg1, ARG2_TYPE inArg2, ARG3_TYPE inArg3, ARG4_TYPE inArg4, ARG5_TYPE inArg5, ARG6_TYPE inArg6, ARG7_TYPE inArg7, ARG8_TYPE inArg8)
		: fMessage( new VMethodCallMessage( inObject, inMethod, inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7, inArg8))
		{
		}
		
		~VRefPtr()
		{
			ReleaseRefCountable( &fMessage);
		}
		
		VMethodCallMessage*	operator->()	{ return fMessage;}
	
		VMethodCallMessage	*fMessage;
	};
	
		
private:
	~VMethodCallMessage() {}
};


template<class RESULT, class OBJECT, class METHOD>
class VMessageCallFunctor
{
public:
	VMessageCallFunctor( OBJECT *inObject, METHOD inMethod, IMessageable *inTarget):fObject( inObject), fMethod( inMethod), fTarget( inTarget) {}

	OBJECT*				fObject;
	METHOD				fMethod;
	IMessageable*		fTarget;

	RESULT operator()()
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)();
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, VoidType, VoidType, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1>
	RESULT operator()( ARG1 inArg1)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, VoidType, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2, class ARG3>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2, inArg3);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, ARG3, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, ARG3, ARG4>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, ARG3, ARG4, ARG5>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5, ARG6 inArg6)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5, inArg6);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6, class ARG7, class ARG8>
	RESULT operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5, ARG6 inArg6, ARG7 inArg7, ARG8 inArg8)
	{
		if (fTarget == NULL)
		{
			return (fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7, inArg8);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, RESULT, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7, inArg8);
			msg->PostToAndWaitExecutingMessages( fTarget);
			return msg->fResult;
		}
	}

};


template<class OBJECT, class METHOD>
class VMessageCallFunctor<void,OBJECT,METHOD>
{
public:
	VMessageCallFunctor( OBJECT *inObject, METHOD inMethod, IMessageable *inTarget):fObject( inObject), fMethod( inMethod), fTarget( inTarget) {}
	
	OBJECT*				fObject;
	METHOD				fMethod;
	IMessageable*		fTarget;

	void operator()()
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)();
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, VoidType, VoidType, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1>
	void operator()(ARG1 inArg1)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, VoidType, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2>
	void operator()( ARG1 inArg1, ARG2 inArg2)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, VoidType, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, VoidType(), VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2, class ARG3>
	void operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2, inArg3);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, ARG3, VoidType>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, VoidType(), VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4>
	void operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, ARG3, ARG4>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, VoidType(), VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5>
	void operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, ARG3, ARG4, ARG5>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, VoidType(), VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6>
	void operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5, ARG6 inArg6)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5, inArg6);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, VoidType(), VoidType());
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}

	template<class ARG1, class ARG2, class ARG3, class ARG4, class ARG5, class ARG6, class ARG7, class ARG8>
	void operator()( ARG1 inArg1, ARG2 inArg2, ARG3 inArg3, ARG4 inArg4, ARG5 inArg5, ARG6 inArg6, ARG7 inArg7, ARG8 inArg8)
	{
		if (fTarget == NULL)
		{
			(fObject->*fMethod)( inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7, inArg8);
		}
		else
		{
			typename VMethodCallMessage<OBJECT, METHOD, VoidType, ARG1, ARG2, ARG3, ARG4>::VRefPtr msg( fObject, fMethod, inArg1, inArg2, inArg3, inArg4, inArg5, inArg6, inArg7, inArg8);
			msg->PostToAndWaitExecutingMessages( fTarget);
		}
	}
};


template<class RESULT=void>
struct VMessageCall
{
	template<class OBJECT, class METHOD>
	static VMessageCallFunctor<RESULT, OBJECT, METHOD > Create( OBJECT *inObject, METHOD inMethod, IMessageable *inTarget)
	{
		IMessageable *target = (inTarget->GetMessagingTask() == VTask::GetCurrentID()) ? NULL : inTarget;
		return VMessageCallFunctor<RESULT, OBJECT, METHOD >( inObject, inMethod, target);
	}

	template<class OBJECT, class METHOD>
	static VMessageCallFunctor<RESULT, OBJECT, METHOD > Create( OBJECT *inObject, METHOD inMethod)
	{
		IMessageable *target = (inObject->GetMessagingTask() == VTask::GetCurrentID()) ? NULL : inObject;
		return VMessageCallFunctor<RESULT, OBJECT, METHOD >( inObject, inMethod, target);
	}
};


END_TOOLBOX_NAMESPACE

#endif
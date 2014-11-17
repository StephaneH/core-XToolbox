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
#ifndef __IIdleable__
#define __IIdleable__

#include "Kernel/Sources/VMessage.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VTask;

/*!
	@class	IIdleable
	@abstract	Abstract classes to handle idle callback.
	@discussion
		You only need to override DoIdle().
*/

class XTOOLBOX_API IIdleable
{ 
friend class XIdleMessage;
public:
					IIdleable():fRegisteredTask( NULL),fIdling( false)		{;}
	virtual			~IIdleable();
	
	// Ask idling in specified task
			void	StartIdling( VTask *inTask);
	// ask idling in current task
			void	StartIdling();
	// stop idling
			void	StopIdling();

			void	Idle();

			bool	IsIdling() const			{ return fIdling; }
	
	// Task support
	VTask*	GetRegisteredTask() const			{ return fRegisteredTask; }
	void	SetRegisteredTask(VTask* inTask)	{ fRegisteredTask = inTask; }
	
protected:
	// Override to implement idle callbacks
	virtual	void	DoIdle();

private:
			VTask*	fRegisteredTask;
			bool	fIdling;
};


class XIdleMessage : public VMessage
{ 
public:
			XIdleMessage (IIdleable *inIdleable);
	virtual	~XIdleMessage ();

protected:
	IIdleable*	fIdleable;
	
	virtual void DoExecute ();
};


END_TOOLBOX_NAMESPACE

#endif
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
#ifndef __VProgressIndicator__
#define __VProgressIndicator__


#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VArrayValue.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VUUID.h"
#include "Kernel/Sources/VJSONValue.h"

BEGIN_TOOLBOX_NAMESPACE

class VValueBag;

class XTOOLBOX_API VReadWriteActivityIndicator : public VObject
{
	public:
	VReadWriteActivityIndicator(){;}
	virtual ~VReadWriteActivityIndicator(){;}
	
	virtual void SetActivityRead(bool State, sLONG Delay) = 0;  // if Delay == -1 then unlimited
	virtual void SetActivityWrite(bool State, sLONG Delay) = 0;  // if Delay == -1 then unlimited
	
};

const uLONG  kDelayToCallDoProgress = 500; // milliseconds



class IProgressInfoCollector;

class XTOOLBOX_API VProgressIndicatorInfo : public VObject
{
	public:
	
	enum UpdateFlags
	{
		kMessage				=1,
		kValue					=2,
		kMax					=4,
		kStartTime				=8,
		kCanInterrupt			=16,
		kExtraInfoBagVersion	=32,
		kExtraInfoBag			=64,
		kAll					=1024
	};

	VError	SaveToBag( VValueBag& ioBag, VString& outKind,const VProgressIndicatorInfo* inPrevious=NULL) const;
	VError	LoadFromBag( const VValueBag& inBag);

	VError GetJSONObject(VJSONObject& outObject)const;

	virtual ~VProgressIndicatorInfo();

	VProgressIndicatorInfo()
	{
		fExtraInfoBag=NULL;
		fActivityInfoCollector = NULL;
		fActivityInfo = NULL;
		fIsRemote=false;
		if (VTask::GetCurrent())
		{
			fTaskID = VTask::GetCurrent()->GetID();
		}
		fUUID.Regenerate();
		Reset();
	}
	VProgressIndicatorInfo(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanInterrupt)
	{
		fIsRemote=false;
		fMessage=inMessage;
		fValue=inValue;
		fMax=inMax;
		fActivityInfoCollector = NULL;
		fActivityInfo = NULL;
		fCanInterrupt=inCanInterrupt;
		fStartTime= XBOX::VSystem::GetCurrentTime();
		fExtraInfoBag=NULL;
		if (VTask::GetCurrent())
		{
			fTaskID = VTask::GetCurrent()->GetID();
		}
		fUUID.Regenerate();
		fActivityInfoCollector = NULL;
	};
	VProgressIndicatorInfo(const VProgressIndicatorInfo &inInfo);
	
	VProgressIndicatorInfo&		operator = ( const VProgressIndicatorInfo* inOther);
	VProgressIndicatorInfo&		operator = ( const VProgressIndicatorInfo& inOther);
	bool						operator ==( const VProgressIndicatorInfo& inInfo) const;
	bool						operator !=( const VProgressIndicatorInfo& inInfo) const;
	
	void		GetMessage(VString &outMess)const;
	sLONG8		GetValue()const								{return fValue;}
	sLONG8		GetMax()const								{return fMax;}
	void		Reset();
	void		SetMax(sLONG8 inMax)						{fMax=inMax;}
	void		SetValue(sLONG8 inValue)					{fValue=inValue;}
	void		SetMessage(const VString& inMessage)		{fMessage=inMessage;}
	void		Increment(sLONG8 inInc=1)					{fValue+=inInc;}
	void		Decrement(sLONG8 inDec=1)					{fValue-=inDec;}
	void		GetStartTime(VTime& outTime)				const;
	void		GetDuration(VDuration& outDuration)			const;
	uLONG		GetStamp()									const{return fStartTime;} 
	VTaskID		GetTaskID()									const{return fTaskID;}
	VUUID		GetUUID()									const{return fUUID;}
	bool		GetCanInterrupt()							const{return fCanInterrupt;}
	void						SetProgressInfoCollector(IProgressInfoCollector* inCollector){ fActivityInfoCollector = inCollector; }
	IProgressInfoCollector*		GetInfoCollector()			const{return fActivityInfoCollector;}
	const XBOX::VJSONObject*	GetCollectedInfo()			const{return fActivityInfo;}
	void						SetCollectedInfo(XBOX::VJSONObject* inInfo){XBOX::CopyRefCountable(&fActivityInfo,inInfo);}

	void		SetCanInterrupt(bool inCanInterrupt)		{fCanInterrupt=inCanInterrupt;}
	void		Set(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanIterrupt,bool inResetStamp=true,IProgressInfoCollector* inCollector = NULL);
	const VValueBag*	RetainExtraInfoBag();
	
	void		SetAttribute(const VString& inAttrName,const VValueSingle& inVal);
	void		RemoveAttribute(const VString& inAttrName);

	void		SetRemote(bool inSet)						{fIsRemote=inSet;}
	bool		IsRemote()									const{return fIsRemote;}

	void		UpdateFromRemoteInfo(const VProgressIndicatorInfo& inFrom,uLONG inFlags);

private:
	VTaskID		fTaskID;	//Id of the task that created the object
	VUUID		fUUID;		//Id of the present instance
	VString		fMessage;
	sLONG8		fValue;
	sLONG8		fMax;
	
	uLONG		fStartTime; // debut session et stamp
	bool		fCanInterrupt;
	bool		fIsRemote;
	VValueBag*	fExtraInfoBag;
	uLONG		fExtraInfoBagVersion;
	IProgressInfoCollector* fActivityInfoCollector; //Optional helper that adds instrumentation info
	XBOX::VJSONObject*		fActivityInfo;
};

inline bool	VProgressIndicatorInfo::operator !=( const VProgressIndicatorInfo& inInfo) const						
{ 
	return !((*this)==inInfo);
};
inline bool	VProgressIndicatorInfo::operator ==( const VProgressIndicatorInfo& inInfo) const						
{ 
	if(inInfo.fValue == fValue)
	{
		if(inInfo.fMax == fMax)
		{
			if(fStartTime==inInfo.fStartTime)
			{
				if(fCanInterrupt==inInfo.fCanInterrupt)
					return inInfo.fMessage==fMessage;
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
};
inline void VProgressIndicatorInfo::Set(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanIterrupt,bool inResetStamp,XBOX::IProgressInfoCollector* inCollector)
{
	fMessage=inMessage;
	fValue=inValue;
	fMax=inMax;
	fCanInterrupt=inCanIterrupt;
	if(inResetStamp)
	{
		fStartTime= XBOX::VSystem::GetCurrentTime();
	}
	fActivityInfoCollector = inCollector;

#if WITH_RTM_DETAILLED_ACTIVITY_INFO
	//When GetJSONObject() is called, it will merge the activity info properties with the returned object.
	//The current method (i.e. Set()) is used to create progress sub-sessions. If we don't reset the current 
	//activity info and "inherit" the parent session's, the object returned by GetJSONObject() will be bloated
	//with repeated properties
	XBOX::ReleaseRefCountable(&fActivityInfo);
#endif
};

inline void		VProgressIndicatorInfo::GetStartTime(VTime& outTime)			const
{
	//fStartTime is obtained by XBOX::VSystem::GetCurrentTime()
	//VTime does not have the same time so we must convert
	uLONG now = XBOX::VSystem::GetCurrentTime();
	XBOX::VDuration elapsed(now - fStartTime);
	XBOX::VTime::Now(outTime);
	outTime.Substract(elapsed);
}

inline void		VProgressIndicatorInfo::GetDuration(VDuration& outDuration)			const
{
	uLONG now = XBOX::VSystem::GetCurrentTime();
	outDuration.FromNbMilliseconds(now - fStartTime);
}

class XTOOLBOX_API VProgressSessionInfo : public VObject
{
	public:
	
	VProgressSessionInfo()
	{
		fSessionCount=0;
		fInfoCount=0;
		fPercentDone=0;
		fChangesVersion=0;
	}
	virtual ~VProgressSessionInfo()
	{
		
	}
	VProgressSessionInfo&		operator = ( const VProgressSessionInfo& inOther);

	void									SetInfo(sLONG inSessionVersion,sLONG inSessionCount,SmallReal inPercent,const VProgressIndicatorInfo* in0,const VProgressIndicatorInfo* in1=NULL,const VProgressIndicatorInfo* in2=NULL);
	bool									operator ==( const VProgressSessionInfo& inInfo) const;
	bool									operator !=( const VProgressSessionInfo& inInfo) const;
	VProgressIndicatorInfo*					GetCurrent();
	
	sLONG									GetSessionCount()const		{return fSessionCount;}
	sLONG									GetInfoCount()const			{return fInfoCount;}
	SmallReal								GetPercentDone()const		{return fPercentDone;}
	void									SetChanged();
	sLONG									GetChangesVersion()const		{return fChangesVersion;}
	
public:
	
	VProgressIndicatorInfo					fProgressArray[3];
	sLONG									fSessionCount;
	sLONG									fInfoCount;
	SmallReal								fPercentDone;
	
	uLONG									fChangesVersion;
};
inline void	VProgressSessionInfo::SetChanged()
{
	fChangesVersion++;
}
inline VProgressIndicatorInfo* VProgressSessionInfo::GetCurrent()
{
	if(fSessionCount==0)
		return NULL;
	if(fSessionCount==1)
		return &fProgressArray[0];
	else if(fSessionCount==2)
		return &fProgressArray[1];
	else 
		return &fProgressArray[2];
} ;

inline bool VProgressSessionInfo::operator !=( const VProgressSessionInfo& inInfo) const
{
	return !((*this)==inInfo);
}
inline bool VProgressSessionInfo::operator ==( const VProgressSessionInfo& inInfo) const
{
	return inInfo.fChangesVersion==fChangesVersion;
}
inline void VProgressSessionInfo::SetInfo(sLONG inChangesVersion,sLONG inSessionCount,SmallReal inPercent,const VProgressIndicatorInfo* in0,const VProgressIndicatorInfo* in1,const VProgressIndicatorInfo* in2)
{
	inSessionCount>=3 ? fInfoCount=3 : fInfoCount=inSessionCount;
	fSessionCount=inSessionCount;
	fPercentDone=inPercent;
	fProgressArray[0]=in0;
	fProgressArray[1]=in1;
	fProgressArray[2]=in2;
	fChangesVersion=inChangesVersion;
};

typedef std::deque < VProgressIndicatorInfo>			_ProgressSessionInfoStack ;
typedef _ProgressSessionInfoStack::const_iterator		_ProgressSessionInfoStack_Const_Iterrator ;
typedef _ProgressSessionInfoStack::iterator				_ProgressSessionInfoStack_Iterrator ;


class XTOOLBOX_API VRemoteProgressSessionInfo : public VObject
{
	public:
	
	enum UpdateFlags
	{
		kSessionCount			=1,
		kChangesVersion			=2,
		kSessionVersion			=4,
		kMasterSessionVersion	=8,
		kUserAbort				=16,
		kCanInterrupt			=32,
		kUpdateCurrent			=64,
		kUpdateSession			=128	
	};

	VError	SaveToBag( VValueBag& ioBag, VString& outKind,const VRemoteProgressSessionInfo* inPrevious=NULL) const;
	VError	LoadFromBag( const VValueBag& inBag);
		
	VRemoteProgressSessionInfo();
	VRemoteProgressSessionInfo(const VRemoteProgressSessionInfo* inFrom);
	virtual ~VRemoteProgressSessionInfo()
	{
		
	}
	VRemoteProgressSessionInfo&				operator = ( const VRemoteProgressSessionInfo& inOther);

	void									FromProgressInfo(const VProgressIndicatorInfo* inCurrent,const _ProgressSessionInfoStack* inStack,sLONG nbSession,uLONG inChangesVersion,uLONG inVersion,uLONG inMasterVersion);
	bool									operator ==( const VRemoteProgressSessionInfo& inInfo) const;
	bool									operator !=( const VRemoteProgressSessionInfo& inInfo) const;
	const VProgressIndicatorInfo*			GetCurrent() const;
	
	sLONG									GetSessionCount()const			{return fSessionCount;}
	
	void									SetChanged();
	sLONG									GetChangesVersion()const		{return fChangesVersion;}
	void									Reset();
	void									SetUserAbort(bool inUserAbort){fUserAbort=inUserAbort;fFlags|=kUserAbort;};
	void									SetCanInterrupt(bool inCanInterrupt){fCanInterrupt=inCanInterrupt;fFlags|=kCanInterrupt;};
public:
	sLONG											fSessionCount;
	_ProgressSessionInfoStack						fSessionStack;
	
	uLONG											fChangesVersion;
	uLONG											fMasterSessionVersion;
	uLONG											fSessionVersion;
	bool											fUserAbort;
	bool											fCanInterrupt;
	sLONG											fFlags;
	uLONG											fCurrentSessionFlags;
};


class VProgressIndicator;

/**
 * \brief This interface is to be implemented by classes to enable a VProgressIndicator to provide more specific activity progress information.
 * \details VProgressIndicator can be queried to retrieve simple progress-related informations about an activity that it tracks. In order to return
 * additional details about the progress of this activity, it relies on a provider which must implement this interface.
 */
class IProgressInfoCollector
{
public:

#if WITH_RTM_DETAILLED_ACTIVITY_INFO
	/**
	* \brief Collects additionnal specific information about a progress-tracked activity
	* \param inIndicator the progress indicator which is being queried for progress information
	* \param inForRootSession true if the info is being collected for @inIndicator's first session
	*/
	virtual	VJSONObject*	CollectInfo(VProgressIndicator *inIndicator,bool inForRootSession) = 0;
#else
	/**
	 * \brief Collects additionnal specific information about a progress-tracked activity
	 * \param inIndicator the progress indicator which is being queried for progress information
	 */
	virtual	VJSONObject*	CollectInfo(VProgressIndicator *inIndicator) = 0;
#endif

	/**
	 * \brief Request that the session string be loaded
	 * @param ioMessage where the string will be stored
	 * @return true if loading went well, false otherwise
	 */
	virtual	bool			LoadMessageString(XBOX::VString& ioMessage)= 0;
};


class VRefCountedSessionInfo : public VObject, public IRefCountable
{
public:
	typedef std::vector<XBOX::VProgressIndicatorInfo*> VectorOfProgressInfoPtr;
	VRefCountedSessionInfo(std::vector<XBOX::VProgressIndicatorInfo*> & inVector);
	virtual ~VRefCountedSessionInfo();

	const std::vector<XBOX::VProgressIndicatorInfo*>& GetSessionInfo()const{return *fRefCountedArray;}
	
	//std::vector<XBOX::VProgressIndicatorInfo*>& GetSessionInfo()const{return fRefCountedArray;}

private:
	VRefCountedSessionInfo();
	VRefCountedSessionInfo(const VRefCountedSessionInfo&);

private:
	std::vector<XBOX::VProgressIndicatorInfo*> *fRefCountedArray;
};

class XTOOLBOX_API VProgressIndicator : public VObject, public IRefCountable
{
public:
						VProgressIndicator ();
						VProgressIndicator(const VUUID& inUUID,sLONG inVersion);
	virtual				~VProgressIndicator ();

		void			WriteInfoForRemote(VStream& inStream);
		void			WriteInfoForRemote(VValueBag& inBag,VString& outKind);


	// Title support
		void			SetTitle (const VString& inTitle) { fTitle.FromString(inTitle); };
		const VString&	GetTitle () const { return fTitle; };

	// Progress support
	bool					Progress (sLONG8 inCurValue);
	bool					Increment(sLONG8 inInc=1);
	bool					Decrement(sLONG8 inDec=1);
	sLONG8					GetCurrentValue () const;

	void					SetMaxValue (sLONG8 inMaxValue);
	sLONG8					GetMaxValue () const ;
	bool					IsEndKnown () const { return (GetMaxValue() != -1); };
	sLONG					GetSessionCount()const;
	
	// Session support

	/**
	 * \brief Starts a new progress tracking session with the default interruptability and the default prgress info provider
	 * \param inMaxValue the value to reach for 100% completion
	 * \param inMessage the progress tracking message
	 * \param inCanInterrupt wether the tracked activity can be interrupted
	 * \param inCollector  the info provider for this session
	 * \details @c inCollector will be used for this session. If not defined:
	 * - if *the* parent session has a provider defined, then it is used.
	 * - if *the* parent session has no provider defined, then the default one is used.
	 * - if no parent session is defined then then the default provider is used.
	 * \see SetDefaultInfoCollector()
	 */
	void							BeginSession (sLONG8 inMaxValue, const VString& inMessage);
	
	void							BeginSession(sLONG8 inMaxValue, const VString& inMessage, IProgressInfoCollector* inCollector);


	/**
	 * \brief Starts a new progress tracking session
	 * \param inMaxValue the value to reach for 100% completion
	 * \param inCurrentValue the current value to start the session with (0 by default)
	 * \param inMessage the progress tracking message
	 * \param inCanInterrupt wether the tracked activity can be interrupted
	 * \param inCollector  the info provider for this session
	 * \details @c inCollector will be used for this session. If not defined:
	 * - if *the* parent session has a provider defined, then it is used.
	 * - if *the* parent session has no provider defined, then the default one is used.
	 * - if no parent session is defined then then the default provider is used.
	 * \see SetDefaultInfoCollector()
	 */
	void						BeginSession (sLONG8 inMaxValue,sLONG8 inCurrentValue, const VString& inMessage,bool inCanInterrupt,IProgressInfoCollector* inCollector = NULL);
	void						BeginSession (sLONG8 inMaxValue, const VString& inMessage,bool inCanInterrupt,IProgressInfoCollector* inCollector = NULL);
	void						BeginUndeterminedSession ( const VString& inMessage);
	
	
	/**
	 * \brief Starts an unbound session
	 * \param inMessage the progress tracking message
	 * \param inCanInterrupt wether the tracked activity can be interrupted
	 * \param inCollector  the info provider for this session
	 * \see BeginSession() for rules regarding how @c inCollector is used
	 * \see SetDefaultInfoCollector()
	 */
				void					BeginUndeterminedSession ( const VString& inMessage,bool inCanInterrupt,IProgressInfoCollector* inCollector = NULL);

				void					EndSession ();

				void					Reset(const VString& inMessage,sLONG8 inMaxValue,sLONG8 inValue=0);
				
				void					GetMessage(VString &outMess)const;
				void					SetMessage(const VString& inMessage);
				void					SetMessageAndIncrementValue (const VString& inMessage,sLONG8 inIncrement=1);
				void					SetMessageAndValue (const VString& inMessage,sLONG8 inValue);
				void					SetAttribute (const VString& inAttr,const VValueSingle& inVal);
				void					RemoveAttribute (const VString& inAttr);
	
	// Accessors
				void					SetCanInterrupt (bool inSet,bool inCurrentSessionOnly=false);
				bool					GetCanInterrupt () const { return fCanInterrupt; };
				
				void					SetInterruptWillRevert (bool inSet) { fCanRevert = inSet; };
				bool					GetInterruptWillRevert () const { return fCanRevert; };
				
				bool					GetProgressInfo(VProgressSessionInfo& outInfo);
				bool					GetProgressInfo(VRemoteProgressSessionInfo& outInfo,VRemoteProgressSessionInfo* inPrevious)const;
	
	/**
	 * \brief Retrieves basic session info
	 * \param inSessionId the session which info is to be returned
	 * \param outInfo the status info returned
	 * \return true if the requested session was found and false otherwise
	 */
	bool								GetProgressInfo(const XBOX::VUUID& inSessionId,VProgressIndicatorInfo& outInfo)const;
	
	void								SetProgressInfo(const VProgressIndicatorInfo& inInfo);
	SmallReal							ComputePercentDone () const;
	uLONG								GetSessionVersion();
	void								EnableGUI(bool inEnableState){fGUIEnable=inEnableState;}
	
	void								UserAbort();
	bool								IsInterrupted();
	const VUUID&						GetUUID()const{return fUUID;}

	void								UpdateFromRemoteInfo(const VRemoteProgressSessionInfo& inInfo);

	void								SetUserInfo(const VString& inUserInfo) { fUserInfo = inUserInfo; }
	const VString&						GetUserInfo() const	{ return fUserInfo; }

	virtual VError						SaveInfoToBag(VValueBag& outBag);
	virtual bool						IsManagerValid() const	{ return true; }

	/**
	 * \brief Sets the default activity-related information provider instance
	 * \details This method allows you to set the active default ifnormation provider instance.
	 * \see BeginSession()
	 */
	void								SetDefaultInfoCollector( IProgressInfoCollector *inCollector)	{ fDefaultInfoCollector = inCollector;}

	/**
	 * Sets or resets the progress info collector for the current session
	 * \param inCollector the new collector to use or NULL to reset the current session's collector
	 * \param outOldCollector if not NULL on return this parameter will contain a refererence to the session's previous collector. May be NULL on return, indicating that
	 * no previous collector was set.
	 */
	void								SetProgressInfoCollector(IProgressInfoCollector* inCollector, IProgressInfoCollector** outOldCollector = NULL);

	/**
	 * \brief Request the instance to update progress information.
	 * \details This method will instruct the isntance to update progress-information. The update itself will be performed
	 * when the owner of the instance will log progress advance with Incrment/Decrement/Progress methods.
	 * Sessions infos can be retrieved with @c RetainSessionsInfos().
	 * \notice It is safe to call it from any thread.
	 */
	void								UpdateSessionsInfos();

	/**
	 * \brief returns the last updated progress informations for this progress indicator
	 * \details returns the last updated progress information since @c UpdateSessionsInfos() was called.
	 * Depending on the progress indicator owner's pace (at logging progress that is), @c RetainSessionsInfos() may
	 * return slightly outdated information.
	 * \returns NULL if no informations are available
	 * \returns the last  last updated progress informations available
	 */
	VRefCountedSessionInfo*				RetainSessionsInfos()const;
	

protected:
	
	//Checks if some progress info updates have been requested, if so and fetches the info and notifies requesters
			void									_CheckCollectInfo();
	
	virtual	void									DoBeginSession(sLONG /*inSessionNumber*/){;}
	virtual	void									DoEndSession(sLONG /*inSessionNumber*/){;}

	virtual	bool									DoProgress ();	// Return false if you want to stop

			SmallReal								_ComputePercentDone () const;
			void									Lock()const;
			void									Unlock()const;
			void									_CloseRemoteSession();
			void									_CleanRemoteSession();
			void									_EndSession();
			void									_Init ();
		#if VERSIONDEBUG
			void									_CheckCallingTaskIdIsOwner()const;
		#endif //_DEBUG

protected:
	VUUID											fUUID;

	mutable	VCriticalSection						fCrit;
	
			_ProgressSessionInfoStack				fSessionStack;
	mutable	VProgressIndicatorInfo					fCurrent;
			sLONG									fSessionCount;

			sLONG									fRemoteSessionOpen;
			sLONG									fRemoteSessionVersion;
			sLONG									fSaveSessionCount;
			
			bool									fCanInterrupt;	// Value returned from Progress is handled by caller
			bool									fCanRevert;		// Interrupt will return to initial state
			bool									fGUIEnable;
			
			VString									fTitle;
			uLONG									fSessionVersion;			// unique id, change a chaque ouverture de la session principal	
			sLONG									fUserAbort;
			
			uLONG									fChangesVersion;

			VString									fUserInfo;

			IProgressInfoCollector*					fDefaultInfoCollector;

			
			
			mutable	VCriticalSection				fCollectedInfoMutex; //protected fLastCollectedSessionInfo
			VRefCountedSessionInfo					*fLastCollectedSessionInfo;

			///Array where session related info will be stored -- Not owned by this class
			VSyncEvent*								fCollectedInfoEvent;
			sLONG									fCollectedInfoStamp;
			sLONG									fNeedCollectInfoStamp;
			

			//to ensure SetMessage()/Progress()/Increment() etc are called from the proper thread
			sLONG									fDbgOwnerTaskId;
};

inline void	VProgressIndicator::UserAbort()
{
	XBOX::VInterlocked::Exchange(&fUserAbort,(sLONG)1);
}
inline bool	VProgressIndicator::IsInterrupted()									
{
	return fUserAbort==1;
}

inline uLONG VProgressIndicator::GetSessionVersion()
{
	uLONG result;
	XBOX::VInterlocked::Exchange((sLONG*)&result,(sLONG)fSessionVersion);
	return result;
};

class XTOOLBOX_API IRemoteProgressCreator
{
	public:
		IRemoteProgressCreator(){};
		virtual ~IRemoteProgressCreator(){};

	virtual VProgressIndicator* CreateRemoteProgress(VStream& inStream, const VUUID& inClientUUID)=0;
	virtual VProgressIndicator* CreateRemoteProgress(const VUUID& inUUID,sLONG inVersionp, const VUUID& inClientUUID)=0;
	virtual VProgressIndicator* CreateRemoteProgress(const VValueBag& inValueBag,const VUUID& inClientUUID)=0;
};

class XTOOLBOX_API VProgressManager : public VObject
{
public:
	virtual ~VProgressManager();
	static bool Init();
	static bool Deinit();
	
	static VProgressManager* Get(){return sProgressManager;}

	static VProgressIndicator* CreateRemoteProgress(VStream& inStream, const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inStream,inClientUUID);
		}
		return  result;
	}

	static VProgressIndicator* CreateRemoteProgress(const VUUID& inUUID,sLONG inVersion,const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inUUID,inVersion,inClientUUID);
		}
		return  result;
	}
	
	static VProgressIndicator* CreateRemoteProgress(const VValueBag& inValueBag,const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inValueBag,inClientUUID);
		}
		return  result;
	}

	static void RegisterRemoteProgressCreator(IRemoteProgressCreator* inCreator)
	{
		if(sProgressManager)
			sProgressManager->fRemoteCreator=inCreator;
	}
	static void RegisterProgress(VProgressIndicator* inProgress);
	static void UnregisterProgress(VProgressIndicator* inProgress);
	static VProgressIndicator* RetainProgress(const VUUID& inUUID);
	
	static VError GetProgressInfo(VValueBag& outBag, const VString* userinfo = NULL) // if userinfo == nil then get all progress infos
	{
		if (sProgressManager)
			return sProgressManager->_GetProgressInfo(outBag, userinfo);
		else
			return VE_OK;
	}

	/**
	 * \brief Returns an array containing all progress info for all registered progress indicators
	 * \param outCollection the array that will be filled with the information
	 */
	VError GetAllProgressInfo(std::vector< VRefPtr<VRefCountedSessionInfo> >& outCollection);

	static VProgressIndicator* RetainProgressIndicator(const VString& userinfo)
	{
		if (sProgressManager)
			return sProgressManager->_RetainProgressIndicator(userinfo);
		else
			return NULL;
	}

private:
	VProgressManager();
	void _RegisterProgress(VProgressIndicator* inProgress);
	void _UnregisterProgress(VProgressIndicator* inProgress);

	VProgressIndicator* _RetainProgress(const VUUID& inUUID);
	VError _GetProgressInfo(VValueBag& outBag, const VString* userinfo = NULL);
	VProgressIndicator* _RetainProgressIndicator(const VString& userinfo);
	
	typedef std::map < const VUUID, VProgressIndicator* > _VProgressIndicatorMap ;
	typedef _VProgressIndicatorMap::iterator _VProgressIndicatorMap_Iterrator ;
	typedef _VProgressIndicatorMap::const_iterator _VProgressIndicatorMap_Const_Iterrator ;

	static VProgressManager*				sProgressManager;
	
	VCriticalSection						fCrit;
	///UUID to VProgressIndicator map
	_VProgressIndicatorMap					fMap;
	
	IRemoteProgressCreator*					fRemoteCreator;
};

END_TOOLBOX_NAMESPACE

#endif

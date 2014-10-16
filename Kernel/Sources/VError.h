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
#ifndef __VError__
#define __VError__

#include <map>

#if VERSIONMAC
#include <mach/error.h>
#endif

#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VKernelErrors.h"
#include "Kernel/Sources/VStackCrawl.h"
#include "Kernel/Sources/VString.h"


BEGIN_TOOLBOX_NAMESPACE


/*!
	@header	VError
	@abstract Error management classes.
	@discussion
		The VError type is a 64 bits integer that encodes both a component signature and an error code.
		That way, you're free to define your error codes for your own component without conflict.
		For conveniency, the MAKE_VERROR macro helps at defining errors:
		ex:
		const VError	VE_MEMORY_FULL = MAKE_VERROR (kCOMPONENT_XTOOLBOX,100);
		
		When you want to return an error, you should use the vThrowError method that pushes a copy of the error code
		into a dedicated stack for the current thread.
		Hence, you can use vGetLastError () to get at any time the last error generated inside the current thread.
		
		Along with an error code you can push a complete object instance derived from VErrorBase to add meaningfull information
		about the error, such as the file name for the "file not found" error.

		VErrorVerbose is a class derived from VErrorBase that provides a convenient way to push multiple information for an error
		in the same way as the stdlib C printf function. It is automatically used when you call the printf version of vThrowError.
		
		In some occasions, you don't want generated errors being saved in the thread context.
		For instance, you may want to execute some code that may throw a VE_MEMORY_FULL error but you don't want this error be kept
		in the thread error stack so that callers won't detect this error.
		In that case you use the StErrorContextInstaller facility class:
		
		Boolean isOK;
		
		// Installs a new error context that catches any error
		StErrorContextInstaller errorContext;
		
		// Do something that might fail
		TryMemoryHungryAlgorythm();

		// Test error condition
		isOK = (errorContext.GetLastError() != VE_OK);
		
		// If failed, don't keep generated errors because we'll try something else.
		if (!isOK)
			errorContext.Flush();
			
		if (!isOK)
		{ 
			// Try something else
			TryAnotherAlgorythm();
		};
*/


// Class definitions
#if VERSIONMAC

typedef OSStatus VNativeError;
const VNativeError	VNE_OK = noErr;

#elif VERSION_LINUX

typedef int VNativeError;	//errno type ?
const VNativeError	VNE_OK = 0;

#elif VERSIONWIN

typedef DWORD VNativeError;
const VNativeError	VNE_OK = NO_ERROR;

#endif


// Needed declaration
class VString;
class VValueBag;
class ILocalizer;

template<class Type> class VArrayPtrOf;

/*!
	@class	VErrorBase
	@abstract	Error base class.
	@discussion
		All thrown errors are encapsulated inside an object derived from VErrorBase
		which provides information such as the error code, the task ID & name of
		the thread in which the error occured, the stack crawl in debug mode only, etc..
		
		Try to provide as much info as possible at the time the error is built since
		it may be sent from server to be displayed on the client where all info may not be available.
		
		If you use the vThrowError facility method, a VErrorBase object is automatically instantiated for you.
		
		You may wish to implement your own class derived from VErrorBase to provide more specific information.
		In that case you have to call VTask::PushError or VTask::PushRetainedError to throw your error.

			VTask::GetCurrentTask()->PushRetainedError(new VMyComponentError(someInfo));
*/

class XTOOLBOX_API VErrorBase : public VObject, public IRefCountable
{ 
public:
	enum { eMaxStackCrawlDepth = 8, eStackDepthToSkip = 4 };
	typedef std::map<OsType,ILocalizer*>	MapOfLocalizer;
	
									VErrorBase( VError inErrCode, VNativeError inNativeErrCode);
									VErrorBase( const VErrorBase& inOther);

									// empty constructor or serialization purposes
									VErrorBase();

	virtual							~VErrorBase();
	
	// Accessors
			VError					GetError () const							{ return fError; }
			VNativeError			GetNativeError() const						{ return fNativeError; }
			
	// describe the errror code (mandatory)
	// not to be localized.
	// ex: "error -43"
	virtual	void					GetErrorString( VString& outError) const;

	// give description for the error code (optionnal)
	// to be localized.
	// ex: "File 'something.4db' not found."
	virtual	void					GetErrorDescription( VString& outError) const;

	// describe the action (optionnal)
	// to be localized.
	// ex: "While opening the database 'something'".
	virtual	void					GetActionDescription( VString& outAction) const;
	
	// describe the component where the error occured.
	// not to be localized.
	// ex: "database engine component"
	virtual	void					GetLocation( VString& outLocation) const;
	
	// Give the task name where the error occured.
	// note that the task may have been killed when you call GetTaskName.
	// the name and VTaskID returned are the ones at the time the error occured.
	// not to be localized.
	// ex: "user mode"
	virtual	void					GetTaskName( VString& outName, VTaskID *outTaskID) const;

	/*
		@brief The action description is based upon an error code which is by default the same as the main error code.
		This is mainly used for system errors.
		example:
			when opening a file, OSX returns fnfErr(-43) returned by GetNativeError(),
			which is mapped into VE_FILE_NOT_FOUND returned by GetError(),
			and the action error is VE_FILE_CANNOT_OPEN_FILE returned by GetActionError().

			But if OSX returns an unknown native error code that the xbox cannot map,
			then the main and the action error will both be set to VE_FILE_CANNOT_OPEN_FILE.
	*/
	
			VError					GetActionError() const									{ return fActionError; }
			void					SetActionError( VError inError)							{ fActionError = inError; }

	static	VError					NativeErrorToVError( VNativeError inNativeErrCode, VError inDefaultErrCode = VE_UNKNOWN_ERROR);
	static	VError					EncodedNativeErrorToVError( VError inErrCode, VError inDefaultErrCode);
	static	VNativeError			VErrorToNativeError( VError inErrCode);

	// tries to ask the system a description for the error.
	// (empty string if failed)
	static	void					GetNativeErrorString( VNativeError inNativeErrCode, VString& outString);
			
	// Private debug utility
	// Inherited from VObject
	// for debug purpose only.
	virtual	CharSet					DebugDump( char* inTextBuffer, sLONG& ioBufferSize) const;	// Dump a debugging string describing the error
	virtual	void					DumpToString( VString& outString) const;
	
	// Each VErrorBase has non NULL parameters bag you can use to set error properties.
	// These properties are automatically inserted into the error message description.
			VValueBag*				GetBag()												{ return fParameters;}
	const	VValueBag*				GetBag() const											{ return fParameters;}

	// localizer registration per component signature.
	// warning: you are responsible for ensuring that registered localizers are always valid.
	// (unregister them before deletion)
	static void						RegisterLocalizer( OsType inComponentSignature, ILocalizer* inLocalizer);
	static void						UnregisterLocalizer( OsType inComponentSignature);
	static	ILocalizer*				GetLocalizer( OsType inComponentSignature);

	// mamanager init/deinit: private use
	static	bool					Init();
	static	void					DeInit();

			bool					CallLocalizer( ILocalizer *inLocalizer, VError inMessageError, VString& outMessage) const;

	// saving and loading within bags.
	// warning: the internal parameters bag is just retained and copied for efficiency so clone it if you need to modify it.
			VError					LoadFromBag( const VValueBag& inBag);
			VError					SaveToBag( VValueBag& ioBag) const;

	// return stack crawl (available only if compiled WITH_ASSERT)
			void					GetStackCrawl( VString& outStackCrawl) const;
			
protected:
			VError					fError;		// XToolBox error code
			VNativeError			fNativeError;	// Native err code if available

private:
	static	MapOfLocalizer			sLocalizers;
	
			VError					fActionError;	// XToolBox error code detailing the action
			VTaskID					fTaskID;	// TaskID of the task that has thrown the error. May not exist anymore
			VString					fTaskName;	// Name of the task that has thrown the error. May not exist anymore
			VValueBag*				fParameters;		// parameters
			VStackCrawl				fStackCrawl;	// Stack crawl
};



class XTOOLBOX_API VErrorImported : public VErrorBase
{
	public:

		VErrorImported( VError inErrCode, const VString& errorMessage):VErrorBase(inErrCode,0) 
		{
			fMessage = errorMessage;
		};

		virtual	void					GetLocation( VString& outLocation) const;
		virtual	void					GetErrorDescription( VString& outString) const;

	protected:
		virtual void DumpToString( VString& outString) const;
		VString fMessage;

};


typedef std::vector<VRefPtr<VErrorBase> >	VErrorStack;


// Private helper functions (do not call directly - use the ones below)
XTOOLBOX_API VError	_ThrowNativeError (VNativeError inNativeErrCode);
XTOOLBOX_API void	_ThrowError (VError inErrCode, const VString* p1 = NULL, const VString* p2 = NULL, const VString* p3 = NULL);
XTOOLBOX_API void	_PushRetainedError( VErrorBase *inError);


// Throw a basic XToolBox error.
// Does nothing if passed errcode is VE_OK.
// inComponent should be the signature of the component which thrown the error. (kCOMPONENT_XTOOLBOX for xtoolbox)
inline VError vThrowError (VError inErrCode)
{ 
	if (inErrCode == VE_OK)
		return VE_OK;
	
	_ThrowError(inErrCode);
	return inErrCode;
};


inline VError vThrowError (VError inErrCode, const VString& p1)
{ 
	if (inErrCode == VE_OK)
		return VE_OK;

	_ThrowError(inErrCode, &p1);
	return inErrCode;
};


inline VError vThrowError (VError inErrCode, const VString& p1, const VString& p2)
{ 
	if (inErrCode == VE_OK)
		return VE_OK;

	_ThrowError(inErrCode, &p1, &p2);
	return inErrCode;
};


inline VError vThrowError (VError inErrCode, const VString& p1, const VString& p2, const VString& p3)
{ 
	if (inErrCode == VE_OK)
		return VE_OK;

	_ThrowError(inErrCode, &p1, &p2, &p3);
	return inErrCode;
};



XTOOLBOX_API VError vThrowUserGeneratedError(VError inErrCode, const VString& errorMessage);


// A variation for throwing native error code. Should be used from inside XToolbox code only.
// Native means Carbon on Mac, Win32 on Windows
inline VError vThrowNativeError (VNativeError inNativeErrCode, const char *inDebugString = NULL)
{ 
	if (inNativeErrCode == VNE_OK)
		return VE_OK;
	return _ThrowNativeError( inNativeErrCode);
};


inline VError vThrowPosixError( int inPosixError)			{ return (inPosixError == 0) ? VE_OK : vThrowError( MAKE_VERROR( kCOMPONENT_POSIX, inPosixError));}

#if VERSIONMAC
inline VError vThrowMachError( mach_error_t inMachError)	{ return (inMachError == 0) ? VE_OK : vThrowError( MAKE_VERROR( kCOMPONENT_MACH, inMachError));}

VError MakeVErrorFromCFErrorAnRelease( CFErrorRef inCFError);
#endif


/*
	@brief facility class for easy error throwing.
	
	example 1:
	
	if (bad_condition)
	{
		// creates the error object

		StThrowError<>	throwErr( VE_BAD_CONDITION);

		// add some user info into VError object VValueBag.

		throwErr->SetString( "name", offendind_object.GetName());
		throwErr->SetLong( "value", offendind_object.GetValue());
		
		// the throwErr destructor will push the error object
	}

	example 2:

	in case you have your specialized version of VErrorBase,
	its constructor should have one of the following prototypes:
		( VError inDefaultErrCode, VNativeError inNativeErrCode)
		( SomeType inSomeData, VError inDefaultErrCode, VNativeError inNativeErrCode)
	
	if (bad_condition)
	{
		// creates the error object.
		// MyErrorObjectClass gets the offending object so it can read some of its attributes for user information.

		StThrowError<MyErrorObjectClass>	throwErr( offendind_object, VE_BAD_CONDITION);
		
		// the throwErr destructor will push the error object
	}
	
*/
template<class ErrorType = VErrorBase>
class StThrowError
{
public:

	StThrowError( VError inErrCode)
		: fError( new ErrorType( inErrCode, 0))
	{
	}

	StThrowError( VError inDefaultErrCode, VNativeError inNativeErrCode)
		: fError( new ErrorType( VErrorBase::NativeErrorToVError( inNativeErrCode, inDefaultErrCode), inNativeErrCode))
	{
		fError->SetActionError( inDefaultErrCode);
	}

	StThrowError( VError inDefaultErrCode, VError inErrCode)
		: fError( new ErrorType( VErrorBase::EncodedNativeErrorToVError( inErrCode, inDefaultErrCode), VErrorBase::VErrorToNativeError( inErrCode)))
	{
		fError->SetActionError( inDefaultErrCode);
	}

	template<class TargetType>
	StThrowError( TargetType inTarget, VError inErrCode)
		: fError( new ErrorType( inTarget, inErrCode, 0))
	{
	}

	template<class TargetType>
	StThrowError( TargetType inTarget, VError inDefaultErrCode, VNativeError inNativeErrCode)
		: fError( new ErrorType( inTarget, VErrorBase::NativeErrorToVError( inNativeErrCode, inDefaultErrCode), inNativeErrCode))
	{
		fError->SetActionError( inDefaultErrCode);
	}

	template<class TargetType>
	StThrowError( TargetType inTarget, VError inDefaultErrCode, VError inErrCode)
		: fError( new ErrorType( inTarget, VErrorBase::EncodedNativeErrorToVError( inErrCode, inDefaultErrCode), VErrorBase::VErrorToNativeError( inErrCode)))
	{
		fError->SetActionError( inDefaultErrCode);
	}

			VValueBag*				operator -> () const						{ return fError->GetBag(); }
	
			ErrorType&				Get()										{ return *fError; }
			VError					GetError() const							{ return fError->GetError(); }
			VNativeError			GetNativeError() const						{ return fError->GetNativeError(); }
	
	~StThrowError()
	{
		_PushRetainedError( fError);
	}

private:
		ErrorType*	fError;
};


END_TOOLBOX_NAMESPACE

#endif

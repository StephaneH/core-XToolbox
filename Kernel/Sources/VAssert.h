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
#ifndef __VAssert__
#define __VAssert__

#include <set>

#undef assert
#undef assert_compile
#undef testAssert
#undef CHECK_NIL
#undef XBOX_ASSERT_KINDOF
#undef XBOX_ASSERT_KINDOFNIL
#undef XBOX_ASSERT_CLASSOF
//#undef DYNAMIC_CAST

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IRefCountable.h"

// Needed declarations

BEGIN_TOOLBOX_NAMESPACE

class VString;
class VFile;
class VFileStream;
class VErrorBase;
class VCppMemMgr;
class VArrayString;

class IDebugMessageHandler;

// This files is written so that it will have no effect and no overhead
// on release code. However debug and release versions are fully
// reversible.

typedef enum {
	kDEBUGGER_TYPE_NONE	= 0,
	kDEBUGGER_TYPE_MSVISUAL,
	kDEBUGGER_TYPE_GCC,
	kDEBUGGER_TYPE_UNKNOWN
} VDebuggerType;


typedef enum {
	DAC_Continue	= 0,
	DAC_Break		= 1,
	DAC_Abort		= 2,
	DAC_Ignore		= 3,
	DAC_IgnoreAll	= 4
} VDebuggerActionCode;


class XTOOLBOX_API VDebugContext : public VObject
{
public:
							VDebugContext():fLowLevelCount(0), fUIForbiddenCount(0), fBreakForbiddenCount(0), fIsInside(false) {}
	virtual					~VDebugContext()					{}

			bool			Enter()								{ if (fIsInside) return false; fIsInside = true; return true; }
			void			Exit()								{ fIsInside = false;}

			sLONG			StartLowLevelMode()					{ return fLowLevelCount++; }
			sLONG			StopLowLevelMode()					{ return fLowLevelCount--; }
			bool			IsLowLevelMode() const				{ return fLowLevelCount != 0; }

			void			DisableUI()							{ ++fUIForbiddenCount; }
			void			EnableUI()							{ --fUIForbiddenCount; }
			bool			IsUIEnabled() const					{ return fUIForbiddenCount == 0; }

			void			DisableBreak()						{ ++fBreakForbiddenCount; }
			void			EnableBreak()						{ --fBreakForbiddenCount; }
			bool			IsBreakEnabled() const				{ return fBreakForbiddenCount == 0; }

private:
			sLONG			fLowLevelCount;			// if > 0 do minimal things (kernel may be in unstable state)
			sLONG			fUIForbiddenCount;		// if > 0 UI is forbidden (example: preemptive threads on mac)
			sLONG			fBreakForbiddenCount;	// if > 0 Break() is ignored
			bool			fIsInside;
};


/*
	interface to implement assert and error dialogs.
*/
class XTOOLBOX_API IDebugMessageHandler : public IRefCountable
{
public:
	virtual	VDebuggerActionCode			AssertFailed( VDebugContext& inContext, const char *inTestString, const char *inFileName, sLONG inLineNumber) = 0;
    virtual	VDebuggerActionCode			ErrorThrown( VDebugContext& inContext, VErrorBase *inError) = 0;
};


/*
	default implementation that uses notification dialogs in separate process
*/
class XTOOLBOX_API VNotificationDebugMessageHandler : public VObject, public IDebugMessageHandler
{
public:
	virtual	VDebuggerActionCode			AssertFailed( VDebugContext& inContext, const char *inTestString, const char *inFileName, sLONG inLineNumber);
	virtual	VDebuggerActionCode			ErrorThrown( VDebugContext& inContext, VErrorBase *inError);

private:
			VDebuggerActionCode			_TranslateNotificationResponse( ENotificationResponse inResponse);
};


class XTOOLBOX_API VDebugMgr
{
public:
				VDebugMgr();
				~VDebugMgr();

	static	VDebugMgr*				Get();

        #if VERSION_LINUX
            bool                    AssertFailed(const char *inTestString, const char *inFileName, sLONG inLineNumber, const char* inFunctionName="unknown");
        #else
			bool					AssertFailed( const char *inTestString, const char *inFileName, sLONG inLineNumber);
        #endif

			void					ErrorThrown( VErrorBase *inError);
			void					DebugMsg( const char *inMessage, va_list argList);
			void					DebugMsg( const VString& InMessage);

	static	void					Abort();
	static	void					Break();

			void					BreakIfAllowed()				{ if (fBreakAllowed) Break();}

			bool					Init();
			void					DeInit();

	/*!
		@function StartLowLevelMode
		@abstract	Switches the current task to low level debugging.
		@discussion
			When in "low level" mode, all debugger calls are as less intrusive as possible.
			That means minimal other calls to the Xtoolbox are done, no messages are displayed,
			no log is performed, VObject::DebugDump is never called.
			Actually, the only action allowed is to forward plain char* messages to the running debugger.

			This might be useful in rare circumstances, typically for debugging the kernel part of the XToolBox.
	*/
			void					StartLowLevelMode ();


	/*!
		@function StopLowLevelMode
		@abstract	Switches back the current task from low level debugging.
		@discussion
	*/
			void					StopLowLevelMode();

			bool					SetBreakOnAssert( bool inSet);
			bool					GetBreakOnAssert();

			bool					SetBreakOnDebugMsg( bool inSet);
			bool					GetBreakOnDebugMsg();

			bool					SetMessagesEnabled( bool inSet);
			bool					GetMessagesEnabled();

			bool					SetLogInDebugger( bool inSet);
			bool					GetLogInDebugger();

			bool					IsSystemDebuggerAvailable()				{ return fDebuggerAvailable;}

			void					WriteToAssertFile( const VString& inMessage, bool inWithTimeStamp, bool inWithStackCrawl);

			void					SetDebugMessageHandler( IDebugMessageHandler *inDisplayer);
			IDebugMessageHandler*	GetDebugMessageHandler()				{ return fMessageHandler;}

protected:
	static	bool					_GetDebuggerType( VDebuggerType *outType);

private:
// Caution: to ensure full compatibility between release and debug libraries,
// datas must be allways available. However the accessors are inlined so that
// no overhead is generated from release calling code.

	static	VDebugMgr				sInstance;

			const char*				_GetAssertFilePath();
			FILE*					_OpenAssertFile( bool inWithTimeStamp, bool inWithStackCrawl);

			void					fprintf_console( const char *inCString, ...);
			void					vfprintf_console( const char *inCString, va_list inArgList);

			bool					fDebuggerAvailable;	// indicates if we are run under a debugger control
			bool					fMessageEnabled;	// global switch to allow DebugMsg/ErrorThrown/AssertFailed
			bool					fBreakAllowed;		// allows Break()

			bool					fBreakOnAssert;		// calls Break() on AssertFailed
			bool					fBreakOnDebugMsg;	// calls Break() on DebugMsg
			bool					fBreakOnError;		// calls Break() on ErrorThrown

			bool					fLogInDebugger;		// sends messages to the debuggger
			bool					fLogInFile;			// sends messages to a file

			IDebugMessageHandler*	fMessageHandler;	// diagnose window interface

			VArrayString*			fIgnoredAsserts;	// asserts to ignore
			bool					fIgnoreAllAsserts;

			std::set<VError>		fIgnoredErrors;		// errors to ignore
			bool					fIgnoreAllErrors;

			char*					fAssertFilePath;	// full path for assert file
			bool					fAssertFileAlreadyOpened;	// assert file has already been opened once
			sLONG					fAssertFileNumber;
};


//
// Debugging Macros
//

#if COMPIL_GCC && ARCH_386
inline void DebugBreakInline() __attribute__ ((always_inline));
inline void DebugBreakInline()	{ asm ("int3");}
#define XBOX_BREAK_INLINE XBOX::DebugBreakInline()
#elif VERSIONWIN
#define XBOX_BREAK_INLINE ::DebugBreak()
#else
#define XBOX_BREAK_INLINE XBOX::VDebugMgr::Break()
#endif


#if VERSION_LINUX && ASSERT_DONT_BREAK
//Prevent a process from beeing killed on assert if not ran inside debugger
inline void DoNothingInline() __attribute__ ((always_inline));
inline void DoNothingInline()	{ return; }
#undef XBOX_BREAK_INLINE
#define XBOX_BREAK_INLINE XBOX::DoNothingInline()
#endif


#if WITH_ASSERT

#if VERSION_LINUX
	#define xbox_assert(_test_)	( ((_test_) || !XBOX::VDebugMgr::Get()->AssertFailed(#_test_, __FILE__, __LINE__, __PRETTY_FUNCTION__)) ? (void)0 : XBOX_BREAK_INLINE)
#else
    #define xbox_assert(_test_)	( ((_test_) || !XBOX::VDebugMgr::Get()->AssertFailed(#_test_, __FILE__, __LINE__)) ? (void)0 : XBOX_BREAK_INLINE)
#endif

#define assert(_test_) xbox_assert(_test_)

#if COMPIL_CLANG
	#if __has_feature(cxx_static_assert)
		#define WITH_STATIC_ASSERT 1
	#else
		#define WITH_STATIC_ASSERT 0
	#endif
#else
	#define WITH_STATIC_ASSERT 0
#endif

#if WITH_STATIC_ASSERT

#define assert_compile(_test_)	static_assert(_test_,#_test_)

#else

template <bool b> struct compile_assert_template;
template <> struct compile_assert_template<true> {};

#if COMPIL_GCC
#define assert_compile(_test_)	while(false) \
	{ XBOX::compile_assert_template<bool(_test_)> __attribute__((__unused__)) _a;}
#else
#define assert_compile(_test_)	while(false) {XBOX::compile_assert_template<bool(_test_)> _a;}
#endif
#endif

#if VERSION_LINUX

#define CHECK_NIL(_ptr_) ( (_ptr_) ? _ptr_ : ( (XBOX::VDebugMgr::Get()->AssertFailed(#_ptr_ " should not be NULL", __FILE__, __LINE__, __PRETTY_FUNCTION__) ? XBOX_BREAK_INLINE : (void)0), _ptr_) )
#define testAssert(_test_) ( (_test_) ? true : ( (XBOX::VDebugMgr::Get()->AssertFailed(#_test_, __FILE__, __LINE__) ? XBOX_BREAK_INLINE : (void)0), false) )

#else

#define CHECK_NIL(_ptr_) ( (_ptr_) ? _ptr_ : ( (XBOX::VDebugMgr::Get()->AssertFailed(#_ptr_ " should not be NULL", __FILE__, __LINE__,  __PRETTY_FUNCTION__) ? XBOX_BREAK_INLINE : (void)0), _ptr_) )
#define testAssert(_test_) ( (_test_) ? true : ( (XBOX::VDebugMgr::Get()->AssertFailed(#_test_, __FILE__, __LINE__) ? XBOX_BREAK_INLINE : (void)0), false) )

#endif

// _object_ is of class _class_name_ or a derived class, _object_ cannot be NULL
#define XBOX_ASSERT_KINDOF(_class_name_, _object_) \
	 (XBOX_ASSERT_VOBJECT(_object_), xbox_assert(dynamic_cast<const _class_name_*>(reinterpret_cast<const XBOX::VObject*>(_object_))))

// _object_ is of class _class_name_ or a derived class, _object_ can be NULL
#define XBOX_ASSERT_KINDOFNIL(_class_name_, _object_) \
	if (_object_) {\
		 (XBOX_ASSERT_VOBJECT(_object_), xbox_assert(dynamic_cast<const _class_name_*>(reinterpret_cast<const XBOX::VObject*>(_object_)))); \
	}

// _object_ must be of class _class_name_ and nothing else
#define XBOX_ASSERT_CLASSOF(_class_name_, _object_) \
	 (XBOX_ASSERT_VOBJECT(_object_), xbox_assert(typeid(_class_name_) == typeid(*_object_)))

// cast _object_ into a _class_name_ class _object_, _object_ can be NULL
//#define DYNAMIC_CAST(_class_name_, _object_) ((_object_) ? (XBOX_ASSERT_VOBJECT(_object_), CHECK_NIL(dynamic_cast<_class_name_ *>(reinterpret_cast<XBOX::VObject*>(_object_)))) : (_class_name_ *)_object_)
#else

#define assert_compile(_test_)

#define assert(_test_)
#define xbox_assert(_test_)
#define testAssert(_test_)	 (_test_)

#define CHECK_NIL(_ptr_) (_ptr_)

#define XBOX_ASSERT_KINDOF(_class_name_, _object_)
#define XBOX_ASSERT_KINDOFNIL(_class_name_, _object_)
#define XBOX_ASSERT_CLASSOF(_class_name_, _object_)
//#define DYNAMIC_CAST(_class_name_,_object_) ((_class_name_ *) (_object_))

#endif


#if WITH_DEBUGMSG

inline void DebugMsg(const VString& inMessage)
{
	VDebugMgr::Get()->DebugMsg( inMessage);
}

inline void DebugMsg(const char *inMessage, ...)
{
	va_list argList;
	va_start(argList, inMessage);
	VDebugMgr::Get()->DebugMsg( inMessage, argList);
	va_end(argList);
}

#else

inline void DebugMsg(const VString&) {}
inline void DebugMsg(const char *, ...) {}

#endif	// VERSION_DEBUG

END_TOOLBOX_NAMESPACE

#endif

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

#if VERSION_LINUX && !defined(__clang__)
#undef  _GNU_SOURCE // strerror_r - force XSI-compliant
#endif
#include "VKernelPrecompiled.h"
#include "VError.h"
#include "VTask.h"
#include "VProcess.h"
#include "VString.h"
#include "VArrayValue.h"
#include "VValueBag.h"
#include "VArray.h"
#include "ILocalizer.h"

#if VERSIONMAC
#include <mach/mach_error.h>
#endif

BEGIN_TOOLBOX_NAMESPACE

namespace ErrorBagKeys
{
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( error_code, VLong8, VError);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( os_error_code, VLong, VNativeError);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( action_error_code, VLong8, VError);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( task_id, VLong, VTaskID);
	CREATE_BAGKEY_NO_DEFAULT( task_name, VString);
	VValueBag::StKey parameters( "parameters");
	VValueBag::StKey error( "error");
	VValueBag::StKey error_stack( "error_stack");
};


/*
// These error codes are declared in VErrorTypes.h, but are defined here.
// This way, only one copy of these codes exists
const VError VE_NET_UNKNOWN 			= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 801);
const VError VE_NET_INTERRUPTED 		= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 802);
const VError VE_NET_CLOSED 				= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 803);
const VError VE_NET_TIMEOUT 			= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 804);
const VError VE_NET_CONNECTION_REFUSED	= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 805);
const VError VE_NET_HOST_UNREACHABLE	= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 806);
const VError VE_NET_ADDRESS_IN_USE		= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 807);
const VError VE_NET_WOULD_BLOCK			= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 808);
const VError VE_NET_ACCESS_DENIED		= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 809);
const VError VE_NET_INVALID				= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 810);
const VError VE_NET_UNSUPPORTED			= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 811);
const VError VE_NET_UNIMPLEMENTED		= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 812);
const VError VE_NET_ENDPOINT_NOT_FOUND	= MAKE_VERROR(kCOMPONENT_XTOOLBOX, 813);
*/

VErrorBase::MapOfLocalizer		VErrorBase::sLocalizers;

typedef struct VErrorDesc
{
	VNativeError	fNativeError;
	VError			fError;
	sLONG			fMsgID;
} VErrorDesc;


/*
	Le but de cette table de correspondances n'est pas de remapper tous les codes d'erreur,
	car d'une part ce serait impossible et d'autre part le code d'erreur natif est preserve dans la classe VErrorBase.
	
	Donc ce que l'on veut ici c'est remapper les codes d'erreur les plus frequemment testes
	pour pouvoir ecrire du code cross-plateforme dependant de conditions d'erreurs particulieres.
	
	ex:
		if (file.Copy( destination) == VE_DISK_FULL)
		{
			// plus de place: demander a l'utilisteur de choisir une autre destination
			...
		}
	
	Un code non mappe donnera VE_UNKNOWN_ERROR.
	Il est important de privilegier la justesse du mapping a son exhaustivite pour eviter
	les erreurs d'interpretation.
	
*/
static VErrorDesc	sErrMap[] =
{
#if VERSIONMAC
	{ noErr, VE_OK, 0 },
	{ memFullErr, VE_MEMORY_FULL, 0 },
	{ eofErr, VE_STREAM_EOF, 0 },
	{ dskFulErr, VE_DISK_FULL, 0 },
	{ fnfErr, VE_FILE_NOT_FOUND, 0 },
	{ dirNFErr, VE_FOLDER_NOT_FOUND, 0 },
	{ fLckdErr, VE_ACCESS_DENIED, 0 },
	{ vLckdErr, VE_ACCESS_DENIED, 0 },
	{ resNotFound, VE_RES_NOT_FOUND, 0 },
	{ bdNamErr, VE_FILE_BAD_NAME, 0},
	{ permErr, VE_ACCESS_DENIED, 0},	// open read-write a file already opened by someone else as read-write
#elif VERSIONWIN
	{ NO_ERROR, VE_OK, 0 },
	{ ERROR_NOT_ENOUGH_MEMORY, VE_MEMORY_FULL, 0 },
	{ ERROR_OUTOFMEMORY, VE_MEMORY_FULL, 0 },
	{ ERROR_HANDLE_EOF, VE_STREAM_EOF, 0 },
	{ ERROR_HANDLE_DISK_FULL, VE_DISK_FULL, 0 },
	{ ERROR_FILE_NOT_FOUND, VE_FILE_NOT_FOUND, 0 },
	{ ERROR_PATH_NOT_FOUND, VE_FILE_NOT_FOUND, 0 },
	{ ERROR_ACCESS_DENIED, VE_ACCESS_DENIED, 0 },
	{ ERROR_INVALID_NAME, VE_FILE_BAD_NAME, 0},
#endif
	{ 0, VE_UNKNOWN_ERROR, 0 }
};


VErrorBase::VErrorBase()
: fError( VE_OK)
, fNativeError( VNE_OK)
, fParameters( new VValueBag)
, fActionError( VE_OK)
, fTaskID( NULL_TASK_ID)
{
}


VErrorBase::VErrorBase(VError inErrCode, VNativeError inNativeErrCode)
: fError( inErrCode)
, fNativeError( inNativeErrCode)
, fParameters( new VValueBag)
, fActionError( inErrCode)
{
	VTask*	theTask = VTask::GetCurrent();
	fTaskID = theTask->GetID();
	theTask->GetName( fTaskName);	// on prend le nom tout de suite car la tache peut disparaitre ensuite

#if WITH_ASSERT
	fStackCrawl.LoadFrames(eStackDepthToSkip, eMaxStackCrawlDepth);
#endif
}


VErrorBase::VErrorBase( const VErrorBase& inOther)
: fError( inOther.fError)
, fNativeError( inOther.fNativeError)
, fTaskID( inOther.fTaskID)
, fTaskName( inOther.fTaskName)
, fParameters( inOther.fParameters->Clone())
, fActionError( inOther.fActionError)
{
	// can't copy stack crawl !
}


VErrorBase::~VErrorBase()
{
	ReleaseRefCountable( &fParameters);
}


/*
	static
*/
bool VErrorBase::Init()
{
	return true;
}


/*
	static
*/
void VErrorBase::DeInit()
{
}


VError VErrorBase::LoadFromBag( const VValueBag& inBag)
{
	ErrorBagKeys::error_code.Get( &inBag, fError);
	ErrorBagKeys::os_error_code.Get( &inBag, fNativeError);
	ErrorBagKeys::action_error_code.Get( &inBag, fActionError);
	ErrorBagKeys::task_id.Get( &inBag, fTaskID);
	ErrorBagKeys::task_name.Get( &inBag, fTaskName);

	const VBagArray *parameters = inBag.GetElements( ErrorBagKeys::parameters);
	if ( (parameters != NULL) && !parameters->IsEmpty() )
	{
		VValueBag *clonedParameters = parameters->GetNth( 1)->Clone();
		CopyRefCountable( &fParameters, clonedParameters);
		ReleaseRefCountable( &clonedParameters);
	}

	return VE_OK;
}


VError VErrorBase::SaveToBag( VValueBag& ioBag) const
{
	ErrorBagKeys::error_code.Set( &ioBag, fError);
	ErrorBagKeys::os_error_code.Set( &ioBag, fNativeError);
	ErrorBagKeys::action_error_code.Set( &ioBag, fActionError);
	ErrorBagKeys::task_id.Set( &ioBag, fTaskID);
	ErrorBagKeys::task_name.Set( &ioBag, fTaskName);

	ioBag.ReplaceElement( ErrorBagKeys::parameters, fParameters);
	
	return VE_OK;
}


void VErrorBase::RegisterLocalizer( OsType inComponentSignature, ILocalizer* inLocalizer)
{
	sLocalizers[inComponentSignature] = inLocalizer;
}


void VErrorBase::UnregisterLocalizer( OsType inComponentSignature)
{
	sLocalizers.erase( inComponentSignature);
}


ILocalizer* VErrorBase::GetLocalizer( OsType inComponentSignature)
{
	MapOfLocalizer::const_iterator i = sLocalizers.find( inComponentSignature);
	return (i != sLocalizers.end()) ? i->second : NULL;
}


CharSet VErrorBase::DebugDump(char* inTextBuffer, sLONG& ioBufferSize) const
{
	CharSet charSet = VTC_UTF_16;
	if (VProcess::Get() != NULL)
	{
		VString dump;
		DumpToString(dump);
		dump.Truncate(ioBufferSize/2);
		ioBufferSize = (sLONG) dump.ToBlock(inTextBuffer, ioBufferSize, VTC_UTF_16, false, false);
	}
	else
	{
		sLONG errCode = ERRCODE_FROM_VERROR(fError);
		OsType component = COMPONENT_FROM_VERROR(fError);
		ioBufferSize = sprintf(inTextBuffer, "Error code: %ld, component: '%.4s', task ID: %d", errCode, (char*) &component, (int) fTaskID);
	}
	return charSet;
}


void VErrorBase::DumpToString( VString& outString) const
{
	VString dump;

	GetErrorString( outString);
	outString += "\n";
	
	GetErrorDescription( dump);
	if (!dump.IsEmpty())
	{
		outString += dump;
		outString += "\n";
	}

	GetLocation( dump);
	if (!dump.IsEmpty())
	{
		outString += dump;
		outString += "\n";
	}

	GetActionDescription( dump);
	if (!dump.IsEmpty())
	{
		outString += dump;
		outString += "\n";
	}

	if (!fTaskName.IsEmpty())
		dump.Printf("task %d, name: '%S'", fTaskID, &fTaskName);
	else
		dump.Printf("task %d", fTaskID);
	outString += dump;
	outString += "\n";
	
#if WITH_ASSERT
	fStackCrawl.Dump(outString);
#endif
}


void VErrorBase::GetStackCrawl( VString& outStackCrawl) const
{
#if WITH_ASSERT
	fStackCrawl.Dump( outStackCrawl);
#endif
}


VError VErrorBase::NativeErrorToVError( VNativeError inNativeErrCode, VError inDefaultErrCode)
{
	VErrorDesc*	desc = sErrMap;
	while((desc->fNativeError != inNativeErrCode) && (desc->fError != VE_UNKNOWN_ERROR))
		++desc;
	
	VError err = desc->fError;
	if (err == VE_UNKNOWN_ERROR)
		err = inDefaultErrCode;

	return err;
}


VError VErrorBase::EncodedNativeErrorToVError( VError inErrCode, VError inDefaultErrCode)
{
	VError err;
	if (IS_NATIVE_VERROR( inErrCode))
	{
		err = NativeErrorToVError( NATIVE_ERRCODE_FROM_VERROR( inErrCode), inDefaultErrCode);
	}
	else
	{
		err = inErrCode;
	}

	return err;
}


VNativeError VErrorBase::VErrorToNativeError( VError inErrCode)
{
	VNativeError err;
	if (IS_NATIVE_VERROR( inErrCode))
	{
		err = NATIVE_ERRCODE_FROM_VERROR( inErrCode);
	}
	else
	{
		err = 0;
	}
	return err;
}


void VErrorBase::GetErrorString( VString& outError) const
{
	sLONG errCode = ERRCODE_FROM_VERROR(fError);
	OsType component = COMPONENT_FROM_VERROR(fError);

	if (fNativeError != VNE_OK)
	{
		outError.Printf( "Error code: %d (OS error: %d)", errCode, fNativeError);
	}
	else
	{
		outError.Printf( "Error code: %d", errCode);
	}
}


void VErrorBase::GetErrorDescription( VString& outError) const
{
	sLONG errCode = ERRCODE_FROM_VERROR(fError);
	OsType component = COMPONENT_FROM_VERROR(fError);

	bool found = CallLocalizer( GetLocalizer( component), fError, outError);
	
	#if VERSIONMAC
	if (!found && (component == kCOMPONENT_MACH) )
	{
		const char *s = mach_error_string( (mach_error_t) errCode);
		if ( (s != NULL) && (strlen( s) != 0) )
		{
			outError.FromBlock( s, strlen( s), VTC_UTF_8);
			found = true;
		}
	}
	#endif

	if (!found && (component == kCOMPONENT_POSIX) )
	{
#if VERSIONWIN
		wchar_t buff[1024];
		if (_wcserror_s( buff, sizeof( buff)/sizeof(wchar_t), (int) errCode) == 0)
		{
			outError.FromUniCString( buff);
			found = true;
		}
#elif VERSIONMAC
		char buff[1024];
		int r = strerror_r( (int) errCode, buff, sizeof( buff));
		if ( (r == 0 || r == ERANGE) && (strlen( buff) != 0) )
		{
			outError.FromBlock( buff, strlen( buff), VTC_UTF_8);
			found = true;
		}
#elif VERSION_LINUX
	#ifdef __clang__
		//clang uses GNU version of strerror_r (Bug ?)
		char buff[1024];
		buff[0]=0;
		char* msg = strerror_r( (int) errCode, buff, sizeof( buff));

		found = strlen(msg) > 0;	//jmo - not quite sure
		
		if(found)
			outError.FromBlock( msg, strlen( msg), VTC_UTF_8);
	#else
		//gcc uses XSI (portable) version of strerror_r
		char buff[1024];
		int r = strerror_r( (int) errCode, buff, sizeof( buff));
		if ( (r == 0 || (r == -1 && errno == ERANGE)) && (strlen( buff) != 0) )
		{
			outError.FromBlock( buff, strlen( buff), VTC_UTF_8);
			found = true;
		}
	#endif
#endif
	}

	if (fNativeError != VNE_OK)
	{
		// if there's a native error description, take it.
		// Std error dialog will show GetActionDescription() in addition.
		VString errorString;
		GetNativeErrorString( fNativeError, errorString);
		if (!errorString.IsEmpty())
		{
			outError = errorString;
		}
	}

	if (outError.IsEmpty())
	{
		fParameters->GetString( "error_description", outError);
		outError.Format( fParameters);
	}
}

	
void VErrorBase::GetActionDescription( VString& outAction) const
{
	// returns an action string only if different from error description
	if ( (fActionError != fError) || (fNativeError != VNE_OK) )
	{
		CallLocalizer( GetLocalizer( COMPONENT_FROM_VERROR( fActionError)), fActionError, outAction);
	}
	else
	{
		outAction.Clear();
	}

	if (outAction.IsEmpty())
	{
		fParameters->GetString( "action_description", outAction);
		outAction.Format( fParameters);
	}
}


bool VErrorBase::CallLocalizer( ILocalizer *inLocalizer, VError inMessageError, VString& outMessage) const
{
	bool ok;
	if (inMessageError != VE_OK)
	{
		ILocalizer *localizer = (inLocalizer == NULL) ? VProcess::Get()->GetLocalizer() : inLocalizer;
			
		ok = (inLocalizer != NULL) ? localizer->LocalizeErrorMessage( inMessageError, outMessage) : false;
		if (ok)
			outMessage.Format(fParameters);
	}
	else
	{
		ok = true;
	}
	
	if (!ok)
		outMessage.Clear();

	return ok;
}


void VErrorBase::GetLocation( VString& outLocation) const
{
	OsType component = COMPONENT_FROM_VERROR( fError);
	if (component == kCOMPONENT_XTOOLBOX)
		outLocation = "xtoolbox";
	else
	{
		// should call the component to get its name but we are not in kernel ipc.
		outLocation.Printf( "component: '%c%c%c%c'", (char) (component >> 24), (char) (component >> 16), (char) (component >> 8), (char) component);
	}
}


void VErrorBase::GetTaskName( VString& outName, VTaskID *outTaskID) const
{
	outName = fTaskName;
	if (outTaskID)
		*outTaskID = fTaskID;
}


void VErrorBase::GetNativeErrorString( VNativeError inNativeErrCode, VString& outString)
{
	outString.Clear();
	if (inNativeErrCode != VNE_OK)
	{
	#if VERSIONWIN
		LPVOID lpMsgBuf = NULL;
		::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, inNativeErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) &lpMsgBuf, 0, NULL);
		if (lpMsgBuf != NULL)
		{
			outString.FromBlock( lpMsgBuf, sizeof( UniChar) * wcslen( (wchar_t*) lpMsgBuf), VTC_UTF_16);
			::LocalFree( lpMsgBuf);
		}
	#elif VERSIONMAC
		// use 10.4 api if available
		if (::GetMacOSStatusCommentString != NULL && ::GetMacOSStatusErrorString != NULL)
		{
			outString.Printf( "%s (%s).", ::GetMacOSStatusCommentString( inNativeErrCode), ::GetMacOSStatusErrorString( inNativeErrCode));
		}
	#endif
	}
}

//====================================================================================================================


void _ThrowError( VError inErrCode, const VString* p1, const VString* p2, const VString* p3)
{
	if (VProcess::Get() != NULL)
	{
		StAllocateInMainMem mainAllocator;
		VErrorBase* err = new VErrorBase( inErrCode, 0);
		if (err != NULL)
		{
			if (p1 != NULL)
				err->GetBag()->SetString("p1", *p1);
			if (p2 != NULL)
				err->GetBag()->SetString("p2", *p2);
			if (p3 != NULL)
				err->GetBag()->SetString("p3", *p3);
			VTask::GetCurrent()->PushError(err);
		}
		ReleaseRefCountable( &err);
	}
}


VError _ThrowNativeError( VNativeError inNativeErrCode)
{
	VError	xbox_err = VErrorBase::NativeErrorToVError( inNativeErrCode);

	if (VProcess::Get() != NULL)
	{
		StAllocateInMainMem mainAllocator;
		
		VErrorBase* err = new VErrorBase( xbox_err, inNativeErrCode);
		VTask::GetCurrent()->PushError( err);
		ReleaseRefCountable( &err);
	}

	return xbox_err;
}


void _PushRetainedError( VErrorBase *inError)
{
	VTask::PushError( inError);
	ReleaseRefCountable( &inError);
}


//====================================================================================================================



void VErrorImported::DumpToString( VString& outString) const
{
	outString = fMessage;
}



void VErrorImported::GetLocation( VString& outLocation) const
{
	outLocation = L"REST Imported Error";
}


void VErrorImported::GetErrorDescription( VString& outString) const
{
	outString = fMessage;
}


//====================================================================================================================



class VErrorUserGenerated : public VErrorBase
{
public:

	VErrorUserGenerated( VError inErrCode, const VString& errorMessage):VErrorBase(inErrCode,0) 
	{ 
		fMessage = errorMessage;
	}

protected:

	virtual void DumpToString( VString& outString) const
	{
		outString = fMessage;
	}

	virtual	void GetErrorDescription( VString& outError) const
	{
		outError = fMessage;
	}


	VString fMessage;

};


VError vThrowUserGeneratedError(VError inErrCode, const VString& errorMessage)
{
	VErrorUserGenerated* error = new VErrorUserGenerated( inErrCode, errorMessage);
	VTask::GetCurrent()->PushError( error);
	ReleaseRefCountable( &error);
	return inErrCode;

}


//====================================================================================================================

#if VERSIONMAC

VError MakeVErrorFromCFErrorAnRelease( CFErrorRef inCFError)
{
	if (inCFError == NULL)
		return VE_OK;

	VError err;
	CFStringRef cfDomain = CFErrorGetDomain( inCFError);
	CFIndex errorCode = CFErrorGetCode( inCFError);
	if (CFEqual( cfDomain, kCFErrorDomainOSStatus))
	{
		err = MAKE_VERROR( kCOMPONENT_SYSTEM, errorCode);
	}
	else if (CFEqual( cfDomain, kCFErrorDomainPOSIX))
	{
		err = MAKE_VERROR( kCOMPONENT_POSIX, errorCode);
	}
	else if (CFEqual( cfDomain, kCFErrorDomainMach))
	{
		err = MAKE_VERROR( kCOMPONENT_POSIX, errorCode);
	}
	else
	{
		xbox_assert( false);
		err = VE_UNKNOWN_ERROR;
	}
	CFRelease( inCFError);
	return err;

}
#endif

END_TOOLBOX_NAMESPACE


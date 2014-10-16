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
#include "XMacSystem.h"
#include "VString.h"
#include "VError.h"
#include "VValueBag.h"
#include "ILocalizer.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>



// [static]
sLONG XMacSystem::GetMacOSXVersionDecimal()
{
	SInt32 versionMajor, versionMinor, versionBugFix;
	OSErr macError;
	macError = Gestalt( gestaltSystemVersionMajor, &versionMajor);
	macError = Gestalt( gestaltSystemVersionMinor, &versionMinor);
	macError = Gestalt( gestaltSystemVersionBugFix, &versionBugFix);

	// decimal notation because it's easier
	// 10.5.6 = 100506
	return versionMajor * 10000 + versionMinor * 100 + versionBugFix;
}


// [static]
bool XMacSystem::IsSystemVersionOrAbove( SystemVersion inSystemVersion)
{
	SInt32 versionMajor, versionMinor, versionBugFix;
	OSErr macError;
	macError = Gestalt( gestaltSystemVersionMajor, &versionMajor);
	macError = Gestalt( gestaltSystemVersionMinor, &versionMinor);

	xbox_assert( versionMajor >= 10);

	switch( inSystemVersion)
	{
		case MAC_OSX_10_4:	return ((versionMajor == 10) && (versionMinor >= 4)) || (versionMajor > 10);
		case MAC_OSX_10_5:	return ((versionMajor == 10) && (versionMinor >= 5)) || (versionMajor > 10);
		case MAC_OSX_10_6:	return ((versionMajor == 10) && (versionMinor >= 6)) || (versionMajor > 10);
		case MAC_OSX_10_7:	return ((versionMajor == 10) && (versionMinor >= 7)) || (versionMajor > 10);
		case MAC_OSX_10_9:	return ((versionMajor == 10) && (versionMinor >= 9)) || (versionMajor > 10);
		default:			return true;
	}
}


// [static]
VError XMacSystem::GetMacOSXFormattedVersionString( VString& outFormattedVersionString)
{
	XBOX::VString productName;
	XBOX::VString productUserVisibleVersion;

	CFURLRef cfUrl = CFURLCreateWithFileSystemPath( NULL, CFSTR("/System/Library/CoreServices/SystemVersion.plist"), kCFURLPOSIXPathStyle, false);

	CFDataRef cfData = NULL;
	SInt32 error;
	if (cfUrl)
		CFURLCreateDataAndPropertiesFromResource( NULL, cfUrl, &cfData, NULL, NULL, &error);

	CFDictionaryRef cfList = (cfData == NULL) ? NULL : static_cast<CFDictionaryRef>(CFPropertyListCreateFromXMLData( NULL, cfData, kCFPropertyListImmutable, NULL));

	CFStringRef cfProductName = (cfList == NULL) ? NULL : static_cast<CFStringRef>(CFDictionaryGetValue( cfList, CFSTR("ProductName")));
	if (cfProductName != NULL)
		productName.MAC_FromCFString( cfProductName);

	CFStringRef cfProductUserVisibleVersion = (cfList == NULL) ? NULL : static_cast<CFStringRef>(CFDictionaryGetValue( cfList, CFSTR("ProductUserVisibleVersion")));
	if (cfProductUserVisibleVersion != NULL)
		productUserVisibleVersion.MAC_FromCFString( cfProductUserVisibleVersion);
	
	if (cfList)
		CFRelease( cfList);
	if (cfUrl)
		CFRelease( cfUrl);
	if (cfData)
		CFRelease( cfData);

	outFormattedVersionString = productName;
	outFormattedVersionString += " (";
	outFormattedVersionString += productUserVisibleVersion;
	outFormattedVersionString += ")";

	return VE_OK;
}

// [static]
VError XMacSystem::GetProcessorBrandName( XBOX::VString& outProcessorName)
{
	VError error = VE_OK;
	
	static VString sProcessorName;
	
	if (sProcessorName.IsEmpty())
	{
		// machdep.cpu.brand_string n'est dispo que sur intel
		char buffer[512];
		size_t size = sizeof( buffer);
		int	r = ::sysctlbyname( "machdep.cpu.brand_string", buffer, &size, NULL, 0);
		if (testAssert( r == 0))
		{
			sProcessorName.FromBlock( buffer, size, VTC_UTF_8);
		}
		else
		{
			error = VE_UNKNOWN_ERROR;
		}
	}
	
	outProcessorName = sProcessorName;
	
	return error;
}


static bool PutStringIntoDictionary( const VString& inString, CFMutableDictionaryRef inDictionary, CFStringRef inDictionaryKey)
{
	bool ok = false;
	if (!inString.IsEmpty() && (inDictionary != NULL) )
	{
		CFStringRef cfString = inString.MAC_RetainCFStringCopy();
		if (cfString != NULL)
		{
			CFDictionaryAddValue( inDictionary,	inDictionaryKey, cfString);
			CFRelease( cfString);
			ok = true;
		}
	}
	return ok;
}


bool XMacSystem::DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse)
{
	CFOptionFlags flags = (inOptions & EDN_StyleCritical) ? kCFUserNotificationCautionAlertLevel : kCFUserNotificationPlainAlertLevel;

	CFStringRef cfIconFileName = (CFStringRef) CFBundleGetValueForInfoDictionaryKey( CFBundleGetMainBundle(), CFSTR("CFBundleIconFile"));
	CFURLRef cfIconURL = (cfIconFileName == NULL) ? NULL : CFBundleCopyResourceURL( CFBundleGetMainBundle(), cfIconFileName, NULL, NULL);

	CFMutableDictionaryRef dico = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	if (cfIconURL != NULL && dico != NULL)
		CFDictionaryAddValue( dico,	kCFUserNotificationIconURLKey, cfIconURL);
	
	// kCFUserNotificationAlertHeaderKey is required
	if (inTitle.IsEmpty())
	{
		PutStringIntoDictionary( inMessage, dico, kCFUserNotificationAlertHeaderKey);
	}
	else
	{
		PutStringIntoDictionary( inTitle, dico, kCFUserNotificationAlertHeaderKey);
		PutStringIntoDictionary( inMessage, dico, kCFUserNotificationAlertMessageKey);
	}

	ENotificationResponse responseDefault = ERN_None;
	ENotificationResponse responseAlternate = ERN_None;
	ENotificationResponse responseOther = ERN_None;

	switch( inOptions & EDN_LayoutMask)
	{
		case EDN_OK_Cancel:
			if (inOptions & EDN_Default1)
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("OK"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("Cancel"));
				responseDefault = ERN_OK;
				responseAlternate = ERN_Cancel;
			}
			else
			{
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("OK"));
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("Cancel"));
				responseDefault = ERN_Cancel;
				responseAlternate = ERN_OK;
			}
			break;

		case EDN_Yes_No:
			if (inOptions & EDN_Default1)
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("Yes"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("No"));
				responseDefault = ERN_Yes;
				responseAlternate = ERN_No;
			}
			else
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("No"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("Yes"));
				responseDefault = ERN_No;
				responseAlternate = ERN_Yes;
			}
			break; 
		
		case EDN_Yes_No_Cancel:
			if ((inOptions & EDN_DefaultMask) == EDN_Default1)
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("Yes"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("No"));
				CFDictionaryAddValue( dico, kCFUserNotificationOtherButtonTitleKey, CFSTR("Cancel"));
				responseDefault = ERN_Yes;
				responseAlternate = ERN_No;
				responseOther = ERN_Cancel;
			}
			else if ((inOptions & EDN_DefaultMask) == EDN_Default2)
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("No"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("Yes"));
				CFDictionaryAddValue( dico, kCFUserNotificationOtherButtonTitleKey, CFSTR("Cancel"));
				responseDefault = ERN_No;
				responseAlternate = ERN_Yes;
				responseOther = ERN_Cancel;
			}
			else
			{
				CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("Cancel"));
				CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("No"));
				CFDictionaryAddValue( dico, kCFUserNotificationOtherButtonTitleKey, CFSTR("Yes"));
				responseDefault = ERN_Cancel;
				responseAlternate = ERN_No;
				responseOther = ERN_Yes;
			}

		case EDN_Abort_Retry_Ignore:
			CFDictionaryAddValue( dico, kCFUserNotificationDefaultButtonTitleKey, CFSTR("Retry"));
			CFDictionaryAddValue( dico, kCFUserNotificationAlternateButtonTitleKey, CFSTR("Ignore"));
			CFDictionaryAddValue( dico, kCFUserNotificationOtherButtonTitleKey, CFSTR("Abort"));
			responseDefault = ERN_Retry;
			responseAlternate = ERN_Ignore;
			responseOther = ERN_Abort;
			break;

		case EDN_OK:
		default:
			responseDefault = ERN_OK;
	}

	SInt32 error = 0;
	CFUserNotificationRef refNotification = CFUserNotificationCreate( kCFAllocatorDefault, 0 /*CFTimeInterval timeout*/, flags, &error, dico);

	if ( (refNotification != NULL) && (outResponse != NULL) )
	{
		*outResponse = ERN_None;
		if (error == 0)
		{
			CFOptionFlags responseFlags = 0;
			SInt32 rCode = CFUserNotificationReceiveResponse( refNotification, 0 /*CFTimeInterval timeout*/, &responseFlags);
			if (testAssert( rCode == 0))	// "0 if the cancel was successful; a non-0 value otherwise."
			{
				switch( responseFlags & 0x3)
				{
					case kCFUserNotificationDefaultResponse:	*outResponse = responseDefault; break;
					case kCFUserNotificationAlternateResponse:	*outResponse = responseAlternate; break;
					case kCFUserNotificationOtherResponse:		*outResponse = responseOther; break;
					case kCFUserNotificationCancelResponse:		*outResponse = ERN_Timeout; break;
				}
			}
		}
	}

	if (refNotification != NULL)
		CFRelease( refNotification);

	if (cfIconURL != NULL)
		CFRelease( cfIconURL);

	if (dico != NULL)
		CFRelease( dico);

	return error == 0;
}

void XMacSystem::GetProcessList(std::vector<pid> &outPIDs)
{
	kinfo_proc *procList = NULL;
	size_t procCount;
	int                 err;
	kinfo_proc *        result;
	bool                done;
	static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	// Declaring name as const requires us to cast it when passing it to
	// sysctl because the prototype doesn't include the const modifier.
	size_t              length;


	procCount = 0;

	// We start by calling sysctl with result == NULL and length == 0.
	// That will succeed, and set length to the appropriate length.
	// We then allocate a buffer of that size and call sysctl again
	// with that buffer.  If that succeeds, we're done.  If that fails
	// with ENOMEM, we have to throw away our buffer and loop.  Note
	// that the loop causes use to call sysctl with NULL again; this
	// is necessary because the ENOMEM failure case sets length to
	// the amount of data returned, not the amount of data that
	// could have been returned.

	result = NULL;
	done = false;

	do {
		xbox_assert(result == NULL);

		// Call sysctl with a NULL buffer.

		length = 0;
		err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
			NULL, &length,
			NULL, 0);
		if (err == -1) {
			err = errno;
		}

		// Allocate an appropriately sized buffer based on the results
		// from the previous call.

		if (err == 0) {
			result = (kinfo_proc *)malloc(length);
			if (result == NULL) {
				err = ENOMEM;
			}
		}

		// Call sysctl again with the new buffer.  If we get an ENOMEM
		// error, toss away our buffer and start again.

		if (err == 0) {
			err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
				result, &length,
				NULL, 0);
			if (err == -1) {
				err = errno;
			}
			if (err == 0) {
				done = true;
			} else if (err == ENOMEM) {
				xbox_assert(result != NULL);
				free(result);
				result = NULL;
				err = 0;
			}
		}
	} while (err == 0 && ! done);

	// Clean up and establish post conditions.

	if (err != 0 && result != NULL)
	{
		free(result);
		result = NULL;
	}

	procList = result;

	if (err == 0)
	{
		procCount = length / sizeof(kinfo_proc);
	}


	for (unsigned int i = 0; i < procCount; ++i)
	{
		outPIDs.push_back(procList[i].kp_proc.p_pid);
	}

	free(procList);
}

void XMacSystem::GetProcessName(uLONG inProcessID, XBOX::VString& outProcessName)
{
	kinfo_proc *procList = NULL;
	
	size_t procCount;
	int                 err;
	kinfo_proc *        result;
	bool                done;
	static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	// Declaring name as const requires us to cast it when passing it to
	// sysctl because the prototype doesn't include the const modifier.
	size_t              length;


	procCount = 0;

	// We start by calling sysctl with result == NULL and length == 0.
	// That will succeed, and set length to the appropriate length.
	// We then allocate a buffer of that size and call sysctl again
	// with that buffer.  If that succeeds, we're done.  If that fails
	// with ENOMEM, we have to throw away our buffer and loop.  Note
	// that the loop causes use to call sysctl with NULL again; this
	// is necessary because the ENOMEM failure case sets length to
	// the amount of data returned, not the amount of data that
	// could have been returned.

	result = NULL;
	done = false;

	do {
		xbox_assert(result == NULL);

		// Call sysctl with a NULL buffer.

		length = 0;
		err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
			NULL, &length,
			NULL, 0);
		if (err == -1) {
			err = errno;
		}

		// Allocate an appropriately sized buffer based on the results
		// from the previous call.

		if (err == 0) {
			result = (kinfo_proc *)malloc(length);
			if (result == NULL) {
				err = ENOMEM;
			}
		}

		// Call sysctl again with the new buffer.  If we get an ENOMEM
		// error, toss away our buffer and start again.

		if (err == 0) {
			err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
				result, &length,
				NULL, 0);
			if (err == -1) {
				err = errno;
			}
			if (err == 0) {
				done = true;
			} else if (err == ENOMEM) {
				xbox_assert(result != NULL);
				free(result);
				result = NULL;
				err = 0;
			}
		}
	} while (err == 0 && ! done);

	// Clean up and establish post conditions.

	if (err != 0 && result != NULL)
	{
		free(result);
		result = NULL;
	}

	procList = result;

	if (err == 0)
	{
		procCount = length / sizeof(kinfo_proc);
	}

	for (unsigned int i = 0; i < procCount; ++i)
	{
		if ( procList[i].kp_proc.p_pid == inProcessID )
			outProcessName = procList[i].kp_proc.p_comm;
	}

	free(procList);
}

void XMacSystem::GetProcessPath(uLONG inProcessID, XBOX::VString& outProcessPath)
{
	int		mib[3], argmax = 0;
	size_t	size = sizeof(argmax);
	char*	args;
	
	mib[0] = CTL_KERN;
	mib[1] = KERN_ARGMAX;
	
	if (sysctl(mib, 2, &argmax, &size, NULL, 0) != -1)
	{
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROCARGS2;
		mib[2] = inProcessID;
		
		args = (char *)malloc(argmax);
		size = (size_t)argmax;
		if (sysctl(mib, 3, args, &size, NULL, 0) != -1)
			outProcessPath = args + sizeof(argmax);
		free(args);
	}
}

bool XMacSystem::KillProcess(uLONG inProcessID)
{
	bool result = false;

	if ( inProcessID )
		if (kill(inProcessID, SIGKILL) != -1)
			result = true;

	return result;
}


sLONG XMacSystem::GetNumberOfProcessors()
{
	int count;
	size_t size = sizeof(count);
	if (::sysctlbyname("hw.ncpu", &count, &size, NULL, 0))
		return 1;
	return (count <= 0) ? 1 : (sLONG) count;
}


void XMacSystem::Delay( sLONG inMilliseconds)
{
	//You should not call this function with high values (>1s)
	xbox_assert(inMilliseconds<=1000);

	struct timespec ts;

	ts.tv_sec = inMilliseconds/1000;
	ts.tv_nsec = (inMilliseconds % 1000) * 1000000;

	// "If nanosleep() returns due to the delivery of a signal, the value returned will be the -1"
	while( nanosleep(&ts, &ts) == -1)
	{
		xbox_assert( errno == EINTR );	//an error occured
	}
}

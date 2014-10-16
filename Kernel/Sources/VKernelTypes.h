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
#ifndef __VKernelTypes__
#define __VKernelTypes__

#include "Kernel/Sources/VTypes.h"

BEGIN_TOOLBOX_NAMESPACE

// XToolbox signatures
const OsType	kXTOOLBOX_SIGNATURE	= 'xbox';	// Used for VErrors and callback signature
const OsType	kOBJECT_PROPERTY	= 'this';	// Used for storing VObjects* as native Refcon and properties


// A VError is a 64 bits integer.
// The high 32bits word is the component signature (0 means 'system')
//
typedef uLONG8 VError;

const OsType	kCOMPONENT_SYSTEM	= 0;
const OsType	kCOMPONENT_XTOOLBOX	= kXTOOLBOX_SIGNATURE;
const OsType	kCOMPONENT_MACH		= 'MACH';	// for OSX Mach subsystem (kern_return_t)
const OsType	kCOMPONENT_POSIX	= 'POSX';	// for posix/bsd subsystem (mac, linux, win)

// Utility macros for creating or using VErrors
#define MAKE_VERROR(inComponentSignature, inErrCode) ((XBOX::VError) ((((uLONG8) inComponentSignature)<<32)|((uLONG)inErrCode)) )
#define ERRCODE_FROM_VERROR(inError) ((sLONG) ((inError) & 0xFFFFFFFF))
#define COMPONENT_FROM_VERROR(inError) ((OsType) ((inError) >> 32))

// check that the component part is kCOMPONENT_SYSTEM
// native errors should never be thrown.
#define IS_NATIVE_VERROR(inError)			( ((inError) >> 32 == 0) && (((inError) & 0xFFFFFFFF)) )
#define MAKE_NATIVE_VERROR(inNativeError)	( ((XBOX::VError) inNativeError) & 0xFFFFFFFF)
#define NATIVE_ERRCODE_FROM_VERROR(inError) ((VNativeError) ((inError) & 0xFFFFFFFF))

// gcc supports int64 enums but not visual studio
#if COMPIL_GCC
#define DECLARE_VERROR(inComponentSignature, inErrCode, inName) enum {inName = ((XBOX::VError) (((uLONG8) inComponentSignature<<32)|(uLONG)inErrCode) )};
#else
#define DECLARE_VERROR(inComponentSignature, inErrCode, inName) const VError inName = ((XBOX::VError) (((uLONG8) inComponentSignature<<32)|(uLONG)inErrCode) );
#endif


// Utility constants
const sLONG8	SECONDS_IN_A_DAY	= 86400;
const sWORD		NEW_RES_ID		= -1;

#define MAX_NUM_PRECISION	-1


#if VERSIONMAC
	const char	FOLDER_SEPARATOR	= ':';
#elif VERSION_LINUX
    const char FOLDER_SEPARATOR     = '/';
#elif VERSIONWIN
	const char	FOLDER_SEPARATOR	= 0x5C;
#endif

const char ALLFILEEXT = '*';

typedef enum KindPlatform
{
	KP_MAC = 0,
    KP_LINUX,
	KP_WIN,
	KP_VIRTUAL,
#if VERSIONMAC
	KP_NATIVE = KP_MAC
#elif VERSION_LINUX
    KP_NATIVE = KP_LINUX
#elif VERSIONWIN
	KP_NATIVE = KP_WIN
#endif
} KindPlatform;

/*
typedef enum PackMsg
{
	PackMsgServerDeInit	= -220,
	PackMsgServerDisposeData	= -207,
	PackMsgServerWriteData	= -206,
	PackMsgServerReadData	= -205,
	PackMsgServerKillClient	= -202,
	PackMsgServerNewClient	= -201,
	PackMsgServerInit	= -200,
	PackMsgProcessDeInit	= -4,
	PackMsgProcessInit	= -3,
	PackMsgClientDeInit	= -2,
	PackMsgDeInit	= PackMsgClientDeInit,
	PackMsgClientInit	= -1,
	PackMsgInit	= PackMsgClientInit
} PackMsg;
*/

typedef enum TaskState
{
	TS_CREATING = 0,// the very beginning, not ready to run
	TS_RUN_PENDING,	// the task waits for the OS to launch it
	TS_RUNNING,		// everything's ok
	TS_DYING,		// someone asked the task to stop
	TS_DEAD			// the task is dead but the VTask is waiting to be deleted
} TaskState;

typedef sLONG VTaskID;

const VTaskID NULL_TASK_ID = 0;


typedef enum ListDestroy
{
	DESTROY = 0,
	NO_DESTROY,
	DESTROY_IF_OWNER
} ListDestroy;


typedef enum CompareMethod
{
	CM_SMALLER_OR_EQUAL = -2,
	CM_SMALLER,
	CM_EQUAL,
	CM_BIGGER,
	CM_BIGGER_OR_EQUAL
} CompareMethod;


typedef enum CompareResult
{
	CR_UNRELEVANT = -1000,
	CR_SMALLER = -1,
	CR_EQUAL,
	CR_BIGGER,
	CR_EQUAL_NULL_LEFT,
	CR_EQUAL_NULL_RIGHT
} CompareResult;


#if VERSIONWIN

// ****IMPORTANT****
// don't compare the values of this enum but use instead VSystem::IsSystemVersionOrAbove
// If you add some value in this enum remember to update XWinSystem::GetMajorAndMinorFromSystemVersion
typedef enum SystemVersion
{
	WIN_UNKNOWN=0,
	WIN_XP,
	WIN_VISTA,
	WIN_SERVER_2003,
	WIN_SERVER_2008,
	WIN_SERVER_2008R2,
	WIN_SEVEN,
	WIN_EIGHT,
	WIN_EIGHT_ONE,
	WIN_SERVER_2012,
	WIN_SERVER_2012R2
} SystemVersion;

#endif	// VERSIONWIN


#if VERSIONMAC

typedef enum SystemVersion
{
	MAC_UNKNOWN		= 0,
	MAC_CLASSIC		= 1,
	MAC_OSX			= 2,
	MAC_OSX_10_0	= 2,
	MAC_OSX_10_1	= 3,
	MAC_OSX_10_2	= 4,
	MAC_OSX_10_3	= 5,
	MAC_OSX_10_4	= 6,
	MAC_OSX_10_5	= 7,
	MAC_OSX_10_6	= 8,
	MAC_OSX_10_7	= 9,
	MAC_OSX_10_9	= 11
} SystemVersion;

#endif	// VERSIONMAC


#if VERSION_LINUX

typedef enum SystemVersion
{
	LINUX_UNKNOWN		= 0,
    LINUX_2_6
} SystemVersion;

#endif	// VERSION_LINUX
	
typedef enum CpuKind
{
	CPU_SAME = -3,
	CPU_SWAP,
	CPU_AUTO,
	CPU_68K,
	CPU_PPC,
	CPU_X86,
#if BIGENDIAN
	CPU_NATIVE = CPU_PPC
#else
	CPU_NATIVE = CPU_X86
#endif
} CpuKind;


typedef enum EURLPathStyle
{
	eURL_POSIX_STYLE = 0,
	eURL_HFS_STYLE, 
	eURL_WIN_STYLE,
#if VERSIONMAC
	eURL_NATIVE_STYLE = eURL_HFS_STYLE
#elif VERSION_LINUX
    eURL_NATIVE_STYLE = eURL_POSIX_STYLE
#elif VERSIONWIN
	eURL_NATIVE_STYLE = eURL_WIN_STYLE
#endif
} EURLPathStyle;


typedef enum EURLParts
{
	e_Scheme = 0,
	e_NetLoc,
	e_Path,
	e_Params,
	e_Query,
	e_Fragment,
	e_LastPart
} EURLParts;


typedef struct URLPart
{
	sLONG	pos;
	sLONG	size;
} URLPart;


typedef enum ECarriageReturnMode
{
	eCRM_NONE = 1,
	eCRM_CRLF,
	eCRM_CR,
	eCRM_LF,
	eCRM_LAST,

#if VERSIONMAC
	eCRM_NATIVE = eCRM_CR
#elif VERSION_LINUX
    eCRM_NATIVE = eCRM_LF
#elif VERSIONWIN
	eCRM_NATIVE = eCRM_CRLF
#endif

} ECarriageReturnMode;

typedef enum FilePathStyle
{
	FPS_SYSTEM = 0,
	FPS_POSIX,
#if !VERSION_LINUX
	FPS_DEFAULT=FPS_SYSTEM
#else
	FPS_DEFAULT=FPS_POSIX
#endif
} FilePathStyle;

typedef enum FileAccess
{
	FA_READ = 0,	// Read access - Read/Write share
	FA_READ_WRITE,	// Exclusive Read/Write access
	FA_SHARED,		// Read/Write access - Read/Write share
	FA_MAX			// Tries Read/Write access with shared Read then FA_READ if failed
} FileAccess;


typedef uLONG FileOpenOptions;	// options for VFile::Open
enum {
	FO_OpenResourceFork		= 1,	// Open resource fork ( ":AFP_Resource" NTFS sream on Windows)
	FO_CreateIfNotFound		= 2,	// Create the file if it doesn't exist
	FO_Overwrite			= 4,	// If file already exist, truncate it (needs FA_WRITE)
	FO_SequentialScan		= 8,	// The file is to be accessed sequentially from beginning to end
	FO_RandomAccess			= 16,	// The file is to be accessed randomly
	FO_WriteThrough			= 32,	// The data is written to the system cache, but is flushed to disk without delay
	FO_Default				= FO_SequentialScan		// file must exist
};

typedef uLONG FileCreateOptions;	// options for VFile::Create
enum {
	FCR_Overwrite			= 4,	// overwrite destination file
	FCR_Default				= 0
};

typedef uLONG FileCopyOptions;	// options for VFile::Copy and VFile::Move
enum {
	FCP_Overwrite			= 4,	// overwrite destination file
	FCP_ContinueOnError		= 8,	// while copying multiple files, tells one should continue copying remaining files.
	FCP_SkipInvisibles		= 16,	// don't copy invisible files
	FCP_Default				= 0
};


typedef enum FolderFilter
{
	FF_NO_FILTER = 0,
	FF_NO_FOLDERS,
	FF_NO_FILES
} FolderFilter;


typedef uLONG FileIteratorOptions;
enum {
	FI_WANT_NONE		= 0,
	FI_RECURSIVE		= 1,
	FI_WANT_FOLDERS		= 2,
	FI_WANT_FILES		= 4,
	FI_WANT_INVISIBLES	= 8,
	FI_RESOLVE_ALIASES	= 16,
	FI_ITERATE_DELETE	= 32, /* for mac only : to use if you want to iterate some file/folder that you want to delate ( during the iterate ) */
	FI_WANT_ALL	= FI_WANT_FILES | FI_WANT_FOLDERS | FI_WANT_INVISIBLES,
	FI_NORMAL_FILES	= FI_WANT_FILES | FI_RESOLVE_ALIASES,
	FI_NORMAL_FOLDERS	= FI_WANT_FOLDERS | FI_RESOLVE_ALIASES
};


typedef enum DateOrder
{
	DO_MONTH_DAY_YEAR = 0,
	DO_DAY_MONTH_YEAR,
	DO_YEAR_MONTH_DAY,
	DO_MONTH_YEAR_DAY,
	DO_DAY_YEAR_MONTH,
	DO_YEAR_DAY_MONTH
} DateOrder;


typedef enum MultiByteCode
{
	SC_UNKNOWN = -1,
	SC_ROMAN,
	SC_JAPANESE,
	SC_TRADCHINESE,
	SC_SIMPCHINESE,
	SC_KOREAN,
	SC_ARABIC,
	SC_HEBREW,
	SC_GREEK,
	SC_CYRILLIC,
	SC_CENTRALEUROROMAN,
	SC_CROATIAN,
	SC_ICELANDIC,
	SC_FARSI,
	SC_ROMANIAN,
	SC_THAI,
	SC_VIETNAMESE,
	SC_TURKISH
} MultiByteCode;

typedef enum EForkType
{
	eDataFork,
	eRezFork,
	eAllFork
}EForkType;

typedef uLONG EFileAttributes;
enum {
	FATT_LockedFile = 1,
	FATT_HidenFile = 2
};


typedef uLONG VMStatus;	/** @brief status of a contiguous range of virtual memory pages **/
enum {
	VMS_FreeMemory		= 1
};


typedef uLONG XMLStringOptions;
//constants to format VTime to xml time or datetime string with timezone or not
//for instance, XSO_Time_UTC will translate to xml datetime format with GMT+0 timezone
//				XSO_Time_Local will translate to xml datetime format with local timezone 
//				XSO_Time_TimeOnly|XSO_Time_NoTimezone will translate to xml time format without timezone (local time)
//(note : if neither XSO_UTC neither XSO_Local, add local timezone)
enum {
	XSO_Time_Local				= 0,	//use local timezone (for instance 2009-02-17T13:25:00+01:00)
	XSO_Time_UTC				= 1,	//use GMT+0 timezone (for instance 2009-02-17T12:25:00Z)
	XSO_Time_NoTimezone			= 2,	//do not write timezone (local datetime or time but timezone dependant)(for instance 2009-02-17T13:25:00)
	XSO_Time_TimeOnly			= 4,	//force xml time format (otherwise use xml datetime format)(for instance 13:25:00)
	XSO_Time_DateOnly			= 8,	//force xml date format (otherwise use xml datetime format)(for instance 2009-02-17)
	XSO_Time_DateNull			= 16,   //force null date - for datetime formats only (for instance 0000-00-00T13:25:00)

	XSO_Duration_ISO			= 1,    //format duration as xml duration (for instance P0DT13H25M00S)
	XSO_Duration_Seconds		= 0,    //format duration as number of seconds (for instance 48300)

	XSO_Default = XSO_Time_UTC			//equal to XSO_Duration_ISO too
};

typedef enum NormalizationMode
{
	/*
		same values as ICU
	*/
	
  /** No decomposition/composition. */
  NM_NONE = 1, 
  /** Canonical decomposition. */
  NM_NFD = 2,
  /** Compatibility decomposition. */
  NM_NFKD = 3,
  /** Canonical decomposition followed by canonical composition. */
  NM_NFC = 4,
  /** Compatibility decomposition followed by canonical composition. */
  NM_NFKC = 5,
  /** "Fast C or D" form. */
  NM_FCD = 6
} NormalizationMode;


typedef uLONG EDisplayNotificationOptions;
enum {
	// buttons layout: select one among these
	// buttons layout
	EDN_OK					= 0,	// this is the default
	EDN_OK_Cancel			= 1,
	EDN_Yes_No				= 2,
	EDN_Yes_No_Cancel		= 4,
	EDN_Abort_Retry_Ignore	= 5,
	EDN_LayoutMask			= 15,
	
	// only of these
	EDN_Default1			= 16,		// first button is the default
	EDN_Default2			= 32,		// second button is the default
	EDN_Default3			= 48,		// third button is the default
	EDN_DefaultMask			= 48,
	
	EDN_StyleCritical		= 512
};

typedef uLONG ENotificationResponse;
enum {
	ERN_None		= 0,
	ERN_OK			= 1,
	ERN_Cancel		= 2,
	ERN_Yes			= 3,
	ERN_No			= 4,
	ERN_Abort		= 5,
	ERN_Retry		= 6,
	ERN_Ignore		= 7,

	ERN_Timeout		= 8		// notification closed because of timeout
};


/*
	Logging message level for ILogger
*/
typedef uLONG EMessageLevel;
enum
{
	EML_Dump,			// Read/Write IO buffers for network operations. Logs only (and not all the time !)
	EML_Trace,			// More detailed information. Expect these to be written to logs only.
	EML_Assert,			// A failed assertion.
	EML_Debug,			// Detailed information on the flow through the system. Expect these to be written to logs only.
	EML_Information,	// Interesting runtime events (startup/shutdown). Expect these to be immediately visible on a console, so be conservative and keep to a minimum.
	EML_Warning,		// Use of deprecated APIs, poor use of API, 'almost' errors, other runtime situations that are undesirable or unexpected, but not necessarily "wrong". Expect these to be immediately visible on a status console.
	EML_Error,			// Other runtime errors or unexpected conditions. Expect these to be immediately visible on a status console.
	EML_Fatal			// Severe errors that cause premature termination. Expect these to be immediately visible on a status console.
};

//enable enhanced text document & layout support
#define ENABLE_VDOCTEXTLAYOUT		1


#define kDOC_VERSION_XHTML11		0		//XHTML 1.1 format (default if not 4D SPAN or 4D XHTML text)

#define kDOC_VERSION_SPAN4D			1		//v13 span format (4D SPAN text): on Windows stored point font-size value is equal to 4D form font size*72/screen dpi

#define kDOC_VERSION_XHTML4D		2		//V15 XHMTL 4D document format: full support for new XHTML4D document format (should be fully implemented only for v15+)
											
#define kDOC_VERSION_MAX			kDOC_VERSION_XHTML4D		

#if ENABLE_VDOCTEXTLAYOUT
#define kDOC_VERSION_MAX_SUPPORTED	kDOC_VERSION_XHTML4D 
#else
#define kDOC_VERSION_MAX_SUPPORTED	kDOC_VERSION_SPAN4D 
#endif

END_TOOLBOX_NAMESPACE

#endif

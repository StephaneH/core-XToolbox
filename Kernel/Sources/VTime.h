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
#ifndef __VTime__
#define __VTime__

#include "Kernel/Sources/VValueSingle.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VDuration : public VValueSingle
{
public:
	typedef VValueInfo_scalar<VDuration,VK_DURATION,sLONG8>	InfoType;
	static	const InfoType	sInfo;

	VDuration():fMilliseconds( 0)	{;}
	VDuration (const VDuration& inOriginal);
	
	explicit	VDuration (sLONG8 inMilliseconds);
	explicit	VDuration (sLONG8* inDataPtr, bool inInit);

	// Duration support
	void	GetDuration (sLONG& outDay, sLONG& outHour, sLONG& outMinute, sLONG& outSecond, sLONG& outMillisecond) const;
	void	SetDuration (sLONG inDay, sLONG inHour, sLONG inMinute, sLONG inSecond, sLONG inMillisecond);
	void	AddDuration (sLONG inDay, sLONG inHour, sLONG inMinute, sLONG inSecond, sLONG inMillisecond);
	void	Add (const VDuration& inDuration);
	void	Substract (const VDuration& inDuration);
	void	Multiply (Real inValue);
	void	Divide (Real inValue);
	Real	Divide (const VDuration& inDuration) const;
	void	Remainder (const VDuration& inDuration);

	sLONG8	ConvertToHours () const;
	sLONG8	ConvertToMinutes () const;
	sLONG8	ConvertToSeconds () const;
	sLONG8 	ConvertToMilliseconds () const;

	void	FromNbDays (sLONG8 inNbDays);
	void	FromNbHours (sLONG8 inNbHours);
	void	FromNbMinutes (sLONG8 inNbMinutes);
	void	FromNbSeconds (sLONG8 inNbSeconds);
	void	FromNbMilliseconds (sLONG8 inNbMilliseconds);

	sLONG	GetDay () const;
	sLONG	GetHour () const;
	sLONG	GetMinute () const;
	sLONG	GetSecond () const;
	sLONG	GetMillisecond () const;

	// Accessors
	void	AdjustDay (sLONG inDay);
	void	AdjustHour (sLONG inHour);
	void	AdjustMinute (sLONG inMinute);
	void	AdjustSecond (sLONG inSecond);
	void	AdjustMillisecond (sLONG inMillisecond);

	void	AddDays (sLONG inDay);
	void	AddHours (sLONG inHour);
	void	AddMinutes (sLONG inMinute);
	void	AddSeconds (sLONG inSecond);
	void	AddMilliseconds (sLONG8 inMillisecond);

	Real	GetDecimalHour () const;
	void	FromDecimalHour (Real inDecHour);
	void	AddDecimalHour (Real inDeccHour);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo () const;
	virtual Boolean	CanBeEvaluated () const { return true; };
	virtual void	Clear ();

	virtual Boolean	GetBoolean () const;
	virtual sBYTE	GetByte () const;
	virtual sWORD	GetWord () const;
	virtual sLONG	GetLong () const;
	virtual sLONG8	GetLong8 () const;
	virtual Real	GetReal () const;
	virtual void	GetFloat (VFloat& outValue) const;
	virtual void	GetDuration (VDuration& outValue) const;
	virtual void	GetString (VString& outValue) const;
	virtual void	GetValue (VValueSingle& outValue) const;

	virtual void	FromBoolean (Boolean inValue); 
	virtual void	FromByte (sBYTE inValue); 
	virtual void	FromWord (sWORD inValue);
	virtual void	FromLong (sLONG inValue);
	virtual void	FromLong8 (sLONG8 inValue);
	virtual void	FromReal (Real inValue);
	virtual void	FromFloat (const VFloat& inValue);
	virtual void	FromDuration (const VDuration& inValue);
	virtual void	FromString (const VString& inValue);
	virtual void	FromValue (const VValueSingle& inValue);

	virtual void*	LoadFromPtr (const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const { return sizeof(sLONG8); };

	virtual CompareResult	CompareToSameKind (const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean	EqualToSameKind (const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean	FromValueSameKind (const VValue* inValue);

	virtual CompareResult	CompareToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean	EqualToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareTo (const VValueSingle& inValue,Boolean inDiacritic = false) const ;
			CompareResult		CompareTo (const VDuration& inTime) const;

	virtual VError				FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError				GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue( VJSONValue& outJSONValue) const;

	/** convert to xml duration string : PdddDTddHddMdd[.dddddd]S (where d is digit)  (XSO_Duration_ISO)
										 or
										 number of seconds as long integer (XSO_Duration_Seconds)

	*/
	virtual	void				GetXMLString( VString& outString, XMLStringOptions inOptions) const;

	/** convert from xml duration string : P[00Y][00M]00DT00H00M00.000S 
										   or
										   number of seconds (long integer)
										   (or  
										   DDD:HH:MM:SS:MS (for backward compatibility))
	@remark
		as duration is a relative time interval,
		the number of days per month is undeterminate so
		it is assumed to be 30 
	*/
	virtual	bool				FromXMLString( const VString& inString);

	bool operator == (const VDuration &inDuration) const;
	bool operator > (const VDuration &inDuration) const;
	bool operator < (const VDuration &inDuration) const;
	bool operator >= (const VDuration &inDuration) const;
	bool operator <= (const VDuration &inDuration) const;
	bool operator != (const VDuration &inDuration) const;

	VDuration operator = (const VDuration& inValue);
	virtual VDuration*	Clone () const { return new VDuration(*this); };

protected:
	sLONG8	fMilliseconds;
};


typedef enum VTimeInitParam {
	eInitWithZero,
	eInitWithCurrentTime
} VTimeInitParam;

class XTOOLBOX_API VTime : public VValueSingle
{
public:
	typedef VValueInfo_scalar<VTime,VK_TIME,uLONG8>	InfoType;
	static	const InfoType	sInfo;

				VTime():fYear( 0), fMonth( 0), fDay( 0), fMilliseconds( 0)	{;}
				VTime (const VTime& inOriginal);
	
	explicit	VTime (uLONG8 inTimeStamp);
	explicit	VTime (uLONG8* inDataPtr, bool inInit);
	explicit	VTime (VTimeInitParam inInitMethod);

	// Time support
	void	Add (const VDuration& inDuration);
	void	Substract (const VTime& inTime, VDuration& outDuration) const;
	void	Substract (const VDuration& inDuration);

	sLONG8	GetMilliseconds () const;
	void	FromMilliseconds (sLONG8 inMilliseconds);

	void	GetLocalTime (sWORD& outYear, sWORD& outMonth, sWORD& outDay, sWORD& outHour, sWORD& outMinute, sWORD& outSecond, sWORD& outMillisecond) const;
	void	FromLocalTime (sWORD inYear, sWORD inMonth, sWORD inDay, sWORD inHour, sWORD inMinute, sWORD inSecond, sWORD inMillisecond);
	
	void	GetUTCTime (sWORD& outYear, sWORD& outMonth, sWORD& outDay, sWORD& outHour, sWORD& outMinute, sWORD& outSecond, sWORD& outMillisecond) const;
	void	FromUTCTime (sWORD inYear, sWORD inMonth, sWORD inDay, sWORD inHour, sWORD inMinute, sWORD inSecond, sWORD inMillisecond);

	sLONG	GetWeekDay () const;	// 0 (Sunday) to 6 (Saturday)
	sLONG	GetYearDay () const;
	void	FromSystemTime (); /* UTC */
	uLONG8	GetStamp () const;
	void	FromStamp (uLONG8 inValue);

	void	AddYears (sLONG inYear);
	void	AddMonths (sLONG inMonth);
	void	AddDays (sLONG inDay);
	void	AddHours (sLONG inHour);
	void	AddMinutes (sLONG inMinute);
	void	AddSeconds (sLONG inSecond);
	void	AddMilliseconds (sLONG8 inMillisecond);

	void	NullIfZero();

	sWORD	GetLocalYear () const;
	sWORD	GetLocalMonth () const;
	sWORD	GetLocalDay () const;
	sWORD	GetLocalHour () const;
	sWORD	GetLocalMinute () const;
	sWORD	GetLocalSecond () const;
	sWORD	GetLocalMillisecond () const;
	
	VDuration	GetLocalDurationSinceMidnight() const;

	void	SetLocalYear (sLONG inYear);
	void	SetLocalMonth (sLONG inMonth);
	void	SetLocalDay (sLONG inDay);
	void	SetLocalHour (sLONG inHour);
	void	SetLocalMinute (sLONG inMinute);
	void	SetLocalSecond (sLONG inSecond);
	void	SetLocalMillisecond (sLONG inMillisecond);

	sLONG	GetJulianDay () const;
	void	FromJulianDay (sLONG inJulianDay);
	void	GetJulianDate (sWORD& outYear, sWORD& outMonth, sWORD& outDay) const;
	void	FromJulianDate (sWORD inYear, sWORD inMonth, sWORD inDay);

	static	sLONG	ComputeJulianDay( sLONG inYear, sLONG inMonth, sLONG inDay);
	static	sLONG	ComputeWeekDay( sLONG inYear, sLONG inMonth, sLONG inDay);	// 0 (Sunday) to 6 (Saturday)
	static	sLONG	ComputeYearDay( sLONG inYear, sLONG inMonth, sLONG inDay);	// 1 = January 1st

	void	toSimpleDateString(VString& outString) const;

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo () const;
	virtual Boolean	CanBeEvaluated () const { return true; };
	virtual void	Clear ();

	virtual void	GetTime (VTime& outValue) const;
	virtual void	GetString( VString& outValue) const;
	virtual Real	GetReal() const;
	virtual void	GetValue (VValueSingle& outValue) const;

			void	GetStringLocalTime (VString& outValue) const;
			void	FormatByOS( VString& outValue, bool useGMT) const;
			void	FormatByOS(VString& outDateValue,VString& outTimeValue,bool useGMT)const;

	virtual void	FromTime (const VTime& inValue);
	virtual void	FromString (const VString& inValue);
	virtual void	FromValue (const VValueSingle& inValue);

			void	FromCompilerDateTime (const char* inDate, const char* inTime);
			void	FromToday( sLONG inMillisecondsSinceMidnight);
			
	/** translate to xml datetime or date or time string 
		(with UTC or local timezone or without timezone: see XMLStringOptions)
	*/
	virtual	bool	FromXMLString( const VString& inString);

	/**
		translate from xml datetime or date or time string (full xml compliancy for optimal compatibility)
	*/
	virtual	void	GetXMLString( VString& outString, XMLStringOptions inOptions) const;
		
	/*
		partial RFC 3339 support (seconds fractional part is truncated):
		output UTC: 2006-12-04T00:00:00[Z|[+01:00]]? 
					or 
					00:00:00[Z|[+01:00]]?
					or
					2006-12-04[Z|[+01:00]]?
	@see 
		XMLStringOptions
	*/
			void	GetRfc3339String( VString& outString, XMLStringOptions inOptions) const;

	/**
		input:	2006-12-04T00:00:00[.000]?[Z|[+01:00]]? 
				or 
				00:00[:00[.000]?]?[Z|[+01:00]]?
				or
				2006-12-04[Z|[+01:00]]? 
	*/
			bool	FromRfc3339String( const VString& inString);

			void	FromRfc822String(XBOX::VString &inStr);
			void	GetRfc822String( VString& outString) const;

	virtual void*	LoadFromPtr (const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const { return sizeof(sLONG8); };

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

	virtual CompareResult	CompareToSameKind (const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean	EqualToSameKind (const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean	FromValueSameKind (const VValue* inValue);

	virtual CompareResult	CompareToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean	EqualToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareTo (const VValueSingle& inValue,Boolean inDiacritic = false) const ;
			CompareResult		CompareTo (const VTime& inTime) const;

	virtual VError				FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError				GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue( VJSONValue& outJSONValue) const;

	bool operator == (const VTime &inTime) const;
	bool operator > (const VTime &inTime) const;
	bool operator < (const VTime &inTime) const;
	bool operator >= (const VTime &inTime) const;
	bool operator <= (const VTime &inTime) const;
	bool operator != (const VTime &inTime) const;

	VTime operator = (const VTime& inValue);
	virtual VTime*	Clone () const { return new VTime(*this); };

	// Utilities
	static bool	CheckDate (sWORD inYear, sWORD inMonth, sWORD inDay);
	static bool	IsLeapYear (sWORD inYear);
	static void	Now(VTime &outTime);

protected:
	sWORD	fYear;
	sBYTE	fMonth;
	sBYTE	fDay;
	sLONG	fMilliseconds;
};

END_TOOLBOX_NAMESPACE

#endif

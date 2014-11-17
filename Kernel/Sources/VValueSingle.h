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
#ifndef __VValueSingle__
#define __VValueSingle__

#include "Kernel/Sources/VValue.h"
#include "Kernel/Sources/VByteSwap.h"

BEGIN_TOOLBOX_NAMESPACE

// defined below
class VFloat;
class VString;
class VUUID;
class VTime;
class VDuration;
class VJSONValue;

class XTOOLBOX_API VValueSingle : public VValue
{
typedef VValue	inherited;
public:
								VValueSingle( bool inIsNull = false):inherited(inIsNull)			{;}
								VValueSingle( const VValueSingle& inOther):inherited(inOther)		{;}

#if VERSIONDEBUG
	virtual						~VValueSingle();
#endif

	// Inherited from VValue
	virtual const VValueSingle*	IsVValueSingle() const							{ return this; }
	virtual VValueSingle*		IsVValueSingle()								{ return this; }
	virtual VValueSingle*		Clone() const = 0;

	// Value support
	virtual	Boolean				GetBoolean() const								{ return false; }
	virtual sBYTE				GetByte() const									{ return 0; }
	virtual sWORD				GetWord() const									{ return 0; }
	virtual sLONG				GetLong() const									{ return 0; }
	virtual sLONG8				GetLong8() const								{ return 0; }
	virtual Real				GetReal() const									{ return 0.0; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetTime( VTime& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetVUUID( VUUID& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean /*inValue*/)				{}
	virtual void				FromByte( sBYTE /*inValue*/)					{}
	virtual void				FromWord( sWORD /*inValue*/)					{}
	virtual void				FromLong( sLONG /*inValue*/)					{}
	virtual void				FromLong8( sLONG8 /*inValue*/)					{}
	virtual void				FromReal( Real /*inValue*/)						{}
	virtual void				FromFloat( const VFloat& /*inValue*/)			{}
	virtual void				FromDuration( const VDuration& /*inValue*/)		{}
	virtual void				FromTime( const VTime& /*inValue*/)				{}
	virtual void				FromString( const VString& /*inValue*/)			{}
	virtual void				FromVUUID( const VUUID& /*inValue*/)			{}
	virtual void				FromValue( const VValueSingle& /*inValue*/)		{}

	/** @brief XML compliant export	**/
	virtual	void				GetXMLString( VString& outString, XMLStringOptions inOptions) const;

	/** @brief XML compliant import	**/
	/** Returns true if the string was correctly formatted and produced a valid value.	**/
	virtual	bool				FromXMLString( const VString& inString);

	// convert to and from json.
	// warning: doesn't handle null.
	virtual VError				FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError				GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual VError				FromJSONValue(const VJSONValue& inJSONValue);
	virtual VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	// Utilities
	virtual VValueSingle*		ConvertTo( sLONG inWhatType) const;
	virtual CompareResult		CompareTo( const VValueSingle& /*inValue*/, Boolean inDiacritical = false) const = 0;
	virtual Boolean				EqualTo( const VValueSingle& inValue, Boolean inDiacritical) const;

	// Operators
	Boolean	operator ==( const VValueSingle& inValue) const						{ return EqualTo(inValue, false); }
	Boolean	operator !=( const VValueSingle& inValue) const						{ return !EqualTo(inValue, false); }
	Boolean	operator >( const VValueSingle& inValue) const						{ return CompareTo(inValue, false) == CR_BIGGER; }
	Boolean	operator <( const VValueSingle& inValue) const						{ return CompareTo(inValue, false) == CR_SMALLER; }
	Boolean	operator >=( const VValueSingle& inValue) const						{ return CompareTo(inValue, false) >= CR_EQUAL; }
	Boolean	operator <=( const VValueSingle& inValue) const						{ return CompareTo(inValue, false) <= CR_EQUAL; }

	// Inherited from VObject
	virtual CharSet				DebugDump( char* inTextBuffer, sLONG& inBufferSize) const;
};


class VBlob;

template<class VVALUE_CLASS, ValueKind VALUE_KIND, class BACKSTORE_TYPE>
class VValueInfo_scalar : public VValueInfo
{
public:
	typedef VVALUE_CLASS	vvalue_class;
	typedef BACKSTORE_TYPE	backstore_type;

	VValueInfo_scalar():VValueInfo( VALUE_KIND)
		{
			;
		}

	virtual ~VValueInfo_scalar()	{;}

	virtual	VValue* Generate() const
		{
			return new VVALUE_CLASS;
		}

	virtual	VValue* Generate(VBlob* /*inBlob*/) const
	{
		return new VVALUE_CLASS;
	}

	virtual	VValue* LoadFromPtr( const void *ioBackStore, bool /*inRefOnly*/) const
		{
			return new VVALUE_CLASS( *(BACKSTORE_TYPE *) ioBackStore);
		}

	virtual void* ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const
		{
			ByteSwap(( BACKSTORE_TYPE *) inBackStore);
			return ((BACKSTORE_TYPE *)inBackStore) + 1;
		}

	virtual CompareResult CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const
		{
			if ( VALUE_KIND == VK_REAL && !inDiacritical )
			{
				/* Explicit specification did not work on Mac. I can't figure out how to do it on Mac, hence the stupid 'if'. */
				if ( fabs ( *((Real*)inPtrToValueData1) - *((Real*)inPtrToValueData2) ) < kDefaultEpsilon )
					return CR_EQUAL;
				else if (*((Real*)inPtrToValueData1)> *((Real*)inPtrToValueData2) )
					return CR_BIGGER;
				else
					return CR_SMALLER;
			}
			else
			{
				if( *((BACKSTORE_TYPE*)inPtrToValueData1) == *((BACKSTORE_TYPE*)inPtrToValueData2))
					return CR_EQUAL;
				else if( *((BACKSTORE_TYPE*)inPtrToValueData1) > *((BACKSTORE_TYPE*)inPtrToValueData2))
					return CR_BIGGER;
				else
					return CR_SMALLER;
			}
		}

	virtual CompareResult CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& /*inOptions*/) const
		{
			if( *((BACKSTORE_TYPE*)inPtrToValueData1) == *((BACKSTORE_TYPE*)inPtrToValueData2))
				return CR_EQUAL;
			else if( *((BACKSTORE_TYPE*)inPtrToValueData1) > *((BACKSTORE_TYPE*)inPtrToValueData2))
				return CR_BIGGER;
			else
				return CR_SMALLER;
		}

	virtual CompareResult CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const
		{
			return CompareTwoPtrToData( inPtrToValueData1, inPtrToValueData2, inDiacritical);
		}

	virtual VSize GetSizeOfValueDataPtr( const void* /*inPtrToValueData*/) const
		{
			return sizeof( BACKSTORE_TYPE);
		}

	virtual	VSize GetAvgSpace() const
		{
			return sizeof( BACKSTORE_TYPE);
		}
};

/*
Do the epsilon stuff only if diacritical is false.
*/
/*class VReal;

This explicit specification does not work on Mac. It complains about the method being virtual outside of the class definition.
If virtual is removed (and/or from the class itself), then there are linkage problems with multiple definitions for
the same symbol.

template< >	virtual CompareResult VValueInfo_scalar<VReal, VK_REAL, Real>::CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical) const
		{
			if ( inDiacritical )
			{
				if( *((Real*)inPtrToValueData1) == *((Real*)inPtrToValueData2))
					return CR_EQUAL;
				else if( *((Real*)inPtrToValueData1) > *((Real*)inPtrToValueData2))
					return CR_BIGGER;
				else
					return CR_SMALLER;
			}
			else
			{
				if ( fabs ( *((Real*)inPtrToValueData1) - *((Real*)inPtrToValueData2) ) < kDefaultEpsilon )
					return CR_EQUAL;
				else if (*((Real*)inPtrToValueData1)> *((Real*)inPtrToValueData2) )
					return CR_BIGGER;
				else
					return CR_SMALLER;
			}
		};
*/


class VEmpty;
class XTOOLBOX_API VEmpty_info : public VValueInfo
{
public:
	VEmpty_info():VValueInfo( VK_EMPTY)								{;}
	virtual	VValue*		Generate() const;
	virtual	VValue*		Generate(VBlob* inBlob) const;
	virtual	VValue*		LoadFromPtr( const void *ioBackStore, bool /*inRefOnly*/) const;
	virtual void*		ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const;
};


class XTOOLBOX_API VEmpty : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VEmpty_info InfoType;
	static	const InfoType	sInfo;

			VEmpty();
	virtual	~VEmpty();

	// Inherited from VValue
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean		CanBeEvaluated() const;
	virtual VEmpty*		Clone() const;

	// Inherited from VValueSingle
	virtual CompareResult	CompareTo( const VValueSingle& /*inValue*/, Boolean inDiacritic = false) const;
};


class XTOOLBOX_API VByte : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VValueInfo_scalar<VByte,VK_BYTE,sBYTE>	InfoType;
	static	const InfoType	sInfo;

								VByte():fValue( 0)									{;}
								VByte( const VByte& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VByte( sBYTE inValue):fValue( inValue)				{;}
	explicit					VByte( sBYTE* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const { return true; }
	virtual void				Clear();

					operator sBYTE() const									{ return fValue;}
			VByte&	operator =( sBYTE inValue)								{ FromByte(inValue); return* this; }

			sBYTE				Get() const									{ return fValue; }

	virtual Boolean				GetBoolean() const							{ return (fValue != 0); }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return (sWORD) fValue; }
	virtual sLONG				GetLong() const								{ return (sLONG) fValue; }
	virtual sLONG8				GetLong8() const							{ return (sLONG8) fValue; }
	virtual Real				GetReal() const								{ return (Real) fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);

	virtual void				FromValue( const VValueSingle& inValue);
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual void*				LoadFromPtr( const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VByte*				Clone() const								{ return new VByte(*this); }

private:
			sBYTE				fValue;
};


class XTOOLBOX_API VWord : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VValueInfo_scalar<VWord,VK_WORD,sWORD>	InfoType;
	static	const InfoType	sInfo;

								VWord():fValue( 0)							{;}
								VWord( const VWord& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VWord( sWORD inValue):fValue( inValue)		{;}
	explicit					VWord( sWORD* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const { return true; }
	virtual void				Clear();

					operator sWORD() const									{ return fValue;}
			VWord&	operator =( sWORD inValue)								{ FromWord( inValue); return* this; }

			sWORD				Get() const									{ return fValue; }

	virtual Boolean				GetBoolean() const							{ return (fValue != 0); }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return fValue; }
	virtual sLONG				GetLong() const								{ return (sLONG) fValue; }
	virtual sLONG8				GetLong8() const							{ return (sLONG8) fValue; }
	virtual Real				GetReal() const								{ return (Real) fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);
	virtual void				FromValue( const VValueSingle& inValue);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual void*				LoadFromPtr( const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VWord*				Clone() const								{ return new VWord(*this); }

private:
			sWORD				fValue;
};


class XTOOLBOX_API VLong : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VValueInfo_scalar<VLong,VK_LONG,sLONG>	InfoType;
	static	const InfoType	sInfo;

								VLong():fValue( 0)							{;}
								VLong( const VLong& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VLong( sLONG inValue):fValue( inValue)		{;}
	explicit					VLong( sLONG* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const						{ return true; }
	virtual void				Clear();

	// no virtual intentionnaly
					operator sLONG() const									{ return fValue; }
			VLong&	operator =( sLONG inValue)								{ FromLong( inValue); return* this; }

			sLONG				Get() const									{ return fValue; }

	virtual Boolean				GetBoolean() const							{ return (fValue != 0); }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return (sWORD) fValue; }
	virtual sLONG				GetLong() const								{ return fValue; }
	virtual sLONG8				GetLong8() const							{ return (sLONG8) fValue; }
	virtual Real				GetReal() const								{ return (Real) fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);
	virtual void				FromValue( const VValueSingle& inValue);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual void*				LoadFromPtr( const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VLong*				Clone() const								{ return new VLong(*this); }

private:
			sLONG				fValue;
};


class XTOOLBOX_API VLong8 : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VValueInfo_scalar<VLong8,VK_LONG8,sLONG8>	InfoType;
	static	const InfoType	sInfo;

								VLong8():fValue( 0)							{;}
								VLong8( const VLong8& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VLong8( sLONG8 inValue):fValue( inValue)	{;}
	explicit					VLong8( sLONG8* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const { return true; }
	virtual void				Clear();

					operator sLONG8() const									{ return fValue;}
			VLong8&	operator =( sLONG8 inValue)								{ FromLong8(inValue); return* this; }

			sLONG8				Get() const									{ return fValue;}

	virtual Boolean				GetBoolean() const							{ return (fValue != 0); }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return (sWORD) fValue; }
	virtual sLONG				GetLong() const								{ return (sLONG) fValue; }
	virtual sLONG8				GetLong8() const							{ return fValue; }
	virtual Real				GetReal() const								{ return (Real) fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);
	virtual void				FromValue( const VValueSingle& inValue);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual void*				LoadFromPtr(  const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VLong8*				Clone() const								{ return new VLong8(*this); }

protected:
			sLONG8				fValue;
};


class XTOOLBOX_API VBoolean : public VValueSingle
{
typedef VValueSingle	inherited;
public:
	typedef VValueInfo_scalar<VBoolean,VK_BOOLEAN,Boolean>	InfoType;
	static	const InfoType	sInfo;

								VBoolean():fValue( false)					{;}
								VBoolean( const VBoolean& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VBoolean( Boolean inValue):fValue( inValue)	{;}
	explicit					VBoolean( uBYTE* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const						{ return true; }
	virtual void				Clear();

						operator Boolean() const							{ return fValue; }
						operator bool() const								{ return fValue != 0; }
			VBoolean&	operator =( Boolean inValue)						{ FromBoolean( inValue); return *this; }

			Boolean				Get() const									{ return fValue; }

	virtual Boolean				GetBoolean() const							{ return fValue; }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return (sWORD) fValue; }
	virtual sLONG				GetLong() const								{ return (sLONG) fValue; }
	virtual sLONG8				GetLong8() const							{ return (sLONG8) fValue; }
	virtual Real				GetReal() const								{ return (Real) fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);
	virtual void				FromValue( const VValueSingle& inValue);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual	void				GetXMLString( VString& outString, XMLStringOptions inOptions) const;
	virtual	bool				FromXMLString( const VString& inString);

	virtual void*				LoadFromPtr( const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	// Returns the number of bytes written by WriteToStream()
	virtual VSize				GetStreamSpace() const{return sizeof(sWORD);}

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual VError					FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError					GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VBoolean*			Clone() const								{ return new VBoolean(*this); }

private:
			Boolean				fValue;
};


class XTOOLBOX_API VReal : public VValueSingle
{
typedef VValueSingle	inherited;
public:

	class XTOOLBOX_API VValueInfo_real : public VValueInfo_scalar<VReal,VK_REAL,Real>
	{
	public:
		VValueInfo_real(){}
		virtual CompareResult CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const;

	};

	typedef VValueInfo_real	InfoType;
	static	const InfoType		sInfo;

								VReal():fValue( 0.0)						{;}
								VReal( const VReal& inOther):inherited( inOther),fValue( inOther.fValue)	{;}
	explicit					VReal( Real inValue):fValue( inValue)		{;}
	explicit					VReal( Real* inDataPtr, Boolean inInit);

	// Inherited from VValueSingle
	virtual const VValueInfo*	GetValueInfo() const;
	virtual Boolean				CanBeEvaluated() const						{ return true; }
	virtual void				Clear();

					operator Real() const									{ return fValue; }
			VReal&	operator =( Real inValue)								{ FromReal( inValue); return* this; }

			Real				Get() const									{ return fValue; }

	virtual Boolean				GetBoolean() const							{ return (fValue != 0.0); }
	virtual sBYTE				GetByte() const								{ return (sBYTE) fValue; }
	virtual sWORD				GetWord() const								{ return (sWORD) fValue; }
	virtual sLONG				GetLong() const								{ return (sLONG) fValue; }
	virtual sLONG8				GetLong8() const							{ return (sLONG8) fValue; }
	virtual	Real				GetReal() const								{ return fValue; }
	virtual void				GetFloat( VFloat& outValue) const;
	virtual void				GetDuration( VDuration& outValue) const;
	virtual void				GetString( VString& outValue) const;
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				FromBoolean( Boolean inValue);
	virtual void				FromByte( sBYTE inValue);
	virtual void				FromWord( sWORD inValue);
	virtual void				FromLong( sLONG inValue);
	virtual void				FromLong8( sLONG8 inValue);
	virtual void				FromReal( Real inValue);
	virtual void				FromFloat( const VFloat& inValue);
	virtual void				FromDuration( const VDuration& inValue);
	virtual void				FromString( const VString& inValue);
	virtual void				FromValue( const VValueSingle& inValue);

	virtual	void				GetXMLString( VString& outString, XMLStringOptions inOptions) const;
	virtual	bool				FromXMLString( const VString& inString);

	virtual	VError				FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError				GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	static	void				RealToXMLString( Real inValue, VString& outString);

	virtual void*				LoadFromPtr( const void* inDataPtr, Boolean inRefOnly = false);
	virtual void*				WriteToPtr( void* inDataPtr, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize				GetSpace(VSize /*inMax*/ = 0) const							{ return sizeof(fValue); }

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual CompareResult		CompareToSameKind( const VValue* inValue, Boolean inDiacritical =false) const;
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual Boolean				FromValueSameKind( const VValue* inValue);

	virtual CompareResult		CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean				EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual CompareResult		CompareTo( const VValueSingle& inValue, Boolean inDiacritic = false) const;

	virtual VError				FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError				GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual VReal*				Clone() const								{ return new VReal(*this); }

private:
			Real			fValue;
};



class XTOOLBOX_API VectorOfVValue : public std::vector<const VValueSingle*>
{
	public:
		VectorOfVValue(bool AutoDisposeValues = false)
		{
			fAutoDisposeValues = AutoDisposeValues;
		}

		virtual ~VectorOfVValue()
		{
			if (fAutoDisposeValues)
			{
				for (std::vector<const VValueSingle*>::iterator cur = begin(), last = end(); cur != last; cur++)
				{
					delete (VValueSingle*)*cur;
				}
			}
		}

		void SetAutoDispose(bool x)
		{
			fAutoDisposeValues = x;
		}

	protected:
		bool fAutoDisposeValues;
};



END_TOOLBOX_NAMESPACE

#endif


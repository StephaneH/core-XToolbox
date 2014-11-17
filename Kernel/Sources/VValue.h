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
#ifndef __VValue__
#define __VValue__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IStreamable.h"
#include <map>


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VValue;
class VValueSingle;
class VValueBag;
class VArrayValue;
class VBlob;
class VIntlMgr;
class VJSONValue;

const sLONG	kMAX_PREDEFINED_VALUE_TYPES		= 100;
const Real kDefaultEpsilon = (1.0e-06);

// Class declaration
typedef enum ValueFlag {
#if BIGENDIAN
	Value_flag4		= 1,
	Value_flag3		= 2,
	Value_flag2		= 4,
	Value_flag1		= 8,
	Value_UseAlternateMemory	= 32,
	Value_IsDirty	= 64,
	Value_IsNull	= 128
#else
	Value_IsNull	= 1,
	Value_IsDirty	= 2,
	Value_UseAlternateMemory	= 4,
	Value_flag1		= 16,
	Value_flag2		= 32,
	Value_flag3		= 64,
	Value_flag4		= 128
#endif
} ValueFlag;


enum
{
/*
	because VValueKind can be saved in external documents,
	do not insert any value.

	values are unsigned.
*/
	VK_UNDEFINED = 0xffffu,
	VK_EMPTY = 0,
	VK_BOOLEAN,
	VK_BYTE,
	VK_WORD,
	VK_LONG,
	VK_LONG8,
	VK_REAL,
	VK_FLOAT,
	VK_TIME,
	VK_DURATION,
	VK_STRING,
	VK_BLOB,
	VK_IMAGE,
	VK_UUID,
	VK_TEXT,	// Private for DB4D use
	VK_SUBTABLE, // Private for DB4D use
	VK_SUBTABLE_KEY, // Private for DB4D use
	VK_OBSOLETE_STRING_DB4D, // obsolete
	VK_BLOB_DB4D, // Private for DB4D use
	VK_STRING_UTF8, // Private for DB4D use
	VK_TEXT_UTF8, // Private for DB4D use
	VK_BLOB_OBJ, // Private for DB4D use
	VK_FIRST_SINGLE	= VK_EMPTY,
	VK_LAST_SINGLE	= VK_BLOB_OBJ,		// to update each time you add a new kind

	VK_ARRAY_BOOLEAN = 100,
	VK_ARRAY_BYTE,
	VK_ARRAY_WORD,
	VK_ARRAY_LONG,
	VK_ARRAY_LONG8,
	VK_ARRAY_REAL,
	VK_ARRAY_FLOAT,
	VK_ARRAY_TIME,
	VK_ARRAY_DURATION,
	VK_ARRAY_STRING,
	VK_ARRAY_BLOB,
	VK_ARRAY_IMAGE,
	VK_ARRAY_UUID,

	VK_FIRST_ARRAY 	= VK_ARRAY_BOOLEAN,
	VK_LAST_ARRAY	= VK_ARRAY_UUID,

	VK_BAG = 200
};
typedef uWORD ValueKind;

enum
{
	JSON_Default = 0,
	JSON_WithQuotesIfNecessary = 1,
	JSON_PrettyFormatting = 8,
	JSON_AlreadyEscapedChars = 16,
	JSON_UniqueSubElementsAreNotArrays = 32,
	JSON_AllowDates = 64

};
typedef uLONG JSONOption;

/**

	@brief VCompareOptions encapsulates various compare options

**/

class VJSONObject;

class XTOOLBOX_API VCompareOptions : public VObject
{
public:
	VCompareOptions()
		: fIntlManager( NULL), fLike( false), fDiacritical( false), fBeginsWith( false), fEpsilon( kDefaultEpsilon)
		{}

	VCompareOptions( VIntlMgr *inIntlManager, bool inLike, bool inBeginsWith, bool inDiacritical, Real inEpsilon)
		: fIntlManager( inIntlManager), fLike( inLike), fDiacritical(inDiacritical), fBeginsWith( inBeginsWith), fEpsilon( inEpsilon)
		{}

	VCompareOptions( const VCompareOptions& inOther)
		: fIntlManager( inOther.fIntlManager), fLike( inOther.fLike), fDiacritical(inOther.fDiacritical), fBeginsWith( inOther.fBeginsWith), fEpsilon( inOther.fEpsilon)
		{}

	VIntlMgr*	GetIntlManager() const							{ return fIntlManager;}
	void		SetIntlManager( VIntlMgr *inIntlManager)		{ fIntlManager = inIntlManager;}
	void		SetIntlManagerIfEmpty( VIntlMgr *inIntlManager)
	{
		if (fIntlManager == NULL)
			fIntlManager = inIntlManager;
	}

	Real		GetEpsilon() const								{ return fEpsilon;}
	void		SetEpsilon( Real inEpsilon)						{ fEpsilon = inEpsilon;}

	bool		IsLike() const									{ return fLike;}
	void		SetLike( bool inLike)							{ fLike = inLike;}

	bool		IsBeginsWith() const							{ return fBeginsWith;}
	void		SetBeginsWith( bool inBeginsWith)				{ fBeginsWith = inBeginsWith;}

	bool		IsDiacritical() const							{ return fDiacritical;}
	void		SetDiacritical( bool inDiacritical)				{ fDiacritical = inDiacritical;}

	VCompareOptions& operator=( const VCompareOptions& inOther)
		{
			fIntlManager = inOther.fIntlManager;
			fLike = inOther.fLike;
			fBeginsWith = inOther.fBeginsWith;
			fDiacritical = inOther.fDiacritical;
			fEpsilon = inOther.fEpsilon;
			return *this;
		}

	VError ToJSON(VJSONObject* outObj);

	VError FromJSON(VJSONObject* inObj);

private:
	VIntlMgr*	fIntlManager;
	bool		fLike;
	bool		fDiacritical;
	bool		fBeginsWith;
	Real		fEpsilon;
};


/*
	VValueInfo

	Describes a VValue. Allows Creation, loading and byteswap of vvalues.
	Each VValue class must register itself with VValue::RegisterValueInfo()

*/

class XTOOLBOX_API VValueInfo : public VObject
{
public:
	VValueInfo( ValueKind inKind);
	VValueInfo( ValueKind inKind, ValueKind inTrueKind);
	virtual							~VValueInfo();

			ValueKind				GetKind() const																				{ return fKind;}
			ValueKind				GetTrueKind() const																			{ return fTrueKind;}

	virtual	VValue*					Generate() const = 0;
	virtual	VValue*					Generate(VBlob* inBlob) const = 0;
	virtual	VValue*					LoadFromPtr( const void * /*inBackStore*/, bool /*inRefOnly*/) const = 0;
	virtual	void*					ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const = 0;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;

	virtual CompareResult			CompareTwoPtrToData_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining_Like( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;

	virtual CompareResult			CompareTwoPtrToDataWithOptions( const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const;

	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;

	// Max bytes to reserve when using LoadFromPtr/WriteToPtr (must be generic for all objects of this kind)
	virtual	VSize					GetAvgSpace() const;

	void _SetKind(ValueKind newKind) { fKind = newKind; }

private:
			ValueKind				fKind;
			ValueKind				fTrueKind;
};


/*
	VValueInfo_default

	Default template for very simple VValues (no byteswap)
*/
template<class T, ValueKind K>
class VValueInfo_default : public VValueInfo
{
public:
	VValueInfo_default():VValueInfo( K)																{;}
	VValueInfo_default(ValueKind inTrueKind):VValueInfo( K, inTrueKind)								{;}
	virtual	VValue*					Generate() const												{ return new T;}
	virtual	VValue*					Generate(VBlob* /*inBlob*/) const								{ return new T;}
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const
		{
			T *t = new T;
			if (t)
				t->LoadFromPtr( inBackStore, inRefOnly);
			return t;
		}
	virtual void*					ByteSwapPtr( void *inBackStore, bool /*inFromNative*/) const		{ return inBackStore;}
};

typedef std::map<ValueKind, const VValueInfo*> VValueInfoIndex;


class XTOOLBOX_API VValue : public VObject
{
public:
									VValue():fFlagsByte( 0)											{ fFlags.fIsDirty = true;}
									VValue( bool inIsNull):fFlagsByte( 0)							{ fFlags.fIsNull = inIsNull;fFlags.fIsDirty = true;}
									VValue( const VValue& inOther):fFlagsByte( inOther.fFlagsByte)	{ fFlags.fIsDirty = true;}

	// Kind support
	virtual	const VValueInfo*		GetValueInfo() const;

	// value kind
			ValueKind				GetValueKind() const											{ return GetValueInfo()->GetKind();}

	// primary kind (more specific than GetValueKind, used mainly by DB4D)
	// NewValueFromValueKind expects a TrueValueKind
	// example: DB4DString is a VString. DB4DString::GetValueKind() returns VK_STRING and DB4DString::GetTrueValueKind() returns VK_TEXT
			ValueKind				GetTrueValueKind() const										{ return GetValueInfo()->GetTrueKind();}

	// Value support
	virtual	void					Clear();

	// Kind casting (returns NULL if cast fails)
	virtual const VValueSingle*		IsVValueSingle() const;
	virtual VValueSingle*			IsVValueSingle();

	// State accessors
			void					SetDirty( bool inSet = true);
			bool					IsDirty() const													{ return fFlags.fIsDirty != 0; }
			void					ClearDirty()													{ fFlags.fIsDirty = 0; };

			void					SetNull( bool inSet = true);
			bool					IsNull() const													{ return fFlags.fIsNull != 0; }

	// convert to and from json.
	// warning: doesn't handle null.
	virtual VError					FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError					GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual	VError					FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError					GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	// Comparison support (inValue MUST be the same kind as this)
	// CAUTION: checking is performed in debug mode only for performance reasons. So take care!

	virtual	CompareResult			CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool					EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult			CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool					EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual	CompareResult			Swap_CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual	CompareResult			CompareToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareBeginingToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	Boolean					EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	Boolean					FromValueSameKind( const VValue* inValue);

	virtual	CompareResult			CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult			Swap_CompareToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareBeginingToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			IsTheBeginingToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean					EqualToSameKindPtr( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult			CompareToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareBeginingToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;
	virtual	Boolean					EqualToSameKind_Like( const VValue* inValue, Boolean inDiacritical = false) const;

	virtual	CompareResult			CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual	CompareResult			Swap_CompareToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual CompareResult			IsTheBeginingToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean					EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean					Swap_EqualToSameKindPtr_Like( const void* inPtrToValueData, Boolean inDiacritical = false) const;

	// Storage support
	virtual void*					LoadFromPtr( const void* inData, Boolean inRefOnly = false);
	virtual void*					WriteToPtr( void* ioData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize					GetSpace(VSize inMax = 0) const;	// Actual number of bytes needed by WriteToPtr

	virtual VSize					GetFullSpaceOnDisk() const	// will differ for blobs and pictures
	{
		return GetSpace();
	}

	// Inherited from IStreamable
	virtual	VError					ReadFromStream( VStream* ioStream, sLONG inParam = 0);
	virtual VError					WriteToStream( VStream* ioStream, sLONG inParam = 0) const;

	// Returns the number of bytes written by WriteToStream()
	virtual VSize					GetStreamSpace() const	// will differ for VBoolean
	{
		return GetSpace();
	}

	// Called only if created with a dataPtr (see notes abouts constructors)
	virtual	VError					Flush( void* inDataPtr, void* inContext) const;
	virtual void					SetReservedBlobNumber(sLONG inBlobNum);

	// Utilities
	virtual VValue*					FullyClone(bool ForAPush = false) const; // no intelligent cloning (for example cloning by refcounting for Blobs)
	virtual void					RestoreFromPop(void* context);
	virtual void					Detach(Boolean inMayEmpty = false); // used by blobs in DB4D

	virtual void					SetOutsidePath(const VString& /*inPosixPath*/, bool /*inIsRelative*/ = false) // even if the path IS empty , will set the state of the VValue as outside ( path will then be autogenerated by DB4D if empty)
	{
	}

	virtual void					GetOutsidePath(VString& /*outPosixPath*/, bool* /*outIsRelative*/ = NULL)
	{
	}

	virtual void					SetOutsideSuffixe(const VString& /*inExtention*/)
	{
	}

	virtual bool					IsOutsidePath()
	{
		return false;
	}

	virtual VError					ReloadFromOutsidePath()
	{
		return VE_OK;
	}

	virtual bool					ForcePathIfEmpty() const
	{
		return false;
	}

	virtual void					SetForcePathIfEmpty(bool /*x*/)
	{
	}


	virtual void					SetHintRecNum(sLONG /*TableNum*/, sLONG /*FieldNum*/, sLONG /*inHint*/)
	{
		// nothing to do here, needs to be done for blobs in DB4D
	}

	virtual void					AssociateRecord(void* /*primkey*/, sLONG /*FieldNum*/)
	{
		// nothing to do here, needs to be done for blobs in DB4D
	}


	virtual	VError					ReadRawFromStream( VStream* ioStream, sLONG inParam = 0); //used by db4d server to read data without interpretation (for example : pictures)

	virtual VValueSingle* GetDataBlob() const
	{
		return NULL;
	}

	virtual VValue*					Clone() const = 0;

	static	VValue*					NewValueFromValueKind( ValueKind inTrueKind);
	static	const VValueInfo*		ValueInfoFromValueKind( ValueKind inTrueKind);
	static	void					RegisterValueInfo( const VValueInfo *inValueGenerator);
	static	void					UnregisterValueInfo( const VValueInfo *inValueGenerator);
	static	CompareResult			InvertCompResult(CompareResult comp);


	// from OLDBOX, don't know exactly what that means
	virtual	Boolean					CanBeEvaluated() const;

protected:
			bool					GetFlag( ValueFlag inIndex) const								{ return ((fFlagsByte & (uBYTE) inIndex) != 0); }
			void					SetFlag( ValueFlag inIndex)										{ fFlagsByte |= (uBYTE) inIndex; }
			void					ClearFlag( ValueFlag inIndex)									{ fFlagsByte &= ~ (uBYTE) inIndex; }

			void					SetFlag1( bool inValue)											{ fFlags.fCustom1 = inValue ? 1 : 0; }
			void					SetFlag2( bool inValue)											{ fFlags.fCustom2 = inValue ? 1 : 0; }
			void					SetFlag3( bool inValue)											{ fFlags.fCustom3 = inValue ? 1 : 0; }
			void					SetFlag4( bool inValue)											{ fFlags.fCustom4 = inValue ? 1 : 0; }

	// Override to be notified when state change
	virtual void					DoNullChanged();
	virtual void					DoDirtyChanged();

	// Call to notifiy when value changed
			void					GotValue( bool inNull = false)									{ SetNull(inNull); fFlags.fIsDirty = true;}

private:
	union {
		uBYTE	fFlagsByte;
		struct {
			uBYTE	fIsNull : 1;
			uBYTE	fIsDirty : 1;
			uBYTE	fBit3 : 1;
			uBYTE	fBit4 : 1;
			uBYTE	fCustom1 : 1;	// reserved for subclasses
			uBYTE	fCustom2 : 1;	// reserved for subclasses
			uBYTE	fCustom3 : 1;	// reserved for subclasses
			uBYTE	fCustom4 : 1;	// reserved for subclasses
		} fFlags;
	};

	static	VValueInfoIndex*		sInfos;
};


END_TOOLBOX_NAMESPACE

#endif

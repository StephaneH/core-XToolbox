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
#ifndef __VArrayValue__
#define __VArrayValue__

#include "Kernel/Sources/VValueMultiple.h"
#include "Kernel/Sources/ISortable.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VArrayValue : public VValueMultiple, public ISortable
{
public:
			VArrayValue ();
	virtual ~VArrayValue ();
	
	virtual ValueKind	GetElemValueKind () const = 0;
	
	// Items count handling
	virtual Boolean	SetCount (sLONG inNb);	// Decreasing count will drop latest values
	sLONG	GetCount () const;

	// inherited from VValue
	virtual	void	Clear();
	
	virtual Boolean	Insert (sLONG inAt, sLONG inNb);
	virtual void	Delete (sLONG inAt, sLONG inNb = 1);

	// Insertion macros
	sLONG	AppendBoolean (Boolean inValue);
	sLONG	AppendByte (sBYTE inValue); 
	sLONG	AppendWord (sWORD inValue);
	sLONG	AppendLong (sLONG inValue);
	sLONG	AppendLong8 (sLONG8 inValue);
	sLONG	AppendReal (Real inValue);
	sLONG	AppendFloat (const VFloat& inValue);
	sLONG	AppendDuration (const VDuration& inValue);
	sLONG	AppendTime (const VTime& inValue);
	sLONG	AppendString (const VString& inValue);
	
	// Search & sort support
	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const = 0;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	virtual void	Unsort ();
	
	static void	SynchronizedSort (Boolean inDescending, VArrayValue* inArray, ...);

	sLONG	GetDataSize () const;
	Boolean	SetDataSize (sLONG inNewSize);
	
	virtual void*	LoadFromPtr (const void* inData, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void* inData, Boolean inRefOnly = false, VSize inMax = 0) const;
	
	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement) = 0;
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const = 0;
	virtual VValueSingle*	CloneValue (sLONG inElement) const = 0;
	
	// Utilities
	virtual Boolean Copy (const VArrayValue& inOriginal);
	void	Destroy ();
	
protected:
	sLONG	fElemSize;	// Size of an element in the array
	sLONG	fCount;		// Number of logical elements
	VPtr	fDataPtr;	// Items storage
	sLONG	fDataSize;
	
	// Data accessing support
	uBYTE*	LockAndGetData () const;
	void	UnlockData () const;
	
	// Items management support
	virtual void	MoveElements (sLONG inFrom, sLONG inTo, sLONG inCount);
	virtual void	DeleteElements (sLONG inAt, sLONG inNb);
	virtual void	InitElements (sLONG inAt, sLONG inNb);
	Boolean	ValidIndex (sLONG inIndex) const;
};



template<class VVALUE_CLASS, ValueKind VALUE_KIND, class BACKSTORE_TYPE>
class XTOOLBOX_API VValueInfo_array : public VValueInfo_default< VVALUE_CLASS, VALUE_KIND>
{
public:
	VValueInfo_array() {};
// load & xrite & byteswap: tbd
};


class XTOOLBOX_API VArrayBoolean : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayBoolean,VK_ARRAY_BOOLEAN,Boolean>	InfoType;
	static	const InfoType	sInfo;

	VArrayBoolean (sLONG inNbElems = 0);
	VArrayBoolean (const VArrayBoolean& inOriginal);

	// Inherited from VArrayValue	
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_BOOLEAN; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayBoolean(*this); };

	virtual Boolean SetCount (sLONG inNb);
	virtual void	Delete (sLONG inAt, sLONG inNb = 1);

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (Boolean inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;
	
	virtual void	 FromBoolean (Boolean inValue, sLONG inElement);
	virtual Boolean GetBoolean (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	virtual void	Unsort ();

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
protected:
	virtual void	MoveElements (sLONG inFrom, sLONG inTo, sLONG inCount);
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual void	InitElements (sLONG inFrom, sLONG inNb);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayByte : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayByte,VK_ARRAY_BYTE,sBYTE>	InfoType;
	static	const InfoType	sInfo;

	VArrayByte (sLONG inNbElems = 0);
	VArrayByte (const VArrayByte& inOriginal);

	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (sBYTE inValue) const;	// Use only if array is sorted

	// Utilities
	sBYTE	GetMin () const;
	sBYTE	GetMax () const;

	// Inherited from VArrayValue	
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_BYTE; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayByte(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (sBYTE inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromByte (sBYTE inValue, sLONG inElement);
	virtual sBYTE	GetByte (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayWord : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayWord,VK_ARRAY_WORD,sWORD>	InfoType;
	static	const InfoType	sInfo;

	VArrayWord (sLONG inNbElems = 0);
	VArrayWord (const VArrayWord& inOriginal);
	
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (sWORD inValue) const;	// Use only if array is sorted

	// Utilities
	sWORD	GetMin () const;
	sWORD	GetMax () const;
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_WORD; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayWord(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (sWORD inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	 FromWord (sWORD inValue, sLONG inElement);
	virtual sWORD GetWord (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayLong : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayLong,VK_ARRAY_LONG,sLONG>	InfoType;
	static	const InfoType	sInfo;

	VArrayLong (sLONG inNbElems = 0);
	VArrayLong (const VArrayLong& inOriginal);
	VArrayLong& operator=(const VArrayLong& pFrom);
	
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (sLONG inValue) const;	// Use only if array is sorted

	// Utilities
	sLONG	GetMin () const;
	sLONG	GetMax () const;
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_LONG; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayLong(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (sLONG inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	 FromLong (sLONG inValue, sLONG inElement);
	virtual sLONG GetLong (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;
	
	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayLong8 : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayLong8,VK_ARRAY_LONG8,sLONG8>	InfoType;
	static	const InfoType	sInfo;

	VArrayLong8 (sLONG inNbElems = 0);
	VArrayLong8 (const VArrayLong8& inOriginal);

	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (sLONG8 inValue) const;	// Use only if array is sorted
	
	// Inherited from VArrayValue	
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_LONG8; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayLong8(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (sLONG8 inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromLong8 (sLONG8 inValue, sLONG inElement);
	virtual sLONG8	GetLong8 (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayReal : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayReal,VK_ARRAY_REAL,Real>	InfoType;
	static	const InfoType	sInfo;

	VArrayReal (sLONG inNbElems = 0);
	VArrayReal (const VArrayReal& inOriginal);

	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (Real inValue) const;	// Use only if array is sorted

	// Utilities
	Real	GetAverage () const;
	Real	GetSquareSum () const;
	Real	GetStdDev () const;
	Real	GetSum () const;

	// Inherited from VArrayValue	
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_REAL; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayReal(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (Real inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 
	
	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromReal (Real inValue, sLONG inElement);
	virtual Real GetReal (sLONG inElement) const;

	virtual void	Sort (sLONG inFrom, sLONG inToBoolean, Boolean inDescending = false);
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayFloat : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayFloat,VK_ARRAY_FLOAT,char>	InfoType;
	static	const InfoType	sInfo;

			VArrayFloat (sLONG inNbElems = 0);
			VArrayFloat (const VArrayFloat& inOriginal);
	virtual ~VArrayFloat ();
	
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (const VFloat& inValue) const;	// Use only if array is sorted
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_FLOAT; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayFloat(*this); };
	
	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (const VFloat& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 
	
	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromFloat (const VFloat& inFloat, sLONG inElement);
	virtual void	GetFloat (VFloat& outFloat, sLONG inElement) const;

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	DeleteElements (sLONG inAt, sLONG inNb);
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayTime : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayTime,VK_ARRAY_TIME,uLONG8>	InfoType;
	static	const InfoType	sInfo;

	VArrayTime (sLONG inNbElems = 0);
	VArrayTime (const VArrayTime& inOriginal);
		
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (const VTime& inValue) const;	// Use only if array is sorted

	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_TIME; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayTime(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (const VTime& inTime, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const; 

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromTime (const VTime& inTime, sLONG inElement);
	virtual void	GetTime (VTime& outTime, sLONG inElement) const;

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayDuration : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayDuration,VK_ARRAY_DURATION,sLONG8>	InfoType;
	static	const InfoType	sInfo;

	VArrayDuration (sLONG inNbElems = 0);
	VArrayDuration (const VArrayDuration& inOriginal);
	
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (const VDuration& inDuration) const;	// Use only if array is sorted
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_DURATION; };
	virtual Boolean CanBeEvaluated () const { return true; };
	virtual VValue* Clone () const { return new VArrayDuration(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (const VDuration& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromDuration (const VDuration& inDuration, sLONG inElement);
	virtual void	GetDuration (VDuration& outDuration, sLONG inElement) const;

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};


class XTOOLBOX_API VArrayString : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayString,VK_ARRAY_STRING,char>	InfoType;
	static	const InfoType	sInfo;

			VArrayString (sLONG inNbElems = 0);
			VArrayString (const VArrayString& inOriginal);
	virtual ~VArrayString ();
	
	// Fast search support (use only if array is sorted)
	sLONG	QuickFind (const VString& inValue) const;
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	virtual ValueKind	GetElemValueKind () const { return VK_STRING; };
	virtual Boolean	CanBeEvaluated () const { return true; };
	virtual VValue*	Clone () const { return new VArrayString(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	virtual sLONG	Find (const VString& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;

	virtual Boolean	Copy (const VArrayValue& inOriginal);

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;

	virtual void	FromString (const VString& inString, sLONG inElement);
	virtual void	GetString (VString& outString, sLONG inElement) const;

	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;
	
protected:
	virtual void	DeleteElements (sLONG inAt, sLONG inNb);
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
};



class XTOOLBOX_API VArrayImage : public VArrayValue
{
public:
	typedef VValueInfo_array<VArrayImage,VK_ARRAY_IMAGE,char>	InfoType;
	static	const InfoType	sInfo;

	VArrayImage (sLONG inNbElems = 0);
	//VArrayImage (const VArrayImage& inOriginal);
	virtual ~VArrayImage ();
	

	virtual Boolean	SetCount (sLONG inNb);	// Decreasing count will drop latest values

	// Fast search support (use only if array is sorted)
	//sLONG	QuickFind (const VString& inValue) const;
	
	// Inherited from VArrayValue
	virtual const VValueInfo*	GetValueInfo () const;
	
	virtual ValueKind	GetElemValueKind () const { return VK_IMAGE; };
	
	//virtual Boolean	CanBeEvaluated () const { return true; };
	//virtual VValue*	Clone () const { return new VArrayString(*this); };

	virtual sLONG	Find (const VValueSingle& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;
	
	//virtual sLONG	Find (const VString& inValue, sLONG inFrom = 1, Boolean inUp = true, CompareMethod inCm = CM_EQUAL) const;

	//virtual Boolean	Copy (const VArrayValue& inOriginal);

	virtual void			FromValue (const VValueSingle& inValue, sLONG inElement);
	virtual void			GetValue (VValueSingle& outValue, sLONG inElement) const;
	virtual VValueSingle*	CloneValue (sLONG inElement) const;
	

	//virtual void	FromString (const VString& inString, sLONG inElement);
	//virtual void	GetString (VString& outString, sLONG inElement) const;

	//virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const;

	virtual VError	ReadFromStream (VStream* inStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* inStream, sLONG inParam = 0) const;

protected:
	virtual void	DeleteElements (sLONG inAt, sLONG inNb);
	void DeleteAllElements ();
	virtual void	SwapElements (uBYTE* data, sLONG inA, sLONG inB);
	virtual CompareResult	CompareElements (uBYTE* data, sLONG inA, sLONG inB);
	

private:
	std::vector<VValueSingle *> fValues;
	VArrayImage (const VArrayImage& inOriginal);
};

END_TOOLBOX_NAMESPACE


#endif

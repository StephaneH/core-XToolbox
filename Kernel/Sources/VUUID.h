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
#ifndef __VUUID__
#define __VUUID__

#include "Kernel/Sources/VValueSingle.h"


#if VERSIONMAC
#include "Kernel/Sources/XMacUUID.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxUUID.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinUUID.h"
#endif


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VString;

// Defined bellow
class VUUID;


class XTOOLBOX_API VUUID_info : public VValueInfo_default<VUUID, VK_UUID>
{
public:
	VUUID_info() {}
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual	CompareResult			CompareTwoPtrToDataWithOptions(const void* inPtrToValueData1, const void* inPtrToValueData2, const VCompareOptions& inOptions) const;
	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
	virtual	VSize					GetAvgSpace() const;
};

// Class definitions
struct VUUIDBuffer
{
	inline void FromLong( sLONG inValue )
	{
		fBytes[0] = (uBYTE) (inValue & 0x000000FF);
		fBytes[1] = (uBYTE) ((inValue & 0x0000FF00) >> 8);
		fBytes[2] = (uBYTE) ((inValue & 0x00FF0000) >> 16);
		fBytes[3] = (uBYTE) ((inValue & 0xFF000000) >> 24);
		for ( int i = 4; i < 16; i++ )
			fBytes[i] = 0;
	};	

	inline bool operator < (const VUUIDBuffer& other) const
	{
		return (memcmp(&fBytes[0], &other.fBytes[0], 16) < 0);
		/*
		if (*((uLONG8*) &fBytes[0]) == *((uLONG8*) &other.fBytes[0]))
			return *((uLONG8*) &fBytes[8]) < *((uLONG8*) &other.fBytes[8]);
		else
			return *((uLONG8*) &fBytes[0]) < *((uLONG8*) &other.fBytes[0]);
		 */
	};

	inline bool operator == (const VUUIDBuffer& other) const
	{
		return  *((uLONG8*) &fBytes[8]) == *((uLONG8*) &other.fBytes[8]) && *((uLONG8*) &fBytes[0]) == *((uLONG8*) &other.fBytes[0]);
	};

	inline bool operator != (const VUUIDBuffer& other) const
	{
		return  *((uLONG8*) &fBytes[8]) != *((uLONG8*) &other.fBytes[8]) || *((uLONG8*) &fBytes[0]) != *((uLONG8*) &other.fBytes[0]);
	};

#if BIGENDIAN
	inline void IncFirst8() { ByteSwap((uLONG8*)&fBytes[0]); *((uLONG8*)(&fBytes[0])) += 1; ByteSwap((uLONG8*)&fBytes[0]);}
	inline void DecFirst8() { ByteSwap((uLONG8*)&fBytes[0]); *((uLONG8*)(&fBytes[0])) -= 1; ByteSwap((uLONG8*)&fBytes[0]);}
#else
	inline void IncFirst8() { *((uLONG8*)(&fBytes[0])) += 1; }
	inline void DecFirst8() { *((uLONG8*)(&fBytes[0])) -= 1; }
#endif
	uBYTE fBytes[16];

	inline void SwapBytes() { ; /* does nothing and may stay like this */ };
};


template<> inline VUUIDBuffer ByteSwapValue<VUUIDBuffer>( const VUUIDBuffer& inValue)	{return inValue;}


template<>
inline void ByteSwap( VUUIDBuffer* /*inBegin*/, VUUIDBuffer* /*inEnd*/)
{
	;
	// les UUID ne sont pas swappe
	/*
	for( ; inBegin != inEnd ; ++inBegin)
		ByteSwap( &*inBegin);
	 */
}


template<>
inline void ByteSwap( VUUIDBuffer * /*inFirstValue*/, VIndex /*inNbElems*/)
{
	;
	//ByteSwap( inFirstValue, inFirstValue + inNbElems);
}



class XTOOLBOX_API VUUID : public VValueSingle
{
public:
	typedef VUUID_info	TypeInfo;
	static const TypeInfo	sInfo;

				VUUID ();
	explicit	VUUID (Boolean inGenerate);
				VUUID (const VUUID& inOriginal);
				VUUID (const VUUIDBuffer& inUUID);
	explicit	VUUID ( const char *inUUIDString);	// inUUIDString MUST be a 16-chars length c-string
	virtual		~VUUID ();

#if VERSIONWIN
	explicit	VUUID (const GUID& inGUID);
#endif

	// UUID generation
	void	FromBuffer (const VUUIDBuffer& inUUID);
	void	FromBuffer (const char* inUUIDString);
	void	ToBuffer (VUUIDBuffer& outUUID) const;
	void	BuildFromAnsiString (const VString& inString);	// Use string's first 16 butes as is (see also FromString)
	inline VSize GetSizeInStream() const { return 16; };
	inline const VUUIDBuffer& GetBuffer() const { return fData; };
	inline const VUUIDBuffer& GetBuffer() { return fData; };
	
	void	Regenerate ();
	
	// Operators
	bool	operator == (const VUUID& inUUID) const;
	bool	operator != (const VUUID& inUUID) const;
	VUUID&	operator = (const VUUID& inUUID) { FromVUUID(inUUID); return *this; };
	bool	operator < (const VUUID& inUUID) const;
	
	// Inherited from VValue
	virtual	const VValueInfo*	GetValueInfo() const;
	virtual void				GetValue( VValueSingle& outValue) const;
	
	virtual void	GetVUUID (VUUID& outValue) const;
	virtual void	FromVUUID (const VUUID& inValue);

	virtual void*	LoadFromPtr (const void *inData, Boolean inRefOnly = false);
	virtual void*	WriteToPtr (void *inData, Boolean inRefOnly = false, VSize inMax = 0) const;
	virtual VSize	GetSpace (VSize inMax = 0) const;
	
	virtual void	FromString (const VString& inValue);	// String convertion (hexa notation)
	virtual void	GetString (VString& outValue) const;	// String convertion (hexa notation)

	virtual	void	FromValue( const VValueSingle& inValue);

	virtual VError	GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;

	virtual	VError	FromJSONValue( const VJSONValue& inJSONValue);
	virtual	VError	GetJSONValue(VJSONValue& outJSONValue, bool allowDates = false) const;

	virtual VError	ReadFromStream (VStream* ioStream, sLONG inParam = 0);
	virtual VError	WriteToStream (VStream* ioStream, sLONG inParam = 0) const;
	
	virtual CompareResult	CompareTo (const VValueSingle& inValue, Boolean inDiacritical = false) const;
	virtual CompareResult	CompareToSameKind (const VValue *inValue, Boolean inDiacritical =false) const;
	virtual Boolean	EqualToSameKind (const VValue *inValue, Boolean inDiacritical = false) const;
	virtual Boolean	FromValueSameKind (const VValue *inValue);

	virtual CompareResult	CompareToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;
	virtual Boolean	EqualToSameKindPtr (const void* inPtrToValueData, Boolean inDiacritical = false) const;

	virtual	CompareResult		CompareToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	bool				EqualToSameKindWithOptions( const VValue* inValue, const VCompareOptions& inOptions) const;
	virtual	CompareResult		CompareToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;
	virtual bool				EqualToSameKindPtrWithOptions( const void* inPtrToValueData, const VCompareOptions& inOptions) const;

	virtual VUUID*		Clone () const;
	virtual void		Clear	();

	VString*			GetCachedConvertedString(bool inCreateIfMissing = true);

	static	const VUUID		sNullUUID;
	
protected:
	VUUIDBuffer		fData;
	VString*		fCachedConvertedString;
	
	virtual void	DoNullChanged ();
	void			_GotValue(bool inNull = false);
};

typedef std::vector<XBOX::VUUID>					VectorOfVUUID;

END_TOOLBOX_NAMESPACE

#endif

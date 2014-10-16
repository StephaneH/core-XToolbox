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
#ifndef __VValueBag__
#define __VValueBag__

#include <map>

#include "Kernel/Sources/VValue.h"
#include "Kernel/Sources/VArrayValue.h"
#include "Kernel/Sources/VArray.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VUUID.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VPackedDictionary.h"


BEGIN_TOOLBOX_NAMESPACE


// Defined bellow
class VBagLoader;
class VBagArray;
class IBaggable;
class VValueBag;

// Needed declarations
class VString;
class VStream;
class VPackedValue_VBagArray;

typedef VPackedDictionary<VPackedValue_VBagArray>	VPackedVBagArrayDictionary;



/*!
	@class	VBagArray
	@abstract	Array of bag handling class
	@discussion	A list of VValueBag, used in list of elements
*/
class XTOOLBOX_API VBagArray : public VObject, public IRefCountable
{
public:
	typedef std::vector<VValueBag*>		bags_vector;
	
										VBagArray()									{;}
										VBagArray( const VBagArray& inOther);
	virtual								~VBagArray();

			VIndex						GetCount() const							{ return static_cast<VIndex>( fArray.size()); }
			bool						IsEmpty() const								{ return fArray.empty(); }
			bool						IsValidIndex( VIndex inIndex) const			{ return (inIndex >= 1 && inIndex <= GetCount()); }

			bool						AddNth( VValueBag* inBag, VIndex inIndex);
			bool						AddTail( VValueBag* inBag);
			
			VValueBag*					RetainNth( VIndex inIndex);	// Never NULL
			const VValueBag*			RetainNth( VIndex inIndex) const			{ return const_cast<VBagArray*>( this)->RetainNth( inIndex);}

			VValueBag*					GetNth( VIndex inIndex)						{ return fArray[inIndex-1]; }	// May be NULL
			const VValueBag*			GetNth( VIndex inIndex) const				{ return fArray[inIndex-1]; }	// May be NULL

			void						SetNth( VValueBag* inBag, VIndex inIndex);
			void						DeleteNth( VIndex inIndex);
			void						Destroy();

			void						Delete( VValueBag* inBag);

			VIndex						Find( VValueBag* inBag) const; // return 0 if not found

			VError						ReadFromStreamMinimal( VStream* ioStream);
			VError						WriteToStreamMinimal( VStream* ioStream) const;

			VError						_GetJSONString(VString& outJSONString, sLONG& curlevel, bool prettyformat, JSONOption inModifier) const;

protected:
			bags_vector					fArray;
};


/*!
	@class	VValueBag
	@abstract	
	@discussion
		Elements are stored in a refcounted array of type VBagArray.
		
		So you may write:
		
		VBagArray*	elements = tableBag.RetainElements(L"fields");
		for( sLONG i = 1; i <= elements.GetCount(); i++)
		{
			 VStr255 oneFieldName;
			 VValueBag* oneFieldBag = elements->GetNth(i);
			 oneFieldBag->GetString(L"name", oneFieldName);
			 DebugMsg("field no %d bears the name '%S'\n", i, &oneFieldName);
			 oneFieldBag->Release();
		}
		elements->Release();
*/


class XTOOLBOX_API VValueBag : public VValue, public IRefCountable
{
public:
	typedef StPackedDictionaryKey					StKey;
	typedef std::vector<StKey>						StKeyPath;
	typedef VValueInfo_default<VValueBag, VK_BAG>	TypeInfo;
	static const TypeInfo	sInfo;

	static	const int MAX_BAG_ELEMENTS_ORDERING = 10;

	struct StBagElementsOrdering
	{
		const StKey*	fName;
		const StKey*	fChildren[MAX_BAG_ELEMENTS_ORDERING];
	};

										VValueBag():fElements( NULL)										{;}
										VValueBag( const VValueBag& inOther);
	virtual								~VValueBag();
	
	
	/*
		Attribute and element names are a StKey class.
		This is wrapper that let you use char*, wchar_t* or VString& and StKey& as datatype for the name.

		bag.SetLong( "another id", 43);
		bag.SetLong( L"some other id", 44);
		bag.SetLong( my_vstring_key, 44);
		
		Don't do any charset translation yourself, let the StKey class do the work for you.

		The recommanded usage is to use the CREATE_BAGKEY / USE_BAGKEY macros:
		
		example:

		in .cpp file:	CREATE_BAGKEY( KEY_NAME);
		in .h file:		EXTERN_BAGKEY( KEY_NAME);
		usage:			bag.SetString( KEY_NAME, theName);
		
		You may want to define keys in a namespace:
		
		namespace RectKeys
		{
			CREATE_BAGKEY( left);
			CREATE_BAGKEY( right);
			CREATE_BAGKEY( top);
			CREATE_BAGKEY( bottom);
		};
		bag.SetReal( RectKeys::left, r.left);
		bag.SetReal( RectKeys::right, r.right);
		bag.SetReal( RectKeys::top, r.top);
		bag.SetReal( RectKeys::bottom, r.bottom);
		
	*/

			template<class T>	// may be used for sLONG, uLONG, sWORD, uWORD, OsType, etc... (does a static cast)
			void				SetLong( const StKey& inAttributeName, T inValue)							{ VLong value(static_cast<sLONG>(inValue)); fAttributes.SetValue( inAttributeName, value); }
			void				SetString( const StKey& inAttributeName, const VString& inValue)			{ fAttributes.SetValue( inAttributeName, inValue); }
			void				SetBoolean( const StKey& inAttributeName, Boolean inValue)					{ VBoolean value(inValue); fAttributes.SetValue( inAttributeName, value); }
			void				SetBool( const StKey& inAttributeName, bool inValue)						{ VBoolean value(static_cast<Boolean>( inValue )); fAttributes.SetValue( inAttributeName, value); }
			template<class T>
			void				SetReal( const StKey& inAttributeName, T inValue)							{ VReal value(static_cast<Real>(inValue)); fAttributes.SetValue( inAttributeName, value); }
			void				SetDuration( const StKey& inAttributeName, const VDuration& inValue)		{ fAttributes.SetValue( inAttributeName, inValue); }
			template<class T>
			void				SetLong8( const StKey& inAttributeName, T inValue)							{ VLong8 value(static_cast<sLONG8>(inValue)); fAttributes.SetValue( inAttributeName, value); }
			void				SetVUUID( const StKey& inAttributeName, const VUUID& inValue)				{ fAttributes.SetValue( inAttributeName, inValue); }
			void				SetTime( const StKey& inAttributeName, const VTime& inValue)				{ fAttributes.SetValue( inAttributeName, inValue); }
	
			template<class T>
			bool				GetLong( const StKey& inAttributeName, T& outValue) const					{ VLong v; bool found = fAttributes.GetValue( inAttributeName, v); outValue = static_cast<T>( v.GetLong()); return found; }
			bool				GetString( const StKey& inAttributeName, VString& outValue) const			{ bool found = fAttributes.GetValue( inAttributeName, outValue); if (!found) outValue.Clear(); return found;}
			bool				GetBoolean( const StKey& inAttributeName, Boolean& outValue) const			{ VBoolean v; bool found = fAttributes.GetValue( inAttributeName, v); outValue = v.GetBoolean(); return found; }
			bool				GetBool( const StKey& inAttributeName, bool& outValue) const				{ VBoolean v; bool found = fAttributes.GetValue( inAttributeName, v); outValue = v.GetBoolean() != 0; return found; }
			template<class T>
			bool				GetReal( const StKey& inAttributeName, T& outValue) const					{ VReal v; bool found = fAttributes.GetValue( inAttributeName, v); outValue = static_cast<T>( v.GetReal()); return found; }
			template<class T>
			bool				GetLong8( const StKey& inAttributeName, T& outValue) const					{ VLong8 v; bool found = fAttributes.GetValue( inAttributeName, v); outValue = static_cast<T>( v.GetLong8()); return found; }
			bool				GetVUUID( const StKey& inAttributeName, VUUID& outValue) const				{ bool found = fAttributes.GetValue( inAttributeName, outValue); if (!found) outValue.Clear(); return found;}
			bool				GetTime( const StKey& inAttributeName, VTime& outValue) const				{ bool found = fAttributes.GetValue( inAttributeName, outValue); if (!found) outValue.Clear(); return found;}
			bool				GetDuration( const StKey& inAttributeName, VDuration& outValue) const		{ bool found = fAttributes.GetValue( inAttributeName, outValue); if (!found) outValue.Clear(); return found;}


		/*!
			@function 	GetAttribute
			@abstract 	Returns an attribute value.
			@discussion	The bag stays the owner of the returned VValueSingle.
						Note that specialized versions such as GetLong or GetString
						are more optimized than GetAttribute because they don't have to instantiate the VValueSingle.
		*/
			const VValueSingle*	GetAttribute( const StKey& inAttributeName) const							{ return fAttributes.GetValue( inAttributeName); }
			VValueSingle*		GetAttribute( const StKey& inAttributeName)									{ return fAttributes.GetValue( inAttributeName); }	// different code, don't use const_cast
			template<class T>
			bool				GetAttribute( const StKey& inAttributeName, T& outValue) const				{ bool found = fAttributes.GetValue( inAttributeName, outValue);if (!found) outValue.Clear(); return found;}
			VIndex 				GetAttributeIndex( const StKey& inAttributeName) const						{ return fAttributes.GetIndex( inAttributeName); }
			bool				AttributeExists(const StKey& inAttributeName) const							{ return fAttributes.Find(inAttributeName); }


		/*!
			@function 	SetAttributes
			@abstract 	Set an attribute.
			@discussion	Previous attribute if any is disposed and replaced.
						The bag becomes the owner of the VValueSingle.
		*/
			void				SetAttribute( const StKey& inAttributeName, VValueSingle* inValue)					{ fAttributes.SetValue( inAttributeName, inValue); }
			void				SetAttribute( const StKey& inAttributeName, const VValueSingle& inValue)			{ fAttributes.SetValue( inAttributeName, inValue); }
			bool				SetExistingAttribute( const StKey& inAttributeName, const VValueSingle& inValue)	{ return fAttributes.SetExistingValue( inAttributeName, inValue); }

		/*!
			@function 	RemoveAttribute
			@abstract 	Remove specified attribute.
		*/
			void				RemoveAttribute( const StKey& inAttributeName)								{ fAttributes.Clear( inAttributeName);}

		/*!
			@function 	GetAttributesCount
			@abstract 	Returns the count of attributes
		*/
			VIndex				GetAttributesCount() const													{ return fAttributes.GetCount(); }

		/*!
			@function 	GetNthAttribute
			@abstract 	Returns the nth attribute value and optionnaly its name (outName can be null)
			@discussion	you can't modify the attribute, the bag stays the ownber of the VValueSingle.
		*/
			const VValueSingle*	GetNthAttribute( sLONG inIndex, VString *outName) const						{ return fAttributes.GetNthValue( inIndex, outName); }

		/*!
			@function 	GetElementNamesCount
			@abstract 	Returns the count of element sets
			@discussion	Each set of elements have the same name and is provided as an array of VValueBag.
						The bag stays the owner of the VBagArray.
		*/
			VIndex				GetElementNamesCount() const;
			VBagArray*			GetNthElementName( VIndex inIndex, VString *outName);
			const VBagArray*	GetNthElementName( VIndex inIndex, VString *outName) const					{ return const_cast<VValueBag*>( this)->GetNthElementName( inIndex, outName); }

		/*!
			@function 	IsEmpty
			@abstract 	Returns true if no attribute nor element.
		*/
			bool				IsEmpty() const;

		/*!
			@function 	GetElements
			@abstract 	Returns an array of elements
			@discussion	can return NULL if no element.
		*/
			VBagArray*			GetElements( const StKey& inElementName);
			const VBagArray*	GetElements( const StKey& inElementName) const								{ return const_cast<VValueBag*>( this)->GetElements( inElementName); }

		/*!
			@function 	RetainElements
			@abstract 	Returns an array of elements
			@discussion	Never return NULL( returns an empty array if no elements)
						Release the returned VBagArray after use.
		*/
			VBagArray*			RetainElements( const StKey& inElementName)									{ VBagArray *v = GetElements( inElementName); if (v) v->Retain(); else v = new VBagArray; return v; }
			const VBagArray*	RetainElements( const StKey& inElementName) const							{ return const_cast<VValueBag*>( this)->RetainElements( inElementName); }

		/*!
			@function 	GetElementsCount 	 
			@abstract 	Returns the count of elements of specified type
		*/
			VIndex				GetElementsCount( const StKey& inElementName) const							{ const VBagArray *v = GetElements( inElementName); return v ? v->GetCount() : 0; }
	
		/*!
			@function 	SetElements 	 
			@abstract 	Set a whole array of elements, given VBagArray is retained
			@discussion	Previous elements, if any and if not the same VBagArray, are removed and released.
						The VBagArray is retained on success.
		*/
			void				SetElements( const StKey& inElementName, VBagArray* inElements);
	
		/*!
			@function 	ReplaceElement 	 
			@abstract 	Replace all elements by specified one.
			@discussion	The VValueBag is retained on success.
						If inBag is NULL, RemoveElements is called instead.
		*/
			bool				ReplaceElement( const StKey& inElementName, VValueBag* inBag);
			bool				ReplaceElementByPath( const StKeyPath& inPath, VValueBag *inBag);

		/*!
			@function 	RemoveElements 	 
			@abstract 	Remove all elements of specified name.
		*/
			void				RemoveElements( const StKey& inElementName);

		/*!
			@function 	AddElement 	 
			@abstract 	Add an element.
			@discussion	The VValueBag is retained on success.
		*/
			void				AddElement( const StKey& inElementName, VValueBag* inBag);
			VError				AddElement( const IBaggable* inObject);
	
		/*!
			@function 	RetainUniqueElement
			@abstract 	Helper functions to return a unique element
			@discussion	Returns NULL if there's not one and ONLY one element of specified kind
		*/
			VValueBag*			RetainUniqueElement( const StKey& inElementName)
			{
				VBagArray* array = GetElements( inElementName);
				return ((array != NULL) && (array->GetCount() == 1)) ? array->RetainNth(1) : NULL;
			}
			const VValueBag*	RetainUniqueElement( const StKey& inElementName) const						{ return const_cast<VValueBag*>( this)->RetainUniqueElement( inElementName);}
			

			VValueBag*			RetainUniqueElementWithAttribute( const StKey& inElementName, const StKey& inAttributeName, const VValueSingle& inAttributeValue);
			const VValueBag*	RetainUniqueElementWithAttribute( const StKey& inElementName, const StKey& inAttributeName, const VValueSingle& inAttributeValue) const						{ return const_cast<VValueBag*>( this)->RetainUniqueElementWithAttribute( inElementName,inAttributeName,inAttributeValue);}

			VValueBag*			GetUniqueElement( const StKey& inElementName)
			{
				VBagArray* array = GetElements( inElementName);
				return ((array != NULL) && (array->GetCount() == 1)) ? array->GetNth(1) : NULL;
			}
			const VValueBag*	GetUniqueElement( const StKey& inElementName) const							{ return const_cast<VValueBag*>( this)->GetUniqueElement( inElementName);}

		/*!
			@function 	GetUniqueElementAnyKind
			@abstract 	Helper functions to return a unique element
			@discussion	Returns NULL if there's not one and ONLY one element of any kind
		*/
			VValueBag*			GetUniqueElementAnyKind( VString& outKind);
			const VValueBag*	GetUniqueElementAnyKind( VString& outKind) const							{ return const_cast<VValueBag*>( this)->GetUniqueElementAnyKind( outKind);}


			VValueBag*			GetUniqueElementByPath( const VString& inPath);
			const VValueBag*	GetUniqueElementByPath( const VString& inPath) const						{ return const_cast<VValueBag*>( this)->GetUniqueElementByPath( inPath);}
			VValueBag*			GetUniqueElementByPath( const StKeyPath& inPath);
			const VValueBag*	GetUniqueElementByPath( const StKeyPath& inPath) const						{ return const_cast<VValueBag*>( this)->GetUniqueElementByPath( inPath);}
			
			VValueBag*			GetElementByPathAndCreateIfDontExists( const StKeyPath& inPath);
			
	// Content accessing support

		/*!
			@function 	Destroy
			@abstract 	Remove all elements & properties
		*/
			void				Destroy();

		/*!
			@function 	GetCharacterData
			@abstract 	Retreive character data which is not an attribute nor an element for the XML spec
		 				but which is the special "<>" attribute for bags.
		*/
	static 	const StKey&		CDataAttributeName();
	
			bool				GetCData( VString& outValue) const			{return fAttributes.GetValue( CDataAttributeName(), outValue);}
			void				SetCData( const VString& inValue)			{fAttributes.SetValue( CDataAttributeName(), inValue);}
			void				AppendToCData( const VString& inValue);

		/*!
			@function 	UnionClone
			@abstract 	Set all attributes and add a copy of elements of specified bag.
			@discussion	Returns true on success.
		*/
			bool				UnionClone( const VValueBag *inBag);

		/*!
			@function 	Override
			@abstract 	Set all attributes and add a copy of elements of specified bag.
						If both bags have exactly one element of same kind, one is overriden by the other.
			@discussion	Returns true on success.
		*/
			bool				Override( const VValueBag *inOverridingBag);
			
	/*!
		 @function 	DumpXML
		 @abstract 	Convert into XML text
		 @discussion	
			Pass in inTitle the element name for this VValueBag.
			to obtain a truely valid xml document, prepend the text with something like:
			
				<?xml version="1.0" encoding="UTF-8"?>
				<!DOCTYPE application SYSTEM "xtoolbox.dtd">
	*/
			void				DumpXML( VString& ioDump, const VString& inTitle, bool inWithIndentation) const;

	// Inherited from VValue
	virtual	const VValueInfo*	GetValueInfo() const;

	virtual VError				ReadFromStream( VStream* inStream, sLONG inParam = 0);
			VError				ReadFromStreamMinimal( VStream* inStream);

	virtual VError				WriteToStream( VStream* inStream, sLONG inParam = 0) const;
			VError				WriteToStreamMinimal( VStream *inStream, bool inWithIndex) const;
			
	virtual CharSet				DebugDump( char* inTextBuffer, sLONG& inBufferSize) const;
	virtual	VValueBag*			Clone() const;

			void				SortElements( const StKey& inElementName, const StBagElementsOrdering inRules[]);

	// builds keys using path string. Keys are delimited by '/'
	static	void				GetKeysFromPath( const VString& inPath, StKeyPath& outKeys);
	// same thing but the keys use the specified path buffer without copying it. Make sure the buffer lives as long as the keys.
	static	void				GetKeysFromUTF8PathNoCopy( const char *inUTF8Path, StKeyPath& outKeys);

	virtual VError				FromJSONString(const VString& inJSONString, JSONOption inModifier = JSON_Default);
	virtual VError				GetJSONString(VString& outJSONString, JSONOption inModifier = JSON_Default) const;
			VError				_GetJSONString(VString& outJSONString, sLONG& curlevel, bool prettyformat, JSONOption inModifier) const;

			VJSONObject*        BuildJSONObject(VError& outError) const;



private:

			VPackedVValueDictionary		fAttributes;
			VPackedVBagArrayDictionary*	fElements;

			void						_DumpXML( VString& ioDump, const VString& inTitle, sLONG inIndentLevel) const;

	// Private utilities
			void						DumpXMLAttributes( VString& ioDump, const VString& inTitle, VIndex inCDataAttributeIndex, sLONG inIndentLevel) const;
			void						DumpXMLCData( VString& ioDump, VIndex inCDataAttributeIndex) const;
	static	void						DumpXMLIndentation( VString& ioDump, sLONG inIndentLevel);
	static	bool						NeedsEscapeSequence( const VString& inText, const UniChar* inEscapeFirst, const UniChar* inEscapeLast);

			void						_ReadFromStream_v1( VStream *inStream);
			void						_SortElements( const StBagElementsOrdering *inElementRule, const StBagElementsOrdering inRules[]);
};


#define CREATE_BAGKEY( _ID_)	const XBOX::VValueBag::StKey _ID_( #_ID_)
#define CREATE_BAGKEY_2( _ID_, _const_string_)	const XBOX::VValueBag::StKey _ID_( _const_string_ )
#define EXTERN_BAGKEY( _ID_)	extern const XBOX::VValueBag::StKey _ID_

#define CREATE_BAGKEY_WITH_DEFAULT( _ID_, _VVALUE_TYPE_, _VVALUE_DEFAULT_)			const XBOX::StBagKeyWithDefault<_VVALUE_TYPE_,_VVALUE_TYPE_> _ID_( #_ID_, _VVALUE_DEFAULT_)
#define EXTERN_BAGKEY_WITH_DEFAULT( _ID_, _VVALUE_TYPE_)					extern	const XBOX::StBagKeyWithDefault<_VVALUE_TYPE_,_VVALUE_TYPE_> _ID_

#define CREATE_BAGKEY_WITH_DEFAULT_SCALAR( _ID_, _VVALUE_TYPE_, _SCALAR_TYPE_, _SCALAR_DEFAULT_)		const XBOX::StBagKeyWithDefault<_SCALAR_TYPE_,_VVALUE_TYPE_> _ID_( #_ID_, _SCALAR_DEFAULT_)
#define EXTERN_BAGKEY_WITH_DEFAULT_SCALAR( _ID_, _VVALUE_TYPE_, _SCALAR_TYPE_)					extern	const XBOX::StBagKeyWithDefault<_SCALAR_TYPE_,_VVALUE_TYPE_> _ID_

#define CREATE_PATHBAGKEY_WITH_DEFAULT( _PATH_, _ID_, _VVALUE_TYPE_, _VVALUE_DEFAULT_)			const XBOX::StBagKeyWithDefault<_VVALUE_TYPE_,_VVALUE_TYPE_> _ID_( _PATH_, #_ID_, _VVALUE_DEFAULT_)
#define CREATE_PATHBAGKEY_WITH_DEFAULT_SCALAR( _PATH_, _ID_, _VVALUE_TYPE_, _SCALAR_TYPE_, _SCALAR_DEFAULT_)		const XBOX::StBagKeyWithDefault<_SCALAR_TYPE_,_VVALUE_TYPE_> _ID_( _PATH_, #_ID_, _SCALAR_DEFAULT_)

#define CREATE_PATHBAGKEY( _PATH_, _ID_)			const XBOX::StBagKeyWithDefault<sBYTE,XBOX::VByte> _ID_( _PATH_, #_ID_, 0)
#define EXTERN_PATHBAGKEY( _ID_)					extern	const XBOX::StBagKeyWithDefault<sBYTE,XBOX::VByte> _ID_

#define CREATE_BAGKEY_NO_DEFAULT( _ID_, _VVALUE_TYPE_)				const XBOX::StBagKeyNoDefault<_VVALUE_TYPE_,_VVALUE_TYPE_> _ID_( #_ID_)
#define EXTERN_BAGKEY_NO_DEFAULT( _ID_, _VVALUE_TYPE_)		extern	const XBOX::StBagKeyNoDefault<_VVALUE_TYPE_,_VVALUE_TYPE_> _ID_

#define CREATE_BAGKEY_NO_DEFAULT_SCALAR( _ID_, _VVALUE_TYPE_, _SCALAR_TYPE_)				const XBOX::StBagKeyNoDefault<_SCALAR_TYPE_,_VVALUE_TYPE_> _ID_( #_ID_)
#define EXTERN_BAGKEY_NO_DEFAULT_SCALAR( _ID_, _VVALUE_TYPE_, _SCALAR_TYPE_)		extern	const XBOX::StBagKeyNoDefault<_SCALAR_TYPE_,_VVALUE_TYPE_> _ID_



/*
	@class StBagKeyWithDefault
	@abstract	facility to handle default values in bag attributes
	@discussion	Each attribute in a bag usually has a name, a preferred value type and default value.
				The StBagKeyWithDefault template encapsulates all three elements,
				and let you write code like that:
				
				namespace keys
				{
					StBagKeyWithDefault<uLONG,VLong>		id("id", 0);	// id attribute is a sLONG, default value is zero
					StBagKeyWithDefault<VString,VString>	name("name",CVSTR("untitled"))	// name attribute is a VString, default value is "untitled"
				}
				
				void readbag( VValueBag *bag, uLONG *outID, VString& outName)
				{
					*outID = keys::id.Get( bag);
					keys::name.Get( bag, outName);
				}
				
				If bag is NULL or if the attribute does not exists, the returned value is the default value.
				If you set the default value, the attribute is removed from the bag.
*/

/*
	default template works best with scalar VValues such as VLong, VReal, etc...
*/
template<class T, class V>
class StBagKeyWithDefault
{
public:
	StBagKeyWithDefault( const char *inUTF8Key, const T& inDefault) : fKey( inUTF8Key), fDefault( inDefault)	{;}
	StBagKeyWithDefault( const char *inUTF8Path, const char *inUTF8Key, const T& inDefault) : fKey( inUTF8Key), fDefault( inDefault)	{ VValueBag::GetKeysFromUTF8PathNoCopy( inUTF8Path, fPath);}
	
	// lets you pass StBagKeyWithDefault directly as key parameter to VValueBag functions
	operator const VValueBag::StKey& () const	{ return fKey;}
	operator const VValueBag::StKey* () const	{ return &fKey;}
	
	const VValueBag::StKey&		GetKey() const			{ return fKey;}
	const VValueBag::StKeyPath& GetPath() const			{ return fPath;}
	
	// returns the default value if bag is NULL or attribute does not exists
	// inBag may be NULL
	T Get( const XBOX::VValueBag *inBag) const
		{
			if (inBag != NULL)
			{
				V temp;
				if (inBag->GetAttribute( fKey, temp))
					return caster<V,T>::castFrom( temp);
			}
			return fDefault;
		}

	bool Get( const XBOX::VValueBag *inBag, T& outValue) const
		{
			if ( (inBag != NULL) && inBag->GetAttribute( fKey, outValue))
				return true;

			outValue = fDefault;
			return false;
		}

	T GetUsingPath( const XBOX::VValueBag *inBag) const
		{
			const VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetUniqueElementByPath( fPath);
			if (bag != NULL)
			{
				V temp;
				if (bag->GetAttribute( fKey, temp))
					return caster<V,T>::castFrom( temp);
			}
			return fDefault;
		}

	T GetUsingPath( const XBOX::VValueBag *inBag, bool *outWasFound) const
		{
			const VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetUniqueElementByPath( fPath);
			if (bag != NULL)
			{
				V temp;
				if (bag->GetAttribute( fKey, temp))
				{
					if(outWasFound != NULL)
						*outWasFound = true;
					return caster<V,T>::castFrom( temp);
				}
			}

			if(outWasFound != NULL)
				*outWasFound = false;

			return fDefault;
		}

	bool GetUsingPath( const XBOX::VValueBag *inBag, T& outValue) const
		{
			const VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetUniqueElementByPath( fPath);
			if ( (bag != NULL) && bag->GetAttribute( fKey, outValue))
				return true;

			outValue = fDefault;
			return false;
		}
		
	T operator()( const XBOX::VValueBag *inBag) const
		{
			return Get( inBag);
		}
	
	bool HasAttribute( const XBOX::VValueBag *inBag) const
	{
		return (inBag != NULL) && inBag->AttributeExists( fKey);
	}
	
	bool HasAttributeUsingPath( const XBOX::VValueBag *inBag) const
	{
		const VValueBag *bag = (inBag != NULL) ? inBag->GetUniqueElementByPath( fPath) : NULL;
		return (bag != NULL) && bag->AttributeExists( fKey);
	}
	
	// remove the attribute if specified value equals the default value.
	// inBag may be NULL
	void Set( XBOX::VValueBag *inBag, const T& inValue) const
		{
			if (inBag != NULL)
			{
				if (inValue == fDefault)
					inBag->RemoveAttribute( fKey);
				else
				{
					inBag->SetAttribute( fKey, caster<V,T>::castTo( inValue) );
				}
			}
		}

	void SetUsingPath( XBOX::VValueBag *inBag, const T& inValue) const
		{
			if (inValue == fDefault)
			{
				VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetUniqueElementByPath( fPath);
				if (bag != NULL)
					inBag->RemoveAttribute( fKey);
			}
			else
			{
				VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetElementByPathAndCreateIfDontExists( fPath);
				if (bag != NULL)
				{
					bag->SetAttribute( fKey, caster<V,T>::castTo( inValue));
				}
			}
		}

	// Always sets the attribute, regardless the default value.
	// inBag cannot be NULL
	void SetForced( XBOX::VValueBag *inBag, const T& inValue) const
		{
			if (testAssert( inBag != NULL))
			{
				inBag->SetAttribute( fKey, caster<V,T>::castTo( inValue));
			}
		}

	void SetForcedUsingPath( XBOX::VValueBag *inBag, const T& inValue) const
		{
			VValueBag *bag = (inBag == NULL) ? NULL : inBag->GetElementByPathAndCreateIfDontExists( fPath);
			if (testAssert( bag != NULL))
			{
				bag->SetAttribute( fKey, caster<V,T>::castTo( inValue));
			}
		}

	T GetDefault() const
		{
			return fDefault;
		}
	
private:
	StBagKeyWithDefault( const StBagKeyWithDefault<T,V>& inOther);
	StBagKeyWithDefault &operator=( const StBagKeyWithDefault<T,V>& inOther);


			template<typename V1,typename T1>
			struct caster
			{
				static T1 castFrom( const V1& inValue)			{ return static_cast<T1>( inValue.Get());}
				static V1 castTo( const T1& inValue)			{ return V1( static_cast<typename V1::InfoType::backstore_type>( inValue));}
			};


			template<typename T1>
			struct caster<T1,T1>
			{
				static const T1& castFrom( const T1& inValue)	{ return inValue;}
				static const T1& castTo( const T1& inValue)		{ return inValue;}
			};
			
			template<typename V1>
			struct caster<V1,bool>
			{
				static bool castFrom( const V1& inValue)		{ return inValue.Get() != 0;}
				static V1 castTo( bool inValue)					{ return V1( inValue);}
			};

			const VValueBag::StKey				fKey;
			VValueBag::StKeyPath				fPath;
			const T								fDefault;

};


/*
	class StBagKeyNoDefault
	same behaviour as StBagKeyWithDefault without the default value
*/

template<class T, class V>
class StBagKeyNoDefault
{
public:
	StBagKeyNoDefault( const char *inKey) : fKey( inKey) {;}
	
	// lets you pass StBagKey directly as key parameter to VValueBag functions
	operator const VValueBag::StKey& () const	{ return fKey;}
	operator const VValueBag::StKey* () const	{ return &fKey;}
	
	// return true if the attribute exists
	// inBag may be NULL
	bool Get( const XBOX::VValueBag *inBag, T& outValue) const
		{
			bool found = false;

			if (inBag != NULL)
			{
				V temp;
				found = inBag->GetAttribute( fKey, temp);
				if (found)
					outValue = static_cast<T>( temp);
			}

			return found;
		}

	// set an attribute in the bag
	// inBag may be NULL
	void Set( XBOX::VValueBag *inBag, const T& inValue) const
		{
			if (inBag != NULL)
			{
				V value(inValue);
				inBag->SetAttribute( fKey, value);
			}
		}
	
private:

	StBagKeyNoDefault( const StBagKeyNoDefault<T,V>& inOther);
	StBagKeyNoDefault &operator=( const StBagKeyNoDefault<T,V>& inOther);

			const VValueBag::StKey	fKey;
};



template<>
class StBagKeyNoDefault<VString,VString>
{
public:
	typedef VString V;
	StBagKeyNoDefault( const char *inKey) : fKey( inKey) {;}
	
	// lets you pass StBagKey directly as key parameter to VValueBag functions
	operator const VValueBag::StKey& () const	{ return fKey;}
	operator const VValueBag::StKey* () const	{ return &fKey;}
	
	// return true if the attribute exists
	// inBag may be NULL
	bool Get( const XBOX::VValueBag *inBag, V& outValue) const
		{
			bool found = false;

			if (inBag != NULL)
				found = inBag->GetAttribute( fKey, outValue);
			
			return found;
		}

	// set an attribute in the bag
	// inBag may be NULL
	void Set( XBOX::VValueBag *inBag, const V& inValue) const
		{
			if (inBag != NULL)
				inBag->SetAttribute( fKey, inValue);
		}
	
private:

	StBagKeyNoDefault( const StBagKeyNoDefault<V,V>& inOther);
	StBagKeyNoDefault &operator=( const StBagKeyNoDefault<V,V>& inOther);

			const VValueBag::StKey	fKey;
};



/*!
	@class	IBaggable
	@abstract	Bag storage interface
	@discussion	Similar to IStreamable for bag support
*/
class XTOOLBOX_API IBaggable
{
public:
			IBaggable () {};
	virtual ~IBaggable () {};
	
	virtual VError	LoadFromBagWithLoader( const VValueBag& inBag, VBagLoader* inLoader, void* inContext = NULL);
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const = 0;
};


/*!
	@class	VBagLoader
	@abstract	Bag storage utility class
	@discussion	Handle common bag load/store job
*/
class XTOOLBOX_API VBagLoader : public VObject
{
public:
										VBagLoader( bool inRegenerateUUIDs, bool inFindReferencesByName);
	virtual								~VBagLoader();

		// uuids should be regenerated during the load.
			bool						RegenerateUUIDs() const									{ return fRegenerateUUIDs; }

		// all reference lookup should be done by name instead of by internal id/uuid.
			bool						FindReferencesByName() const							{ return fFindReferencesByName; }

		// loading should stop at first error.
			bool						StopOnError() const										{ return fStopOnError; }
			bool						StopOnError( const VString& /*inOffendingKind*/, const VValueBag& /*inOffendingBag*/) const { return fStopOnError; }
			
		// check for names validity and uniquiness. Default is true
			bool						WithNamesCheck() const									{ return fWithNamesCheck; }

			bool						IsLoadOnly() const										{ return fLoadOnly; }

			void						SetRegenerateUUIDs( bool inRegenerateUUIDs)				{ fRegenerateUUIDs = inRegenerateUUIDs; }
			void						SetFindReferencesByName( bool inFindReferencesByName)	{ fFindReferencesByName = inFindReferencesByName; }
			void						SetStopOnError( bool inStopOnError)						{ fStopOnError = inStopOnError; }
			void						SetWithNamesCheck( bool inWithNamesCheck)				{ fWithNamesCheck = inWithNamesCheck; }
			void						SetLoadOnly(bool inLoadOnly)							{ fLoadOnly = inLoadOnly; }

		// Reading a "id"/VUUID property from specified bag.
		// If RegenerateUUIDs() is true, then the uuid returned is a brand new one
		// and a map is maintained internally so that ResolveUUID() can return the generated uuid from a bag uuid.

		// Use GetUUID() to read the object uuids, and ResolveUUID for reference uuids.
			bool						GetUUID( const VValueBag& inBag, VUUID& outUUID);
			bool						ResolveUUID( VUUID& ioUUID) const;
	
protected:
			typedef std::map<VUUID,VUUID>	MapVUUID;
			bool						fRegenerateUUIDs;
			bool						fFindReferencesByName;
			bool						fStopOnError;
			bool						fWithNamesCheck;
			bool						fLoadOnly;
			MapVUUID					fUUIDs;
};

typedef VRefPtr<VValueBag>			VBagPtr;
typedef VRefPtr<const VValueBag>	VConstBagPtr;

XTOOLBOX_API VValueSingle *ParseXMLValue( const VString& inText);
XTOOLBOX_API void test_ParseXMLValue();




class BagHolder
{
	public:
		VValueBag* bag;

		inline BagHolder()
		{
			bag = new VValueBag();
		}

		inline ~BagHolder()
		{
			bag->Release();
		}

		operator VValueBag*() const { return bag; }
		operator VValueBag*() { return bag; }

		VValueBag& operator * () const { return *bag; }
		VValueBag* operator -> () const { return bag; }
};


class BagElement
{
	public:
		VValueBag* elembag;

		BagElement(VValueBag& Bag, const VValueBag::StKey& ElemName)
		{
			if (&Bag == NULL)
				elembag = NULL;
			else
			{
				elembag = new VValueBag();
				Bag.AddElement(ElemName, elembag);
			}
		}

		inline ~BagElement()
		{
			QuickReleaseRefCountable(elembag);
		}

		operator VValueBag*() const { return elembag; }
		operator VValueBag*() { return elembag; }

		VValueBag& operator * () const { return *elembag; }
		VValueBag* operator -> () const { return elembag; }
};



END_TOOLBOX_NAMESPACE

#endif
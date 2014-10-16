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
#ifndef __VPackedDictionary__
#define __VPackedDictionary__

#include "VStream.h"

BEGIN_TOOLBOX_NAMESPACE

//================================================================================================================

/*
	private
*/
class XTOOLBOX_API _VHashKeyMap
{
public:
	typedef size_t	hashcode_type;
	typedef std::vector<uLONG>	hash_vector;
	static	const uLONG tag_empty;

							_VHashKeyMap()						{;}

			bool			Put( hashcode_type inHash, size_t inValue);
			size_t			Get( hashcode_type inHash)			{ return static_cast<size_t>( fIndex[inHash % fIndex.size()]);}
	
			bool			empty() const						{ return fIndex.empty();}
			void			clear() 							{ return fIndex.clear();}
			
			bool			Rebuild( size_t inCountValues);
			bool			Rebuild( VStream *inCountValues);

			VError			WriteToStream( VStream *inStream);
	
private:
	static	size_t			_ComputeHashVectorSize( size_t inCountValues);

			hash_vector		fIndex;
};

//================================================================================================================


/*
	StPackedDictionaryKey is intended to virtualize what data type VPackedDictionary uses as a key.
	
	That could be:
		char*		null terminated utf8 string (typically hard-coded C-string)
		wchar_t*	null terminated wide char string (typically hard-coded C-string with L prefix)
		VString&	a VString
		
	Currently, the most optimized data type is char* but that could change in the future.
	So let the compiler choose what constructor to use and don't use StPackedDictionaryKey directly.
	
	It doesn't derive from VObject because it is intended to be automatically instantiated on the stack.
	(no need for virtual destructor penalty)
	
*/
class XTOOLBOX_API StPackedDictionaryKey
{
public:
	typedef char	char_type;
	typedef uBYTE	length_type;

	StPackedDictionaryKey( const char *inKey):fKey( inKey),fLength( ::strlen( inKey)),fKeyBuffer(NULL),fHashCode(GetHashCode( fKey, fLength))	{;}	// beware: initialization order = declaration order
	StPackedDictionaryKey( const char *inKey, size_t inLength);	// copy
	StPackedDictionaryKey( const char *inKey, size_t inLength, bool): fKey( inKey),fLength( inLength),fKeyBuffer(NULL),fHashCode(GetHashCode( inKey, inLength))	{;} // no copy
	StPackedDictionaryKey( const wchar_t *inKey);
	StPackedDictionaryKey( const VString& inKey);
	StPackedDictionaryKey( const StPackedDictionaryKey& inOther)	{ _CopyFrom( inOther);}
	~StPackedDictionaryKey()										{ delete [] fKeyBuffer;}

	// to ease writing of ordering rules
	operator const StPackedDictionaryKey* () const					{ return this;}

				size_t				GetKeyLength() const			{ return fLength;}
				const char_type*	GetKeyAdress() const			{ return fKey;}
	
				const char_type*	begin() const					{ return fKey;}
				const char_type*	end() const						{ return fKey + fLength;}

				bool				Equal( const StPackedDictionaryKey& inOther) const	{ return (fLength == inOther.fLength) && (::memcmp( fKey, inOther.fKey, fLength) == 0); }
				
	static 		size_t				CastLength( char_type inLength)	{ return static_cast<size_t>( *reinterpret_cast<length_type*>( &inLength));	}
	static 		char_type			CastLength( size_t inLength)
	{
		xbox_assert( inLength > 0 && inLength <= 255);
		length_type len = static_cast<length_type>( inLength);
		return *reinterpret_cast<char_type*>( &len);
	}
	
	static		_VHashKeyMap::hashcode_type GetHashCode( const char_type *inKey, size_t inLength);
				_VHashKeyMap::hashcode_type GetHashCode() const		{ return fHashCode;}

				StPackedDictionaryKey&	operator=(const StPackedDictionaryKey& inOther)	{ if (&inOther != this) { delete [] fKeyBuffer; _CopyFrom( inOther); } return *this; }


private:
				void				_CopyFrom( const StPackedDictionaryKey& inOther);
				
			const char_type*				fKey;
			size_t							fLength;
			char_type*						fKeyBuffer;
			_VHashKeyMap::hashcode_type		fHashCode;
};


//================================================================================================================

template<class SLOT_TYPE>
class VPackedDictionary_base : public VObject
{
public:
	typedef SLOT_TYPE										slot_type;
	typedef std::vector<SLOT_TYPE>							slot_vector;
	typedef std::vector<StPackedDictionaryKey::char_type>	key_vector;
	enum {threshold_for_hashkeymap = 5};	// automatically build a hash map for keys when count is greater than this threshold


			VPackedDictionary_base()
				{
				}
			
			VPackedDictionary_base( const VPackedDictionary_base& inOther):fKeys( inOther.fKeys), fSlots( inOther.fSlots)
				{
				}

			~VPackedDictionary_base()
				{
				}
				
			VIndex GetCount() const
				{
					return static_cast<VIndex>( fSlots.size());
				}
			
			bool IsEmpty() const
				{
					return fSlots.empty();
				}
			
			bool Find( const StPackedDictionaryKey& inKey) const
				{
					return _Find( inKey) != fSlots.end();
				}
			
			VIndex GetIndex( const StPackedDictionaryKey& inKey) const
				{
					typename slot_vector::const_iterator i = _Find( inKey);
					return (VIndex) ((i != fSlots.end()) ? (i - fSlots.begin() + 1) : 0);
				}
			
			/** @brief look for a slot key into an array of keys. **/
			/** returns 0 if not found, else its position in provided array **/
			VIndex GetNthKeyPosition( VIndex inIndex, const StPackedDictionaryKey * const inKeys[], VIndex inKeyCount)
				{
					VIndex position = 0;
					if (testAssert( inIndex > 0 && inIndex <= GetCount()) )
					{
						typename slot_vector::iterator i = fSlots.begin() + inIndex - 1;
						for( VIndex k = 0 ; k < inKeyCount ; ++k)
						{
							if (inKeys[k] == NULL)
								break;
							if (_Equal( *inKeys[k], *i))
							{
								position = k+1;
								break;
							}
						}
						
					}
					return position;
				}
			
			bool BuildHashKeyMapIfNecessary() const
				{
					if ( fHashKeyMap.empty() && (fSlots.size() > threshold_for_hashkeymap) )
					{
						if (fHashKeyMap.Rebuild( fSlots.size()))
							_FillHashKeyMap();
					}
					return !fHashKeyMap.empty();
				}

			const char*		GetKeyAdress( const slot_type& inSlot) const							{return &fKeys[inSlot.fOffsetKey + 1];}
	static	size_t			GetKeyLength( const key_vector& inKeys, const slot_type& inSlot)		{return StPackedDictionaryKey::CastLength( inKeys[inSlot.fOffsetKey]);}
			size_t			GetKeyLength( const slot_type& inSlot) const							{return GetKeyLength( fKeys, inSlot);}

protected:
			void _Clear()
				{
					fKeys.clear();
					fSlots.clear();
					fHashKeyMap.clear();
				}

			void _Clear( typename slot_vector::iterator inSlot)
				{
					// remove key value
					size_t keyLength = GetKeyLength( *inSlot) + 1;	// add one for length byte
					fKeys.erase( fKeys.begin() + inSlot->fOffsetKey, fKeys.begin() + inSlot->fOffsetKey + keyLength);
					
					// update keys offsets
					for( typename slot_vector::iterator i = inSlot + 1 ; i != fSlots.end() ; ++i)
						i->fOffsetKey -= keyLength;
					
					// remove slot
					fSlots.erase( inSlot);
					// invalidate hash table index
					fHashKeyMap.clear();
				}

			bool _Equal( const StPackedDictionaryKey& inKey, const slot_type& inSlot) const
				{
					return (inKey.GetKeyLength() == GetKeyLength( inSlot)) && (::memcmp( inKey.GetKeyAdress(), GetKeyAdress( inSlot), inKey.GetKeyLength()) == 0);
				}

			typename slot_vector::iterator _FindWithMap( const StPackedDictionaryKey& inKey)
				{
					size_t start = fHashKeyMap.Get( inKey.GetHashCode());
					if (start != _VHashKeyMap::tag_empty)
					{
						typename slot_vector::iterator i = fSlots.begin() + start; 
						do {
							if (_Equal( inKey, *i))
								return i;
						} while( ++i != fSlots.end());

						if (start != 0)
						{
							i = fSlots.begin();
							do {
								if (_Equal( inKey, *i))
									return i;
							} while( ++i != fSlots.begin() + start);
						}
					}

					return fSlots.end();
				}

			typename slot_vector::iterator _Find( const StPackedDictionaryKey& inKey)
				{
					if (BuildHashKeyMapIfNecessary())
						return _FindWithMap( inKey);

					typename slot_vector::iterator i = fSlots.begin();
					while( i != fSlots.end() && !_Equal( inKey, *i) )
						++i;
					return i;
				}

			typename slot_vector::const_iterator _Find( const StPackedDictionaryKey& inKey) const
				{
					return const_cast<VPackedDictionary_base*>( this)->_Find( inKey);
				}
			
			typename slot_vector::iterator _FindNth( VIndex inIndex, VString *outKeyName)
				{
					if (testAssert( inIndex > 0 && inIndex <= GetCount()) )
					{
						typename slot_vector::iterator i = fSlots.begin() + inIndex - 1;
						if (outKeyName)
							outKeyName->FromBlock( GetKeyAdress( *i), GetKeyLength( *i), VTC_UTF_8);
						return i;
					}
					else
					{
						if (outKeyName)
							outKeyName->Clear();
						return fSlots.end();
					}
				}
			
			typename slot_vector::const_iterator _FindNth( VIndex inIndex, VString *outKeyName) const
				{
					return const_cast<VPackedDictionary_base*>( this)->_FindNth( inIndex, outKeyName);
				}
			
			typename slot_vector::iterator _AppendKeyAndSlot( const StPackedDictionaryKey& inKey, const slot_type& inSlot)
				{
					try
					{
						fKeys.push_back( StPackedDictionaryKey::CastLength( inKey.GetKeyLength()));
						fKeys.insert( fKeys.end(), inKey.begin(), inKey.end());

						fSlots.push_back( inSlot);

						// update index if any
						if (!fHashKeyMap.empty())
						{
							_VHashKeyMap::hashcode_type hashcode = inKey.GetHashCode();
							if (!fHashKeyMap.Put( hashcode, fSlots.size() - 1))
							{
								// need rebuild
								if (fHashKeyMap.Rebuild( fSlots.size()))
									_FillHashKeyMap();
							}
						}

						return fSlots.end() - 1;
					}
					catch(...)
					{
						return fSlots.end();
					}
				}
			
			VError _ReadKeysFromStream( VStream *inStream)
				{
					VSize size_slots = inStream->GetLong();
					slot_type emptySlot = {};
					fSlots.resize( static_cast<size_t>( size_slots), emptySlot);

					VSize size_keys = inStream->GetLong();
					fKeys.resize( static_cast<size_t>( size_keys));
					size_keys *= sizeof( key_vector::value_type);
					inStream->GetData( &fKeys.front(), &size_keys);
					
					size_t current_key = 0;
					for( typename slot_vector::iterator i = fSlots.begin() ; i != fSlots.end() ; ++i)
					{
						i->fOffsetKey = current_key;
						current_key += GetKeyLength( fKeys, *i) + 1;
					}
					
					sBYTE withMap = inStream->GetByte();
					assert( withMap == 0 || withMap == 1);
					if (withMap == 1)
						fHashKeyMap.Rebuild( inStream);
					else
						BuildHashKeyMapIfNecessary();

					return inStream->GetLastError();
				}
			
			VError _WriteKeysToStream( VStream *inStream, bool inWithIndex) const
				{
					if (inWithIndex)
						BuildHashKeyMapIfNecessary();
					
					inStream->PutLong( static_cast<uLONG>( fSlots.size()));
					inStream->PutLong( static_cast<uLONG>( fKeys.size()));
					inStream->PutData( &fKeys.front(), static_cast<sLONG>( fKeys.size()));
					
					if (inWithIndex && !fHashKeyMap.empty())
					{
						inStream->PutByte( 1);
						fHashKeyMap.WriteToStream( inStream);
					}
					else
					{
						inStream->PutByte( 0);
					}
					
					return inStream->GetLastError();
				}
			
			void _FillHashKeyMap() const
				{
					for( typename slot_vector::const_iterator i = fSlots.begin() ; i != fSlots.end() ; ++i)
					{
						/*bool ok =*/fHashKeyMap.Put( StPackedDictionaryKey::GetHashCode( GetKeyAdress( *i), GetKeyLength( *i)), i - fSlots.begin());
					}
				}


			key_vector					fKeys;
			slot_vector					fSlots;
	mutable	_VHashKeyMap				fHashKeyMap;
};

//================================================================================================================

/*
	A packed dictionary is a map for (name,key) tuples.
	
	Only keys are packed in a unique memory buffer that can easily be streamed.
	
	Sorting the keys is currently not implemented because we don't need it right now.
	But that would be easy to do because there's already an index (fSlots).
*/

template< class T>
struct _VPackedDictionary_slot
{
	size_t						fOffsetKey;
	typename T::value_type*		fValue;
};

template<class T>
class VPackedDictionary : public VPackedDictionary_base<_VPackedDictionary_slot<T> >
{
public:
	typedef VPackedDictionary_base<_VPackedDictionary_slot<T> >			inherited;
	typedef typename T::value_type										value_type;
	typedef typename inherited::slot_type								slot_type;
	typedef typename inherited::slot_vector								slot_vector;
	typedef typename inherited::key_vector								key_vector;

			VPackedDictionary()
				{
				}
			
			VPackedDictionary( const VPackedDictionary& inOther):inherited( inOther)
				{
					size_t success_count = T::Clone( inherited::fSlots.begin(), inherited::fSlots.end());
					if (success_count != inherited::fSlots.size())	// mem failure
						throw;
				}

			~VPackedDictionary()
				{
					T::Clear( inherited::fSlots.begin(), inherited::fSlots.end());
				}
				
			VPackedDictionary& operator=(const VPackedDictionary& inOther)
				{
					if (this != &inOther)
					{
						inherited::operator=( inOther);
						/*size_t success_count =*/ T::Clone( inherited::fSlots.begin(), inherited::fSlots.end());
					}
					return *this;
				}

			void Clear()
				{
					T::Clear( inherited::fSlots.begin(), inherited::fSlots.end());
					inherited::_Clear();
				}

			void Clear( const StPackedDictionaryKey& inKey)
				{
					typename slot_vector::iterator i = inherited::_Find( inKey);
					if (i != inherited::fSlots.end())
					{
						T::Clear( i, i+1);
						inherited::_Clear( i);
					}
				}
	
			void Replace( value_type *inOldValue, value_type *inNewValue)
				{
					if (inOldValue != inNewValue)
					{
						for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
						{
							if (i->fValue == inOldValue)
							{
								T::SetValue( inNewValue, i->fValue);
								break;
							}
						}
					}
				}
				
			void Append( const StPackedDictionaryKey& inKey, value_type *inValue)
				{
					typename slot_vector::iterator i = _Append( inKey);
					if (i != inherited::fSlots.end())
						T::SetValue( inValue, i->fValue);
				}
			
			value_type* GetNthValue( VIndex inIndex, VString *outKeyName) const
				{
					return _GetValue( inherited::_FindNth( inIndex, outKeyName));
				}

			value_type* GetValue( const StPackedDictionaryKey& inKey) const
				{
					return _GetValue( inherited::_Find( inKey));
				}

			void SetValue( const StPackedDictionaryKey& inKey, value_type *inValue)
				{
					typename slot_vector::iterator i = inherited::_Find( inKey);
					if (i == inherited::fSlots.end())
						i = _Append( inKey);
					
					if (i != inherited::fSlots.end())
					{
						T::SetValue( inValue, i->fValue);
					}
				}
			
			VError WriteToStream( VStream *inStream, bool inWithIndex) const
				{
					inherited::_WriteKeysToStream( inStream, inWithIndex);

					// put values
					T::WriteToStream( inStream, inherited::fSlots.begin(), inherited::fSlots.end());

					return inStream->GetLastError();
				}
			
			VError ReadFromStream( VStream *inStream)
				{
					Clear();

					if (inStream->GetLastError() == VE_OK)
					{
						bool ok;
						try
						{
							ok = inherited::_ReadKeysFromStream( inStream) == VE_OK;
							if (ok)
							{
								// read values
								for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
									i->fValue = NULL;
								T::ReadFromStream( inStream, inherited::fSlots.begin(), inherited::fSlots.end());
							}
						}
						catch(...)
						{
							ok = false;
						}
					
						if (!ok || (inStream->GetLastError() != VE_OK))
							Clear();
					}

					return inStream->GetLastError();
				}

private:
				
			value_type *_GetValue( typename slot_vector::const_iterator inSlot) const
				{
					return (inSlot != inherited::fSlots.end()) ? inSlot->fValue : NULL;
				}
				
			typename slot_vector::iterator _Append( const StPackedDictionaryKey& inKey)
				{
					try
					{
						slot_type slot = { inherited::fKeys.size(), NULL};
						return inherited::_AppendKeyAndSlot( inKey, slot);
					}
					catch(...)
					{
						return inherited::fSlots.end();
					}
				}
};


//================================================================================================================

/*
	A packed dictionary with backstore is a map for (name,key) tuples
	where each key is maintained flat in a backstore memory buffer that can
	be easily streamed.
	
	As far as you don't ask for a value adress, the values are maintained flat
	in the backstore.
	If you ask for a value adress, the value is instantiated and kept by
	the dictionary.
	
	When a key value is accessed by adress from a non const dictionary, the dictionary is
	marked as needing packing: the value will be streamed back in the backstore.
*/



template< class T>
struct _VPackedDictionary_with_backstore_slot
{
			size_t						fOffsetKey;
			size_t						fOffsetValue;
	mutable typename T::value_type*		fValue;
};


template<class T>
class VPackedDictionary_with_backstore : public VPackedDictionary_base< _VPackedDictionary_with_backstore_slot< T> >
{
public:
	typedef VPackedDictionary_base< _VPackedDictionary_with_backstore_slot< T> >	inherited;
	typedef typename T::value_type										value_type;
	typedef typename inherited::slot_type								slot_type;
	typedef typename inherited::slot_vector								slot_vector;
	typedef typename inherited::key_vector								key_vector;

	/*
		l'unite est le uWORD.
		La taille d'une valeur est donc stockee sur un uWORD et tout est aligne sur 2.
		Si une valeur prend plus de 128ko, alors la taille est codee sur 3 uWORD dont le premier vaut 0xffff et les deux suivants la taille reelle.
		note: la taille est stockee en nombre d'elements dans le vecteur donc le nombre d'octets est obtenu en multipliant par sizeof( backstore_vector::value_type)
	*/
	typedef std::vector<uWORD>											backstore_vector;

			VPackedDictionary_with_backstore():fNeedPack( false)
				{
				}
			
			VPackedDictionary_with_backstore( const VPackedDictionary_with_backstore& inOther):inherited( inOther.Pack()), fNeedPack( false), fBackStore( inOther.Pack().fBackStore)
				{
					for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
						i->fValue = NULL;
				}

			~VPackedDictionary_with_backstore()
				{
					T::Clear( inherited::fSlots.begin(), inherited::fSlots.end());
				}
				
			VPackedDictionary_with_backstore& operator=(const VPackedDictionary_with_backstore& inOther)
				{
					if (this != &inOther)
					{
						inherited::operator=( inOther.Pack());
						fBackStore = inOther.fBackStore;
						fNeedPack = false;
						for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
							i->fValue = NULL;
					}
					return *this;
				}

			void Clear()
				{
					T::Clear( inherited::fSlots.begin(), inherited::fSlots.end());
					inherited::_Clear();
					fBackStore.clear();
					fNeedPack = false;
				}

			void Clear( const StPackedDictionaryKey& inKey)
				{
					typename slot_vector::iterator i = inherited::_Find( inKey);
					if (i != inherited::fSlots.end())
					{
						// clear cached value
						T::Clear( i, i+1);
						
						// remove backstore data
						typename slot_vector::iterator the_one_after = i + 1;
						backstore_vector::iterator erase_begin = fBackStore.begin() + i->fOffsetValue;
						backstore_vector::iterator erase_end = (the_one_after == inherited::fSlots.end()) ? fBackStore.end() : (fBackStore.begin() + the_one_after->fOffsetValue);
						size_t erase_size = erase_end - erase_begin;
						fBackStore.erase( erase_begin, erase_end);

						// update backstore offsets
						for(  ; the_one_after != inherited::fSlots.end() ; ++the_one_after)
							the_one_after->fOffsetValue -= erase_size;
						
						inherited::_Clear( i);
					}
				}

			void Replace( value_type *inOldValue, value_type *inNewValue)
				{
					if (inOldValue != inNewValue)
					{
						for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
						{
							if (i->fValue == inOldValue)
							{
								T::SetValue( inNewValue, i->fValue);
								fNeedPack = true;
								break;
							}
						}
					}
				}
	
			void Append( const StPackedDictionaryKey& inKey, const value_type& inValue)
				{
					typename slot_vector::const_iterator i = _Append( inKey, T::GetSavedValueSize( inValue));
					if (i != inherited::fSlots.end())
					{
						T::SaveValue( inValue, GetValueAdress( *i));
					}
				}
			
			const value_type* GetNthValue( VIndex inIndex, VString *outKeyName) const
				{
					return _GetValue( inherited::_FindNth( inIndex, outKeyName));
				}
			
			const value_type* GetValue( const StPackedDictionaryKey& inKey) const
				{
					return _GetValue( inherited::_Find( inKey));
				}

			value_type* GetValue( const StPackedDictionaryKey& inKey)
				{
					value_type *found = _GetValue( inherited::_Find( inKey));
					if (found)
						fNeedPack = true;
					return found;
				}

			bool GetValue( const StPackedDictionaryKey& inKey, value_type& outValue) const
				{
					return _GetValue( inherited::_Find( inKey), outValue);
				}

			void SetValue( const StPackedDictionaryKey& inKey, value_type *inValue)
				{
					typename slot_vector::const_iterator i = inherited::_Find( inKey);
					if (i == inherited::fSlots.end())
						i = _Append( inKey);
					
					if (i != inherited::fSlots.end())
					{
						T::SetValue( inValue, i->fValue);
						fNeedPack = true;
					}
				}

			bool SetExistingValue( const StPackedDictionaryKey& inKey, const value_type& inValue)
				{
					bool found;
					typename slot_vector::const_iterator i = inherited::_Find( inKey);
					if (i != inherited::fSlots.end())
					{
						// try to set the value directly in the backstore if it's not been already instantiated.
						if ( (i->fValue != NULL) || !T::SaveValueCheckSpace( inValue, GetValueAdress( *i)) )
						{
							T::SetValue( inValue, i->fValue);
							fNeedPack = true;
						}
						found = true;
					}
					else
					{
						found = false;
					}
					return found;
				}

			void SetValue( const StPackedDictionaryKey& inKey, const value_type& inValue)
				{
					if (!SetExistingValue( inKey, inValue))
					{
						Append( inKey, inValue);
					}
				}
			
			const VPackedDictionary_with_backstore& Pack() const
				{
					if (fNeedPack)
					{
						_Pack();
						fNeedPack = false;
					}
					return *this;
				}
			
			VError WriteToStream( VStream *inStream, bool inWithIndex) const
				{
					Pack();

					inherited::_WriteKeysToStream( inStream, inWithIndex);

					// put values
					inStream->PutLong( static_cast<sLONG>( fBackStore.size()));
					if (!inStream->NeedSwap())
					{
						inStream->PutData( &fBackStore.front(), fBackStore.size() * sizeof( backstore_vector::value_type));
					}
					else
					{
						for( typename slot_vector::const_iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
						{
							T::ByteSwap( GetValueAdress( *i), true);
						}

						inStream->PutData( &fBackStore.front(), fBackStore.size() * sizeof( backstore_vector::value_type));

						for( typename slot_vector::const_iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
						{
							T::ByteSwap( GetValueAdress( *i), false);
						}
					}
					return inStream->GetLastError();
				}
			
			VError ReadFromStream( VStream *inStream)
				{
					Clear();
			
					if (inStream->GetLastError() == VE_OK)
					{
						bool ok;
						try
						{
							ok = inherited::_ReadKeysFromStream( inStream) == VE_OK;
							if (ok)
							{
								// read values
								VSize size_values = inStream->GetLong();
								fBackStore.resize( static_cast<size_t>( size_values));
								size_values *= sizeof( backstore_vector::value_type);
								inStream->GetData( &fBackStore.front(), &size_values);

								if (inStream->GetLastError() == VE_OK)
								{
									size_t current_value = 0;
									for( typename slot_vector::iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
									{
										i->fValue = NULL;
										i->fOffsetValue = current_value;
										if (inStream->NeedSwap())
										{
											T::ByteSwap( GetValueAdress( *i), false);
										}
										current_value += GetNextValueOffset( fBackStore, *i);
									}
								}
							}
						}
						catch(...)
						{
							ok = false;
						}
					
						if (!ok || (inStream->GetLastError() != VE_OK))
							Clear();
					}

					return inStream->GetLastError();
				}

	static	void*			GetValueAdress( backstore_vector& inBackStore, const slot_type& inSlot)
				{
					return &inBackStore[inSlot.fOffsetValue];
				}

	static	void			PushValueAllocatedBytes( backstore_vector& inBackStore, size_t inValueBytes)
				{
					size_t valueSize = (inValueBytes + sizeof( backstore_vector::value_type) - 1 ) / sizeof( backstore_vector::value_type);
					inBackStore.resize( inBackStore.size() + valueSize);
				}
	
	static	size_t			GetValueAllocatedBytes( backstore_vector& inBackStore, const slot_type& inSlot)
				{
					return T::GetValueAllocatedBytes( GetValueAdress( inBackStore, inSlot));
				}

	static	size_t			GetNextValueOffset( backstore_vector& inBackStore, const slot_type& inSlot)
				{
					size_t value_bytes = T::GetValueAllocatedBytes( GetValueAdress( inBackStore, inSlot));
					size_t valueSize = (value_bytes + sizeof( backstore_vector::value_type) - 1 ) / sizeof( backstore_vector::value_type);
					return valueSize;
				}
	
			void*			GetValueAdress( const slot_type& inSlot) const							{return GetValueAdress( fBackStore, inSlot);}
			size_t			GetValueAllocatedBytes( const slot_type& inSlot) const					{return GetValueAllocatedBytes( fBackStore, inSlot);}

private:

			bool _GetValue( typename slot_vector::const_iterator inSlot, value_type& outValue) const
				{
					bool found;
					
					if (inSlot != inherited::fSlots.end())
					{
						found = true;
						T::Get( inSlot->fValue, GetValueAdress( *inSlot), outValue);
					}
					else
					{
						found = false;
					}
					
					return found;
				}
				
			value_type *_GetValue( typename slot_vector::const_iterator inSlot) const
				{
					value_type *value;

					if (inSlot != inherited::fSlots.end())
					{
						if (inSlot->fValue == NULL)
							inSlot->fValue = T::Load( GetValueAdress( *inSlot));
						value = inSlot->fValue;
					}
					else
					{
						value = NULL;
					}
					
					return value;
				}
				
			typename slot_vector::const_iterator _Append( const StPackedDictionaryKey& inKey)
				{
					try
					{
						slot_type slot = { inherited::fKeys.size(), fBackStore.size(), NULL};

						return inherited::_AppendKeyAndSlot( inKey, slot);
					}
					catch(...)
					{
						return inherited::fSlots.end();
					}
				}
				
			typename slot_vector::const_iterator _Append( const StPackedDictionaryKey& inKey, size_t inValueBytes)
				{
					try
					{
						slot_type slot = { inherited::fKeys.size(), fBackStore.size(), NULL};

						PushValueAllocatedBytes( fBackStore, inValueBytes);

						return inherited::_AppendKeyAndSlot( inKey, slot);
					}
					catch(...)
					{
						return inherited::fSlots.end();
					}
				}
			
			void _Pack() const
				{
					bool ok = true;
					for( typename slot_vector::const_iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i)
					{
						if (i->fValue != NULL)
						{
							ok = T::SaveValueCheckSpace( *i->fValue, GetValueAdress( *i));
							if (!ok)
								break;
						}
					}
					if (!ok)
					{
						try
						{
							backstore_vector	temp_backstore;
							slot_vector	temp_slots( inherited::fSlots);

							typename slot_vector::iterator temp_slots_i = temp_slots.begin();
							for( typename slot_vector::const_iterator i = inherited::fSlots.begin() ; i != inherited::fSlots.end() ; ++i , ++temp_slots_i)
							{
								temp_slots_i->fOffsetValue = temp_backstore.size();
								
								if (i->fValue != NULL)
								{
									size_t value_bytes = T::GetSavedValueSize( *i->fValue);
									PushValueAllocatedBytes( temp_backstore, value_bytes);
									T::SaveValue( *i->fValue, GetValueAdress( temp_backstore, *temp_slots_i));
								}
								else
								{
									size_t value_bytes = GetValueAllocatedBytes( *i);
									PushValueAllocatedBytes( temp_backstore, value_bytes);
									::memcpy( GetValueAdress( temp_backstore, *temp_slots_i), GetValueAdress( *i), value_bytes);
								}
							}
							fBackStore.swap( temp_backstore);
							const_cast<VPackedDictionary_with_backstore*>( this)->inherited::fSlots.swap( temp_slots);
						}
						catch(...)
						{
						}
					}
				}

	mutable	backstore_vector			fBackStore;
	mutable	bool						fNeedPack;
};


//================================================================================================================

class VValueSingle;
class VPackedValue_VValueSingle
{
public:
	typedef VValueSingle value_type;

	static const void* GetVValueDataPtr( const void *inData)
				{
					return reinterpret_cast<const char *>( inData) + sizeof( ValueKind);
				}

	static void* GetVValueDataPtr( void *inData)
				{
					return reinterpret_cast<char *>( inData) + sizeof( ValueKind);
				}
	
	static	size_t GetValueAllocatedBytes( const void *inData)
				{
					// some shortcuts
					ValueKind kind = *reinterpret_cast<const ValueKind *>( inData);
					switch( kind)
					{
						case VK_BOOLEAN:	return sizeof( ValueKind) + sizeof( sBYTE);
						case VK_BYTE:		return sizeof( ValueKind) + sizeof( Boolean);
						case VK_WORD:		return sizeof( ValueKind) + sizeof( sWORD);
						case VK_LONG:		return sizeof( ValueKind) + sizeof( sLONG);
						case VK_LONG8:		return sizeof( ValueKind) + sizeof( sLONG8);
						case VK_REAL:		return sizeof( ValueKind) + sizeof( Real);
						case VK_STRING:		return sizeof( ValueKind) + VString::sInfo.GetSizeOfValueDataPtr( GetVValueDataPtr( inData));
						case VK_DURATION:	return sizeof( ValueKind) + sizeof( sLONG8);
						default:
							{
								const VValueInfo *info = VValue::ValueInfoFromValueKind( kind);
								if (testAssert( info != NULL))
									return sizeof( ValueKind) + info->GetSizeOfValueDataPtr( GetVValueDataPtr( inData));
								else
									return sizeof( ValueKind);
							}
					}
				}
	
	static	value_type *Load( const void *inData)
				{
					value_type *value;
					ValueKind kind = *reinterpret_cast<const ValueKind *>( inData);
					const VValueInfo *info = VValue::ValueInfoFromValueKind( kind);
					if (testAssert( info != NULL))
					{
						value = (value_type*) info->LoadFromPtr( GetVValueDataPtr( inData), false);
					}
					else
					{
						value = NULL;
					}
					return value;
				}


	static	void Get( value_type*& ioValue, const void *inData, value_type& outValue)
				{
					if (ioValue == NULL)
					{
						ValueKind kind = *reinterpret_cast<const ValueKind *>( inData);
						const VValueInfo *info = VValue::ValueInfoFromValueKind( kind);
						if (testAssert( info != NULL))
						{
							if (outValue.GetTrueValueKind() == info->GetTrueKind())
							{
								// exactly same kind -> load from backstore
								outValue.LoadFromPtr( GetVValueDataPtr( inData), false);
							}
							else
							{
								// degenerate case -> load it and ask conversion
								ioValue = reinterpret_cast<value_type*>( info->LoadFromPtr( GetVValueDataPtr( inData), false));
								if (ioValue != NULL)
									ioValue->GetValue( outValue);
							}
						}
					}
					else
					{
						// already loaded -> ask conversion
						ioValue->GetValue( outValue);
					}
				}
	
	template <class ForwardIterator>
	static	void Clear( ForwardIterator inBegin, ForwardIterator inEnd)
				{
					for( ForwardIterator i = inBegin ; i != inEnd ; ++i)
					{
						delete i->fValue;
						i->fValue = NULL;
					}
				}
	
	static	void ByteSwap( void *inData, bool inFromNative)
				{
					assert_compile( sizeof( ValueKind) == sizeof( uWORD));
					
					ValueKind kind;
					if (inFromNative)
					{
						kind = *reinterpret_cast<ValueKind *>( inData);
						XBOX::ByteSwap( reinterpret_cast<uWORD *>( inData));
					}
					else
					{
						XBOX::ByteSwap( reinterpret_cast<uWORD *>( inData));
						kind = *reinterpret_cast<ValueKind *>( inData);
					}
					
					const VValueInfo *info = VValue::ValueInfoFromValueKind( kind);
					if (testAssert( info != NULL))
					{
						info->ByteSwapPtr( GetVValueDataPtr( inData), inFromNative);
					}
				}
	
	static	size_t GetSavedValueSize( const value_type& inValue)
				{
					return inValue.GetSpace() + sizeof( ValueKind);
				}

	static	void SaveValue( const value_type& inValue, void *inData)
				{
					*reinterpret_cast<ValueKind *>( inData) = inValue.GetTrueValueKind();
					inValue.WriteToPtr( GetVValueDataPtr( inData));
				}

	static	bool SaveValueCheckSpace( const value_type& inValue, void *inData)
				{
					if (GetValueAllocatedBytes( inData) == inValue.GetSpace() + sizeof( ValueKind))
					{
						SaveValue( inValue, inData);
						return true;
					}
					return false;
				}

	static	void SetValue( value_type* inValue, value_type*& ioCachedValue)
				{
					if (ioCachedValue != inValue)
					{
						delete ioCachedValue;
						ioCachedValue = inValue;
					}
				}

	static	void SetValue( const value_type& inValue, value_type*& ioCachedValue)
				{
					if (ioCachedValue != &inValue)
					{
						if ( (ioCachedValue != NULL) && (ioCachedValue->GetTrueValueKind() == inValue.GetTrueValueKind()) )
						{
							ioCachedValue->FromValueSameKind( &inValue);
						}
						else
						{
							delete ioCachedValue;
							ioCachedValue = (value_type *) inValue.Clone();
						}
					}
				}

};

typedef VPackedDictionary_with_backstore<VPackedValue_VValueSingle>	VPackedVValueDictionary;

END_TOOLBOX_NAMESPACE

#endif


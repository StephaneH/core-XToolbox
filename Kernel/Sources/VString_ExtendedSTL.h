/*
 *  VString_ExtendedSTL.h
 *  Kernel
 *
 *  Created by Jean-Daniel Guyot on 26/04/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */


#ifndef VSTRING_EXTENDED_STL
#define VSTRING_EXTENDED_STL

#include "Kernel/Sources/VString.h"

#if __GNUC__
#include <ext/hash_map>
#include <ext/hash_set>
#else
#include <hash_map>
#include <hash_set>
#endif


// hash_map or hash_set use needs some configuration tweaks depending on the compiler
#ifndef STL_EXT_NAMESPACE 
#if __GNUC__
#define STL_EXT_NAMESPACE ::__gnu_cxx
#elif VERSIONWIN
#if _MSC_VER>=1310
#define STL_EXT_NAMESPACE stdext
#else
#define STL_EXT_NAMESPACE std
#endif
#elif VERSIONMAC
#define STL_EXT_NAMESPACE Metrowerks
#endif
#endif

#ifndef STL_HASH_FUNCTOR_NAME
#if VERSIONMAC || VERSION_LINUX
#define STL_HASH_FUNCTOR_NAME hash
#elif VERSIONWIN
#define STL_HASH_FUNCTOR_NAME hash_compare
#endif


// Define the hash function and compare function for GCC and VS
#if __GNUC__
namespace __gnu_cxx
{
	using std::size_t;
	
	template<> struct STL_HASH_FUNCTOR_NAME< XBOX::VString >
	{
		size_t operator() ( const XBOX::VString& inString ) const {			
			return inString.GetHashValue();
		}
		
		bool operator() (const XBOX::VString& inString1, const XBOX::VString& inString2) const {
			return (inString1.CompareTo(inString2, true)) == XBOX::CR_SMALLER;
		}
	};
	
	
	template<> struct STL_HASH_FUNCTOR_NAME< XBOX::VString* >
	{
		size_t operator() ( const XBOX::VString* inString ) const {
			
			size_t hashResult = 0;
			if(inString != NULL && inString->GetCPointer() != NULL){
				return inString->GetHashValue();
			}
			return hashResult;
		}
		
		bool operator() (const XBOX::VString* inString1, const XBOX::VString* inString2) const {
			return (inString1->CompareTo(*inString2, true)) == XBOX::CR_SMALLER;
		}
	};
}
#else
namespace STL_EXT_NAMESPACE
{
	using std::size_t;
	
	template<> size_t STL_HASH_FUNCTOR_NAME< XBOX::VString, std::less< XBOX::VString > >::operator() ( const XBOX::VString& inString ) const {		
		return inString.GetHashValue();
	};
	
	template<> bool STL_HASH_FUNCTOR_NAME< XBOX::VString, std::less< XBOX::VString > >::operator() ( const XBOX::VString& inString1, const XBOX::VString& inString2) const {		
		return (inString1.CompareTo(inString2, true)) == XBOX::CR_SMALLER;
	};
}
#endif
#endif


// Structure you pass to a DeletableHashset
struct VString_Hash_Function
{
#if COMPIL_VISUAL
	enum
	{
		bucket_size = 4,
		min_buckets = 8
	};
	
	size_t operator()(const XBOX::VString* inString) const
	{
		return inString->GetHashValue();
	}
	
	bool operator()(const XBOX::VString* inString1, const XBOX::VString* inString2) const
	{
		return (inString1->CompareTo(*inString2, true)) == XBOX::CR_SMALLER;
	}
#else
	bool operator()(const XBOX::VString* inString1, const XBOX::VString* inString2) const
	{
		return *inString1 == *inString2;
	}
#endif
};

template <class Value, class CompareFunc> class DeletableHashSet
{
public:
	
	DeletableHashSet ( ){
		fHashset = new InnerHashSet;
	}
	
	~DeletableHashSet ( ){
		
		//This is a little bit tricky to delete pointers in a hashset (with GCC), because the implementation need to keep valid objects while iterating. Only solution found: Put all the pointers in a vector and iterate through the vector to delete everything.  
		std::vector< XBOX::VString * > vectorOfPointers;
		for ( DeletableHashSetIterator i = fHashset-> begin ( ); i != fHashset-> end ( ); i++ ){
			if ( (*i) != NULL ){
				vectorOfPointers.push_back(*i);
			}
		}
		
		std::vector< XBOX::VString * >::iterator vectorOfPointersIterator = vectorOfPointers.begin();
		while (vectorOfPointersIterator != vectorOfPointers.end()) {
			delete(*vectorOfPointersIterator);
			(*vectorOfPointersIterator) = NULL;
			++vectorOfPointersIterator;
		}
		
		delete fHashset;
	}
	
#if COMPIL_VISUAL
	typedef STL_EXT_NAMESPACE::hash_set< Value, CompareFunc >		InnerHashSet;
#else
	typedef STL_EXT_NAMESPACE::hash_set< Value, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Value>, CompareFunc >		InnerHashSet;
#endif
	
	InnerHashSet*		fHashset;
	typedef typename InnerHashSet::iterator			DeletableHashSetIterator;
	typedef typename InnerHashSet::value_type		DeletableHashSetValueType;
};

template <class Key, class Value> class DeletableHashMap
{
public:
	
	DeletableHashMap ( ){
		fHashmap = new STL_EXT_NAMESPACE::hash_map<Key, Value*, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Key> >;
	}
	
	~DeletableHashMap ( ){
		for ( typename STL_EXT_NAMESPACE::hash_map<Key, Value*, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Key> >::iterator i = fHashmap->begin(); i != fHashmap->end(); i++ )
			if ( (*i).second )
				delete (*i).second;
		
		delete fHashmap;
	}
	
	STL_EXT_NAMESPACE::hash_map<Key, Value*, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Key> >*									fHashmap;
	typedef typename STL_EXT_NAMESPACE::hash_map<Key, Value*, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Key> >::iterator			DeletableHashMapIterator;
	typedef typename STL_EXT_NAMESPACE::hash_map<Key, Value*, STL_EXT_NAMESPACE::STL_HASH_FUNCTOR_NAME<Key> >::value_type		DeletableHashMapValueType;
};


/*
	we can now start to use TR1 in VisualStudio 2008 and MacOSX10.5 SDK.
	
	hash_VString can be used in unordered_map.
	Can't use a collator because the hash code is computed directly on unicode code points.
	If we need collation, we may have to use icu sort keys instead.
	Another possibility is to use VIntl::ToUpperLowerCase on strings to be inserted in the hash table.

	typedef std::tr1::unordered_map<XBOX::VString, int, hash_VString::hash, hash_VString::equal_to> mymap;
	mymap     map;
	std::pair<mymap::const_iterator, bool> result1 = map.insert( mymap::value_type( "hello", 1));
*/

#if COMPIL_VISUAL
#include <unordered_map>
#define NAMESPACE_TR1 std::tr1
#else
#if defined( _LIBCPP_VERSION )
#include <unordered_map>
#define NAMESPACE_TR1 std
#else
#include <tr1/unordered_map>
#define NAMESPACE_TR1 std::tr1
#endif
#endif

BEGIN_TOOLBOX_NAMESPACE

namespace hash_VString
{
	struct equal_to : std::binary_function<VString, VString, bool>
	{
		explicit equal_to()	{}
		equal_to( const equal_to&)	{}

		template<typename String1,typename String2>
		bool	operator()( const String1& s1, const String2& s2) const
		{
			return (s1.GetLength() == s2.GetLength()) && (memcmp( s1.GetCPointer(), s2.GetCPointer(), s1.GetLength() * 2 /*sizeof( UniChar)*/ ) == 0);
		}
	};

	struct hash : std::unary_function<VString, std::size_t>
	{
		template<typename String>
		size_t operator()( const String& s) const						{ return s.GetHashValue();}
	};

};

/*
	specialized template for using unordered_map on a VString key.
	
	typedef unordered_map_VString<int>	mymap;
	mymap	map;
	map.insert( mymap::value_type( "hello", 1));
*/
template<class Value>
class unordered_map_VString : public NAMESPACE_TR1::unordered_map<XBOX::VString,Value,XBOX::hash_VString::hash, XBOX::hash_VString::equal_to>
{
public:
	unordered_map_VString( size_t inBuckets = 10)
		: NAMESPACE_TR1::unordered_map<VString,Value,hash_VString::hash, hash_VString::equal_to>( inBuckets, hash_VString::hash(), hash_VString::equal_to()) {}

	template <typename InputIterator>
	unordered_map_VString( InputIterator begin, InputIterator end) : NAMESPACE_TR1::unordered_map<VString,Value,hash_VString::hash, hash_VString::equal_to>( begin, end) {}
};

template<class Value>
class unordered_multimap_VString : public NAMESPACE_TR1::unordered_multimap<XBOX::VString,Value,XBOX::hash_VString::hash, XBOX::hash_VString::equal_to>
{
public:
	unordered_multimap_VString( size_t inBuckets = 10)
		: NAMESPACE_TR1::unordered_multimap<VString,Value,hash_VString::hash, hash_VString::equal_to>( inBuckets, hash_VString::hash(), hash_VString::equal_to()) {}

	template <typename InputIterator>
	unordered_multimap_VString( InputIterator begin, InputIterator end) : NAMESPACE_TR1::unordered_multimap<VString,Value,hash_VString::hash, hash_VString::equal_to>( begin, end) {}
};

END_TOOLBOX_NAMESPACE


#endif //VSTRING_EXTENDED_STL

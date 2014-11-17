#ifndef __NAME_VALUE_COLLECTION_INCLUDED__
#define __NAME_VALUE_COLLECTION_INCLUDED__


BEGIN_TOOLBOX_NAMESPACE

#include <map>


class XTOOLBOX_API VNameValueCollection : public XBOX::VObject
{
public:
								VNameValueCollection();
	virtual						~VNameValueCollection();

	struct LessThan_ComparisonFunction
	{
		bool operator() (const XBOX::VString& inString1, const XBOX::VString& inString2) const
		{
			return (inString1.CompareToString (inString2) == XBOX::CR_SMALLER);
		}
	};

	typedef std::multimap<XBOX::VString, XBOX::VString, LessThan_ComparisonFunction> NameValueMap;
	typedef NameValueMap::iterator Iterator;
	typedef NameValueMap::const_iterator ConstIterator;

	void						Set (const XBOX::VString& inName, const XBOX::VString& inValue);	
	void						Add (const XBOX::VString& inName, const XBOX::VString& inValue);
	const XBOX::VString&		Get (const XBOX::VString& inName) const;
	bool						Has (const XBOX::VString& inName) const;

	ConstIterator				find (const XBOX::VString& inName) const;
	Iterator					find (const XBOX::VString& inName);

	ConstIterator				begin() const;
	Iterator					begin();
	ConstIterator				end() const;
	Iterator					end();
	bool						empty() const;
	int							size() const;
	void						erase (const XBOX::VString& inName);
	void						erase (Iterator& inIter);
	void						clear();

private:
	NameValueMap				fMap;
};


END_TOOLBOX_NAMESPACE

#endif	// __NAME_VALUE_COLLECTION_INCLUDED__

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

#include "VServerNetPrecompiled.h"
#include "VNameValueCollection.h"
#include "HTTPTools.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE


//--------------------------------------------------------------------------------------------------


const VString	CONST_EMPTY_STRING = CVSTR ("");


//--------------------------------------------------------------------------------------------------


VNameValueCollection::VNameValueCollection()
{
}


VNameValueCollection::~VNameValueCollection()
{
	fMap.clear();
}


void VNameValueCollection::Set (const VString& inName, const VString& inValue)
{
	Iterator it = fMap.find(inName);
	if (it != fMap.end())
		it->second.FromString (inValue);
	else
		fMap.insert (NameValueMap::value_type (inName, inValue));
}


void VNameValueCollection::Add (const VString& inName, const VString& inValue)
{
	fMap.insert (NameValueMap::value_type (inName, inValue));
}


const VString& VNameValueCollection::Get (const VString& inName) const
{
	ConstIterator it = fMap.find (inName);
	if (it != fMap.end())
		return it->second;
	else
		return CONST_EMPTY_STRING;
}


bool VNameValueCollection::Has (const VString& inName) const
{
	return (fMap.count (inName) > 0);
}


VNameValueCollection::ConstIterator VNameValueCollection::find (const VString& inName) const
{
	return fMap.find (inName);
}


VNameValueCollection::Iterator VNameValueCollection::find (const VString& inName)
{
	return fMap.find (inName);
}


VNameValueCollection::ConstIterator VNameValueCollection::begin() const
{
	return fMap.begin();
}


VNameValueCollection::Iterator VNameValueCollection::begin()
{
	return fMap.begin();
}


VNameValueCollection::ConstIterator VNameValueCollection::end() const
{
	return fMap.end();
}


VNameValueCollection::Iterator VNameValueCollection::end()
{
	return fMap.end();
}


bool VNameValueCollection::empty() const
{
	return fMap.empty();
}


int VNameValueCollection::size() const
{
	return (int) fMap.size();
}


void VNameValueCollection::erase (const VString& inName)
{
	fMap.erase (inName);
}


void VNameValueCollection::erase (Iterator& inIter)
{
	fMap.erase (inIter);
}


void VNameValueCollection::clear()
{
	fMap.clear();
}


END_TOOLBOX_NAMESPACE

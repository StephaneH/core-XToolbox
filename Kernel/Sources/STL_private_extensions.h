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
#ifndef __STL_private_extensions__
#define __STL_private_extensions__

BEGIN_TOOLBOX_NAMESPACE

/*
	this file contains extensions to STL that mostly come from http://www.boost.org
	and will ultimately belong to the standard.
	
*/



/*
	is_base_and_derived is a helper class for the "base-driven template specialization" idiom.

	see http://www.boost.org
*/
template<class Base, class Candidate>
class is_base_and_derived
{
public:
	typedef char (&no)[1];
	typedef char (&yes)[2];
	static yes Test(Base);
	static no Test(...);
	enum { value = sizeof(Test(Candidate())) == sizeof(yes) };
};




template <class T> struct remove_pointer                   {typedef T type;};
template <class T> struct remove_pointer<T*>               {typedef T type;};
template <class T> struct remove_pointer<T*const>          {typedef T type;};
template <class T> struct remove_pointer<T*volatile>       {typedef T type;};
template <class T> struct remove_pointer<T*const volatile> {typedef T type;};

template <class T, class U> struct is_same       {static const bool value = false;};
template <class T>          struct is_same<T, T> {static const bool value = true;};

template <class T> struct is_pointer
	{static const bool value = !is_same<T, typename remove_pointer<T>::type>::value;};

/*
	see http://www.boost.org/libs/utility/enable_if.html 
*/
template <bool B, class T = void>
struct enable_if_c
{
  typedef T type;
};

template <class T>
struct enable_if_c<false, typename T> {};

template <class Cond, class T = void>
struct enable_if : public enable_if_c<Cond::value, T> {};

END_TOOLBOX_NAMESPACE

#endif

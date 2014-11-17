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
#ifndef __XTOOLBOX_KERNEL_NON_COPYABLE_H__
# define __XTOOLBOX_KERNEL_NON_COPYABLE_H__




BEGIN_TOOLBOX_NAMESPACE

/*!
** \brief Prevent objects of a class from being copy-constructed or assigned to each other
**
** \code
** #include <vector>
**
** class CannotBeCopied final : public VNonCopyable<CannotBeCopied>
** {
** };
**
** int main()
** {
**	CannotBeCopied a;
**	CannotBeCopied b;
**
**	// explicit copy, compilation will fail in both cases
**	a = b;
**	CannotBeCopied c(a);
**
**	// compilation will fail in this case if there is not move semantics
**	std::vector<CannotBeCopied> array;
**	array.push_back(a); // compilation will fail
**	array.push_back(std::move(a)); // ok
**	return 0;
** }
** \endcode
**
** \internal CRTP based solution. This allows for Empty Base Optimization with multiple inheritance
*/
template<class ChildT>
class VNonCopyable
{
public:
	//! Default constructor
	VNonCopyable()
	{
		// empty on purpose, to not have to explicitly initialize the
		// base class from 'ChildT'
	}


private:
	//! Private copy constructor
	VNonCopyable(const VNonCopyable&) {} // = delete (c++11)
	//! Private assignement operator
	// (the implementation is provided to avoid compilation issues with Visual Studio)
	template<class T>
	VNonCopyable& operator = (const T&) {return *static_cast<T*>(this);} // = delete (c++11)

}; // class VNonCopyable






END_TOOLBOX_NAMESPACE

#endif // __XTOOLBOX_KERNEL_NON_COPYABLE_H__


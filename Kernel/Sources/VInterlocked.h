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
#ifndef __VInterlocked__
#define __VInterlocked__

#if VERSIONMAC
#include <libkern/OSAtomic.h>
#endif


BEGIN_TOOLBOX_NAMESPACE

/*
	@class VInterlocked
	@abstract	Interlocked services
	@discussion
		Is a namespace to group together interlocking services.
*/

class XTOOLBOX_API VInterlocked
{
public:
	
	// returns the value after increment / decrement
	static	sLONG		Increment           (sLONG* inValue);
	static	sLONG		Decrement           (sLONG* inValue);
	static  sLONG		AtomicAdd           (sLONG* inValue, sLONG inAddValue);

    static  sLONG		AtomicGet           (sLONG* inValue)  { return AtomicAdd(inValue, 0); }

	// Support for thread-safe compare-exchange, returns intial content of inValue
	static	sLONG		CompareExchange     (sLONG* inValue, sLONG inCompareValue, sLONG inNewValue);
	static	void*		CompareExchangePtr  (void** inValue, void* inCompareValue, void* inNewValue);
	static	sLONG		Exchange            (sLONG* inValue, sLONG inNewValue);
#if ARCH_64
	static	sLONG8		Exchange            (sLONG8* inValue, sLONG8 inNewValue);
#endif
	static	void*		ExchangeVoidPtr     (void** inValue, void* inNewValue);

	template<class Type>
	static  Type*		ExchangePtr         (Type** inValue, Type* inNewValue = NULL) { return (Type*) ExchangeVoidPtr( (void**) inValue, inNewValue); }

	static  bool		TestAndSet          (sLONG *inValue, sLONG inBitNumber);
	static  bool		TestAndClear        (sLONG *inValue, sLONG inBitNumber);


private:
	// not intended to be instantiated
						VInterlocked();
};

END_TOOLBOX_NAMESPACE

#endif
